#include "Server.hpp"

void Server::clearClient(int clientFd, int index)
{
	if (index < 0 || index >= (int)_clients.size()) return;

	_clients[index]->setIsAuthenticated(false);
	_clients[index]->setIsRegistered(false);

	for (size_t i = 0; i < _channels.size(); ++i)
	{
		if (isInChannel(_clients[index], _channels[i]))
		{
			if (_channels[i]->isAdmin(_clients[index]) != -1)
				_channels[i]->removeAdmin(_clients[index]);
			if (_channels[i]->isClient(_clients[index]) != -1)
				_channels[i]->removeClient(_clients[index]);
			if (_channels[i]->isInvited(_clients[index]) != -1)
				_channels[i]->removeInvited(_clients[index]);
			if (_channels[i]->getClients().empty())
			{
				delete _channels[i];
				_channels.erase(_channels.begin() + i);
			}
			break;
		}
	}

	_clients.erase(_clients.begin() + index);

	for (size_t i = 0; i < _poll.getSize(); ++i)
	{
		if (_poll.getFd(i).fd == clientFd)
		{
			_poll.removeFd(i);
			break;
		}
	}

	close(clientFd);
}

bool Server::validateClientRegistration(int fd, int index)
{
	if (!_clients[index])
		return false;
	if (!_clients[index]->getIsRegistered())
	{
		sendError("451 : You have not registered", fd);
		return false;
	}
	return true;
}