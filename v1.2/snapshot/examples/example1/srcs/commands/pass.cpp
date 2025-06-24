#include "Server.hpp"

/*
	Authenticates user and makes sure the password matches
*/
void Server::pass(std::string buf, int fd, int index)
{
	if (!_clients[index])
		return;

	std::istringstream iss(buf);
	std::string command, password;
	iss >> command >> password;

	if (password != _password)
	{
		sendError("464 : Password incorrect", fd);
		return ;
	}
	_clients[index]->setIsAuthenticated(true);
}