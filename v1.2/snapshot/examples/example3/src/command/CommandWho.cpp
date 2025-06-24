#include "Command.h"

void Command::sendNamelist(std::shared_ptr<Channel> channel_ptr, std::string user_nickname, int client_fd)
{
	auto channel_users = channel_ptr->getUsers(); // get the user list
	std::string servername = server_ptr_->getServerHostname();
	std::string channel_name = channel_ptr->getName();
	std::string userlist = "";
	for (auto it = channel_users.begin(); it != channel_users.end(); it++) // stitching the user list together in this loop
	{
		auto it_lock_first = it->first.lock();
		std::string flag = "";
		if (it_lock_first->isAway() == true)
			flag += "G";
		else
			flag += "H";
		if (it_lock_first->getModeLocalOp() == true)
			flag += "*";
		server_ptr_->sendResponse(client_fd, RPL_WHOREPLY(servername, user_nickname, channel_name, it_lock_first->getUsername(), it_lock_first->getHostname(), it_lock_first->getNickname(), flag, it_lock_first->getRealname()));
	}
	server_ptr_->sendResponse(client_fd, RPL_ENDOFWHO(servername, user_nickname, channel_name));
}

void Command::handleWho(const Message &msg)
{
	std::string command = msg.getCommand();
	std::vector <std::string> params = msg.getParameters();
	int client_fd = msg.getClientfd();
	auto client_ptr = msg.getClientPtr();
	std::shared_ptr <Channel> channel_ptr = server_ptr_->findChannel(params[0]);
	if (channel_ptr && channel_ptr->isUserOnChannel(client_ptr->getNickname()))
		sendNamelist(channel_ptr, client_ptr->getNickname(), client_fd);
	else if (channel_ptr && !channel_ptr->isUserOnChannel(client_ptr->getNickname()))
		server_ptr_->sendResponse(client_fd, ERR_NOTONCHANNEL(server_ptr_->getServerHostname(), client_ptr->getNickname(), params[0]));
}