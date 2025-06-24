#include "Command.h"

void Command::handleTopic(const Message &msg)
{
	std::shared_ptr<Client> client_ptr = msg.getClientPtr();
	if (!client_ptr)
	{
		std::cerr << ("null ptr in handleTopic") << std::endl;
		return;
	}
	std::vector<std::string> parameters = msg.getParameters();
	int client_fd = client_ptr->getFd();
	if (parameters.empty())
	{
		server_ptr_->sendResponse(client_fd, ERR_NEEDMOREPARAMS(client_ptr->getClientPrefix(), "TOPIC"));
		return;
	}

	std::string channel_name = parameters[0];
	std::shared_ptr<Channel> channel_ptr = server_ptr_->findChannel(channel_name);
	std::pair<std::string, std::string> topic;

	if (!channel_ptr)
	{
		server_ptr_->sendResponse(client_fd, ERR_NOSUCHCHANNEL(server_ptr_->getServerHostname(), client_ptr->getNickname(), channel_name));
		return;
	}

	if (!channel_ptr->isUserOnChannel(client_ptr->getNickname()))
	{
		server_ptr_->sendResponse(client_fd, ERR_NOTONCHANNEL(server_ptr_->getServerHostname(), client_ptr->getNickname(), channel_name));
		return;
	}

	if (msg.hasTrailer())
	{
		std::string new_topic = msg.getTrailer();
		if (new_topic.size() > TOPIC_MAX_LENGTH) // if topic is too long
			new_topic = new_topic.substr(0, TOPIC_MAX_LENGTH);
		if (channel_ptr->getModeT() && !channel_ptr->isOperator(client_ptr))
		{
			server_ptr_->sendResponse(client_fd, ERR_CHANOPRIVSNEEDED(server_ptr_->getServerHostname(), channel_ptr->getName()));
			return;
		}
		if (new_topic.empty())
		{
			channel_ptr->clearTopic(client_ptr->getNickname());
			channel_ptr->broadcastMessageToAll(RPL_TOPICCHANGE(client_ptr->getClientPrefix(), channel_ptr->getName(), ""), server_ptr_);
		}
		else
		{
			channel_ptr->setTopic(make_pair(client_ptr->getNickname(), new_topic));
			channel_ptr->broadcastMessageToAll(RPL_TOPICCHANGE(client_ptr->getClientPrefix(), channel_ptr->getName(), new_topic), server_ptr_);
		}
	}
	else
		channel_ptr->sendTopicToClient(client_ptr, server_ptr_);
}

