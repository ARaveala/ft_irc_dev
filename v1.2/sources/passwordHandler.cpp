#include "Server.hpp"
#include "MessageBuilder.hpp"

//class MessageBuilder;
void Server::handlePassword(const std::shared_ptr<Client>& client, const int& client_fd, const std::string& password) {
	std::cout<<"Server::handlePassword::PASSWORD BEING BEING ACTUATED......... \n";
	if (password == get_password())
	{
		std::cout<<"Server::handlePassword::PASSWORD ACCEPTED......... \n";
		client->getMsg().queueMessage(":localhost NOTICE * :Password accepted. Continuing registration.\r\n");
		client->setPasswordValid();
		updateEpollEvents(client_fd, EPOLLOUT, true);
	}
	else {
		std::cout<<"Server::handlePassword::PASSWORD NOT ACCPTED......... \n";
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
        std::cout << "Server::tryRegisterClient::PASSWORD NOT ACCEPTED.........\n";
        client->getMsg().queueMessage(":localhost 464 * :Password incorrect\r\n");
        client->setQuit();
        updateEpollEvents(fd, EPOLLOUT, true);
        return;
    }
    // Registration successful
	// time stamp to stop issue when nick command applied too quickly before registartion has managed to finnish
	client->setRegisteredAt(std::chrono::steady_clock::now());
    client->setHasRegistered();
    broadcastMessage(MessageBuilder::generatewelcome(client->getNickname()), nullptr, nullptr, false , client);
}
