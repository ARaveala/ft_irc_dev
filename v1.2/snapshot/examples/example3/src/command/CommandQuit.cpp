#include "Command.h"

void Command::handleQuit(const Message &msg)
{
	std::shared_ptr<Client> client_ptr = msg.getClientPtr();
	if (!client_ptr)
	{
		std::cerr << ("null ptr in handleQuit") << std::endl;
		return;
	}
	int client_fd = client_ptr->getFd();
	std::string reason = msg.getTrailer().empty() ? "Leaving" : msg.getTrailer();
	server_ptr_->sendResponse(client_fd, "ERROR: Bye, see you soon!\r\n");	
	server_ptr_->disconnectAndDeleteClient(client_ptr, reason);
}