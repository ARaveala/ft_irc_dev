#include "Server.hpp"

bool servRunning = false;

Server::Server(std::string password, int port) : _password(password), _port(port)
{}

Server::~Server()
{
	closeFDs();
	std::cout << "Closing Server..." << std::endl;
	std::cout << "Goodbye" << std::endl; 
}

/*
	Closes file descriptors (Fds) found in pollfds.
*/
void	Server::closeFDs()
{
	std::cout << "Closing File Descriptors" << std::endl;
	for (pollfd &pollfd : _pollfds) 
	{
		close(pollfd.fd);
	}
}

/*
	Uses the client's nickname to find and return its socket.
*/
int		Server::getClientSocket(std::string nickname)
{
	int socket;

	for (size_t i = 0; i < this->_clients.size(); i++)
	{
		if (nickname == _clients[i].getNickname())
		{
			socket = _clients[i].getSocket();
			return (socket);
		}
	}
	return (-2);//Return -2 to differentiate from -1 (failed socket)
}
