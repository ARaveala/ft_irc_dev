#include "Server.hpp"
#include "MessageBuilder.hpp"

//class MessageBuilder;
void Server::handlePassword(const std::shared_ptr<Client>& client, const int& client_fd, const std::string& password) {
	std::cout<<"DEBUG::PASSWORD BEING BEING ACTUATED......... \n";
	if (password == get_password())
	{
		std::cout<<"DEBUG::PASSWORD ACCEPTED......... \n";
		client->getMsg().queueMessage(":localhost NOTICE * :Password accepted. Continuing registration.\r\n");
		client->setPasswordValid();
		updateEpollEvents(client_fd, EPOLLOUT, true);
	}
	else {
		std::cout<<"DEBUG::PASSWORD NOT ACCPTED......... \n";
		client->getMsg().queueMessage(":localhost 464 * :Password incorrect\r\n");
		client->setQuit();
		updateEpollEvents(client_fd, EPOLLOUT, true);
	}
	tryRegisterClient(client);
}

void Server::tryRegisterClient(const std::shared_ptr<Client>& client) {
    int fd = client->getFd();
    if (client->getHasRegistered())
        return;
    if (!client->getHasSentNick() || !client->getHasSentUser())
        return;
    if (!client->getPasswordValid()) {
        std::cout << "DEBUG::PASSWORD NOT ACCEPTED.........\n";
        client->getMsg().queueMessage(":localhost 464 * :Password incorrect\r\n");
        client->setQuit();
        updateEpollEvents(fd, EPOLLOUT, true);
        return;
    }
    // Registration successful
	//tryRegisterClient(client);
	client->setRegisteredAt(std::chrono::steady_clock::now());
    client->setHasRegistered();
    broadcastMessage(MessageBuilder::generatewelcome(client->getNickname()), nullptr, nullptr, false , client);
    updateEpollEvents(fd, EPOLLOUT, true);
}


/**if (command == "NICK") {
    _server->handleNickCommand(...);
    _server->tryRegisterClient(client);
}

if (command == "USER") {
    // set username, realname, etc.
    _server->tryRegisterClient(client);
}

if (command == "PASS") {
    // validate password, setPasswordValid(true) if correct
    _server->tryRegisterClient(client);
}
 */