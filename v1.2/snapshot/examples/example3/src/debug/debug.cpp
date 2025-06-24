#include "debug.h"
#include "../client/Client.h"

#ifdef DEBUG_MODE
#define DEBUG(message, flag) \
{ \
        if (flag == FAILED) \
            std::cerr << RED << message << " failed." << RESET << std::endl; \
        else if (flag == SUCCESS) \
		{\
            std::cout << GREEN << message << " successfully." << RESET << std::endl; \
			sleep(1);\
		}\
}
#else
#define DEBUG(message, flag) ((void)0)
#endif


void	debug(const std::string &message, int flag)
{
	#ifdef DEBUG_MODE
		DEBUG(message, flag);
    #else
	{
		if (flag == THROW_ERR)
			throw(std::runtime_error(message));
        else if (flag == FAILED)
            std::cerr << RED << message << " failed." << RESET << std::endl;
	}
    #endif
}

void debugWhois(std::shared_ptr<Client> client)
{
	std::cout << BLUE << "debug WHOIS\n";
	std::cout << "Client fd: " << client->getFd() << "\n";
	std::cout << "Nickname: " << client->getNickname() << "\n";
	std::cout << "Username: " << client->getUsername() << "\n";
	std::cout << "Real name: " << client->getRealname() << "\n";
	std::cout << "Host name: " << client->getHostname() << "\n";
	std::cout << "IP: " << client->getIpAddress() << "\n";
	//std::cout << "User mode: " << client->getUserMode() << "\n";
	std::cout << "Hasregistered: " << client->getRegisterStatus() << "\n";
	std::cout << "Has sent password: " << client->getHasCorrectPassword() << "\n";
	std::cout << "Prefix: " << client->getClientPrefix() << RESET << std::endl;
}