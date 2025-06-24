#include "Server.hpp"

void Server::sendResponse(std::string msg, int fd)
{
	std::string response = msg + "\r\n";
	send(fd, response.c_str(), response.length(), 0);
}

void Server::sendError(std::string msg, int fd)
{
	std::string response = ":localhost " + msg + "\r\n";
	if (send(fd, response.c_str(), response.length(), 0) == -1)
		std::cerr << "Error sending message: " << strerror(errno) << std::endl;
}

/*
	Returns the nickname of the client that has given clientsocket,
	otherwise empty string.
*/
std::string Server::getNickname(int fd)
{
	for (const std::shared_ptr<Client>& client : _clients)
	{
		if (client->getSocket() == fd)
			return client->getNickname();
	}
	return "";
}

std::shared_ptr<Client> Server::getClient(const std::string& nick)
{
	for (const std::shared_ptr<Client>& client : _clients)
	{
		if (client->getNickname() == nick)
			return client;
	}
	return nullptr;
}

/*
	Loops through all _clients in Server class and compares the
	username given as an argument to them.

	Returns the index of the client with the same username,
	otherwise -1.
*/
int Server::searchByUsername(std::string user)
{
	for (size_t i = 0; i < _clients.size(); ++i)
	{
		if (_clients[i]->getUsername() == user)
			return i;
	}
	return -1;
}

/*
	Loops through all _clients in Server class and compares the
	nick given as an argument to them.

	Returns the index of the client with the same nickname,
	otherwise -1.
*/
int Server::searchByNickname(std::string nick)
{
	for (size_t i = 0; i < _clients.size(); ++i)
	{
		if (_clients[i]->getNickname() == nick)
			return i;
	}
	return -1;
}

/*
	Loops through all clients in the channels
	and compares the client given as an argument to them.

	Returns true if found, false if not.
*/
bool Server::isInChannel(const std::shared_ptr<Client>& client, Channel *channel)
{
	for (const std::shared_ptr<Client>& tmp_client : channel->getClients())
	{
		if (tmp_client == client)
			return true;
	}
	return false;
}

/*
	Loops through channels and fetchs the index of the channel
	with the same name as given argument.

	Returns the index of the channel with the same name,
	otherwise -1.
*/
int Server::getChannelIndex(std::string name)
{
	for (size_t i = 0; i < _channels.size(); ++i)
	{
		if (_channels[i]->getName() == name)
			return i;
	}
	return -1;
}