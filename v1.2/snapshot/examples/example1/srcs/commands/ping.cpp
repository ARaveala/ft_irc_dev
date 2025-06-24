#include "Server.hpp"

/*
	Keeps the connection to clients alive
*/
void Server::ping(std::string buf, int fd, int index)
{
	if (!_clients[index])
		return;
	std::string ip = buf.substr(5);
	sendResponse("PONG " + ip, fd);
}
