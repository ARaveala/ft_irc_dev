#include "Command.h"

void Command::handleKill(const Message &msg)
{
	std::shared_ptr<Client> client_ptr = msg.getClientPtr();
	if (!client_ptr)
	{
		std::cerr << ("null ptr in handleKill") << std::endl;
		return;
	}
	std::string sender_nick = client_ptr->getNickname();
	int client_fd = msg.getClientfd();
	std::vector<std::string> params = msg.getParameters();
//	FOR TESTING PURPOSES
	// if (params[0] == "server")
	// {
	// 	server_ptr_->signalHandler(SIGINT);
	// 	return;
	// }
	std::string target_nick, comment, command;
	command = msg.getCommand();
	comment = msg.getTrailer();
	if (comment.empty())
	{
		server_ptr_->sendResponse(client_fd, ERR_NEEDMOREPARAMS(client_ptr->getClientPrefix(), command));
		return;
	}
	if (params.size())
		target_nick = params[0];
	if (client_ptr->getModeLocalOp() == true)
	{
		std::shared_ptr <Client> target_client = server_ptr_->findClientUsingNickname(target_nick);
		if (target_client)
		{
			std::string kill_message = RPL_KILLED(server_ptr_->getServerHostname(), sender_nick, comment);
			server_ptr_->sendResponse(target_client->getFd(), RPL_KILLMSG(client_ptr->getClientPrefix(), target_client->getNickname(), kill_message));
			Message quit_msg("QUIT :" + kill_message, server_ptr_, target_client->getFd());
			target_client->processCommand(quit_msg, server_ptr_);
			return;
		}
		else
		{
			server_ptr_->sendResponse(client_fd, ERR_NOSUCHNICK(server_ptr_->getServerHostname(), sender_nick, target_nick));
			return;
		}
	}
	else
		server_ptr_->sendResponse(client_fd, ERR_NOPRIVILEGES(sender_nick));
}