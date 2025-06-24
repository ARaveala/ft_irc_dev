#include "Command.h"
#include "../server/Server.h"

void Command::handleJoin(const Message &msg)
{
	std::shared_ptr<Client> client_ptr = msg.getClientPtr();
	if (!client_ptr)
	{
		std::cerr << ("null ptr in handleJoin") << std::endl;
		return;
	}
	int client_fd = client_ptr->getFd();
	if (!client_ptr->getRegisterStatus())
	{
		server_ptr_->sendResponse(client_fd, ERR_NOTREGISTERED(server_ptr_->getServerHostname()));
		return;
	}

	std::vector<std::string> parameters = msg.getParameters();
	if (parameters.empty())
	{
		server_ptr_->sendResponse(client_fd, ERR_NEEDMOREPARAMS(client_ptr->getClientPrefix(), "JOIN"));
		return;
	}
	std::vector<std::string> channels = split(parameters[0], ',');
	std::vector<std::string> keys;
	if (parameters.size() > 1)
		keys = split(parameters[1], ',');
	for (size_t i = 0; i < channels.size(); i++)
	{
		std::string channel_name = channels[i];
		if (client_ptr->getChannels().size() >= CLIENT_MAX_CHANNELS)
		{
			server_ptr_->sendResponse(client_fd, ERR_TOOMANYCHANNELS(server_ptr_->getServerHostname(), client_ptr->getNickname(), channel_name));
			continue;
		}
		if (channel_name.size() > CHANNEL_NAME_MAX_LENGTH)
			channel_name = channel_name.substr(0, CHANNEL_NAME_MAX_LENGTH);
		if (!isValidChannelName(channel_name))
		{
			server_ptr_->sendResponse(client_fd, ERR_NOSUCHCHANNEL(server_ptr_->getServerHostname(), client_ptr->getNickname(), channel_name));
			continue;
		}

		std::shared_ptr<Channel> channel_ptr = server_ptr_->findChannel(channel_name);
		char prefix = channel_name.front();
		if (!channel_ptr)
		{
			// Handling based on channel prefix
			switch (prefix)
			{
			case '#': // Standard channels
			case '&': // Local to server
				channel_ptr = server_ptr_->createNewChannel(channel_name);
				channel_ptr->setChannelCreationTimestamps();
				channel_ptr->addUser(client_ptr, true); // First user becomes the operator
				break;
			case '!': // Safe channels require special handling
				server_ptr_->sendResponse(client_fd, ERR_NOSUCHCHANNEL(server_ptr_->getServerHostname(), client_ptr->getNickname(), channel_name));
				continue;
			case '+': // No modes can be set
				channel_ptr = server_ptr_->createNewChannel(channel_name);
				channel_ptr->addUser(client_ptr, false);
				break;
			default:
				server_ptr_->sendResponse(client_fd, ERR_NOSUCHCHANNEL(server_ptr_->getServerHostname(), client_ptr->getNickname(), channel_name));
				continue;
			}
		}
		else
		{
			if (channel_ptr->isUserOnChannel(client_ptr->getNickname()))
			{
				std::cout << "user " << client_ptr->getNickname() << " tried to join channel " << channel_name << "but they are already there" << std::endl;
				continue;
			}
			if (channel_ptr->isFull())
			{
				server_ptr_->sendResponse(client_fd, ERR_CHANNELISFULL(server_ptr_->getServerHostname(), client_ptr->getNickname(), channel_name));
				continue;
			}
			if (channel_ptr->isInviteOnly() && !channel_ptr->isUserInvited(client_ptr->getNickname()))
			{
				server_ptr_->sendResponse(client_fd, ERR_INVITEONLYCHAN(client_ptr->getHostname(), client_ptr->getNickname(), channel_name));
				continue;
			}
			if (channel_ptr->isPasswordProtected())
			{
				std::string given_password = keys.size() > i ? keys[i] : "";
				if (!channel_ptr->isCorrectPassword(given_password))
				{
					server_ptr_->sendResponse(client_fd, ERR_BADCHANNELKEY(server_ptr_->getServerHostname(), client_ptr->getNickname(), channel_name));
					continue;
				}
			}
			//We should setChannelCreationTimestamps if the user is the first user in the channel
			if (!channel_ptr->isEmpty())
				channel_ptr->addUser(client_ptr, false);
			else
			{
				channel_ptr->setChannelCreationTimestamps();
				channel_ptr->addUser(client_ptr, true);
			}
		}
		client_ptr->joinChannel(channel_ptr);
		server_ptr_->sendResponse(client_fd, RPL_JOINMSG(client_ptr->getClientPrefix(), channel_name));
		sendNamReplyAfterJoin(channel_ptr, client_ptr->getNickname(), client_fd);
		std::time_t unix_timestamp = std::chrono::system_clock::to_time_t(channel_ptr->getChannelCreationTimestamps());
		std::string channel_creation_timestamp_string = std::to_string(unix_timestamp);
		server_ptr_->sendResponse(client_fd, RPL_CREATIONTIME(server_ptr_->getServerHostname(), client_ptr->getNickname(), channel_name, channel_creation_timestamp_string));
		channel_ptr->broadcastMessage(client_ptr, RPL_JOINMSG(client_ptr->getClientPrefix(), channel_ptr->getName()), server_ptr_);
		if (channel_ptr->hasTopic())
			channel_ptr->sendTopicToClient(client_ptr, server_ptr_);
	}
}

/**
 * @brief this function is for sending the names reply to user after joining a new channel
 * 		  the message lists current users of the channel and their operator status
 *
 * @param channel_ptr
 * @param nickname
 * @param client_fd
 */
void Command::sendNamReplyAfterJoin(std::shared_ptr<Channel> channel_ptr, std::string nickname, int client_fd)
{
	auto channel_users = channel_ptr->getUsers(); // get the user list
	std::string servername = server_ptr_->getServerHostname();
	std::string channel_name = channel_ptr->getName();
	std::string userlist = "";
	for (auto it = channel_users.begin(); it != channel_users.end(); it++) // stitching the user list together in this loop
	{
		userlist += " ";
		if (it->second == true)
			userlist += "@";
		userlist += it->first.lock()->getNickname();
	}
	server_ptr_->sendResponse(client_fd, RPL_NAMREPLY(servername, nickname, channel_name, userlist));
	server_ptr_->sendResponse(client_fd, RPL_ENDOFNAMES(servername, nickname, channel_name));
}
