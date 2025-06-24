#include "Command.h"

void Command::handlePart(const Message &msg)
{
	std::shared_ptr<Client> client_ptr = msg.getClientPtr();
	if (!client_ptr)
	{
		std::cerr << ("null ptr in handlePart") << std::endl;
		return;
	}
	int client_fd = client_ptr->getFd();
	std::vector<std::string> parameters = msg.getParameters();

	if (parameters.empty())
	{
		server_ptr_->sendResponse(client_fd, ERR_NEEDMOREPARAMS(client_ptr->getClientPrefix(), "PART"));
		return;
	}

	std::vector<std::string> channels = split(parameters[0], ',');
	std::string part_message = msg.getTrailer().empty() ? client_ptr->getNickname() : msg.getTrailer(); // default part message is the nickname

	for (const std::string &channel_name : channels)
	{
		std::shared_ptr<Channel> channel_ptr = server_ptr_->findChannel(channel_name);
		if (!channel_ptr)
		{
			server_ptr_->sendResponse(client_fd, ERR_NOSUCHCHANNEL(server_ptr_->getServerHostname(), client_ptr->getNickname(), channel_name));
			continue;
		}

		if (!channel_ptr->isUserOnChannel(client_ptr->getNickname()))
		{
			server_ptr_->sendResponse(client_fd, ERR_NOTONCHANNEL(server_ptr_->getServerHostname(), client_ptr->getNickname(), channel_ptr->getName()));
			continue;
		}
		channel_ptr->removeUser(client_ptr);
		client_ptr->leaveChannel(channel_ptr);
		server_ptr_->sendResponse(client_fd, RPL_PART(client_ptr->getClientPrefix(), channel_name, part_message));
		channel_ptr->broadcastMessage(client_ptr, RPL_PART(client_ptr->getClientPrefix(), channel_name, part_message), server_ptr_);
		if (channel_ptr->isEmpty())
		{
			server_ptr_->deleteChannel(channel_ptr->getName());
			return;
		}
	}
}