#include "Command.h"

void Command::handlePass(const Message &msg)
{
	std::shared_ptr<Client> client_ptr = msg.getClientPtr();
	if (!client_ptr)
	{
		std::cerr << ("null ptr in handlePass") << std::endl;
		return;
	}
	int client_fd = client_ptr->getFd();
	if (server_ptr_->getPassword().empty())
		return;
	if (client_ptr->getRegisterStatus() == true)
	{
		server_ptr_->sendResponse(client_fd, ERR_ALREADYREGISTERED(server_ptr_->getServerHostname(), client_ptr->getNickname()));
		return;
	}
	std::vector<std::string> parameters = msg.getParameters();
	if (parameters.empty() || parameters.front().find_first_not_of(" \t\v") == std::string::npos)
	{
		server_ptr_->sendResponse(client_fd, ERR_NEEDMOREPARAMS(std::string("*"), "PASS"));
		return;
	}
	else if (!client_ptr->getRegisterStatus())
	{
		if (parameters[0] == server_ptr_->getPassword())
			client_ptr->setHasCorrectPassword(true);
		else if (parameters[0] != server_ptr_->getPassword())
		{
			server_ptr_->sendResponse(client_fd, ERR_PASSWDMISMATCH(server_ptr_->getServerHostname(), client_ptr->getNickname()));
			server_ptr_->sendResponse(client_fd, "ERROR: Connection got rejected by the server\r\n");
			server_ptr_->closeDeletePollFd(client_fd);
			server_ptr_->deleteClient(client_fd);
		}
	}
}
