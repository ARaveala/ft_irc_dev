#include "../server/Server.h"
#include "Command.h"
#include "../debug/debug.h"

void Command::handleUser(const Message &msg)
{
	std::shared_ptr<Client> client_ptr = msg.getClientPtr();
	if (!client_ptr)
	{
		std::cerr << ("null ptr in handleUser") << std::endl;
		return;
	}
	std::vector<std::string> params = msg.getParameters();
	int client_fd = msg.getClientfd();
	if (!server_ptr_->hasClientSentPass(client_ptr))
		return;
	if (client_ptr->getRegisterStatus() == true)
		server_ptr_->sendResponse(client_fd, ERR_ALREADYREGISTERED(server_ptr_->getServerHostname(), client_ptr->getNickname()));
	else if (params.size() == 3 && !msg.getTrailer().empty())
	{
		client_ptr->setUsername(params[0]);
		client_ptr->setHostname(params[2]);
		client_ptr->setRealname(msg.getTrailer());
		if (!client_ptr->getNickname().empty())
		{
			client_ptr->registerClient();
			client_ptr->setClientPrefix();
			server_ptr_->welcomeAndMOTD(client_fd, server_ptr_->getServerHostname(), client_ptr->getNickname(), client_ptr->getClientPrefix());
		}
	}
	else
	{
		server_ptr_->sendResponse(client_fd, ERR_NEEDMOREPARAMS(client_ptr->getClientPrefix(), "USER"));
		return;
	}
}