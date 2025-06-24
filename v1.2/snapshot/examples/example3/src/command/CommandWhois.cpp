#include "Command.h"

void Command::handleWhois(const Message &msg) {
	std::shared_ptr<Client> client_ptr = msg.getClientPtr();
	if (!client_ptr)
	{
		std::cerr << ("null ptr in handleWhois") << std::endl;
		return;
	}
    int client_fd = client_ptr->getFd();
    std::vector<std::string> parameters = msg.getParameters();
    
    if (parameters.empty()) {
        server_ptr_->sendResponse(client_fd, ERR_NONICKNAMEGIVEN(client_ptr->getClientPrefix()));
        return;
    }

    std::vector<std::string> masks = split(parameters.front(), ',');

    for (const auto& mask : masks)
	{
        bool found = false;
        std::vector<std::shared_ptr<Client>> matched_clients = server_ptr_->findClientsByMask(mask);
        for (const auto& whois_lock_client_ptr : matched_clients)
		{
            found = true;
            server_ptr_->sendResponse(client_fd, RPL_WHOISUSER(server_ptr_->getServerHostname(), client_ptr->getNickname(),
                                                    whois_lock_client_ptr->getNickname(), whois_lock_client_ptr->getUsername(),
                                                    whois_lock_client_ptr->getHostname(), whois_lock_client_ptr->getRealname()));
        }
        if (!found)
            server_ptr_->sendResponse(client_fd, ERR_NOSUCHNICK(server_ptr_->getServerHostname(), client_ptr->getNickname(), mask));
    }

    server_ptr_->sendResponse(client_fd, RPL_ENDOFWHOIS(server_ptr_->getServerHostname(), client_ptr->getNickname(), parameters.front()));
}
