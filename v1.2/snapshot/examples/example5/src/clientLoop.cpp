#include "Server.hpp"


void Server::removeUserFromAllChannels(int i)
{
	for (auto &client : _clients)				 				//remove user from all channels
	{
		if (client.getSocket() == _pollfds[i].fd) 
		{
			std::string userPrefix = client.getPrefix();
			for (auto &channel : _channel_names)
			{
				for (auto &user : channel.getChannelUsers())
				{
					if (client.getNickname() == user.nickname)
					{
						std::string quitMessage = ":" + userPrefix + " QUIT " + ":Client has quit\r\n";
						int j = getChannelIndex(channel.getChannelName(), _channel_names);
						broadcastToChannel(this->_channel_names[j], quitMessage, client, 1);

						std::string message = "REMOVE " + user.nickname + " from " + channel.getChannelName();
						removeUser(user.nickname, channel.getChannelName(), message, 2);
						client.leaveChannel(channel.getChannelName());
					}
				}
			}
		}
	}
}

void		Server::printErrorBytesRead(int	bytesRead, int i)
{
	if (bytesRead == 0)
	{
		std::cout << "Client disconnected, socket " << _pollfds[i].fd << std::endl;
	}
	else 
		std::cerr << "Error reading from socket " << _pollfds[i].fd << " " << strerror(errno) << std::endl;
}

void	Server::disconnectClient(size_t& i, std::map<int, std::string> &clientBuffers)
{
	close(_pollfds[i].fd);
	_pollfds.erase(_pollfds.begin() + i);
	_clients.erase(_clients.begin() + (i - 1));
	clientBuffers.erase(_pollfds[i].fd);
	i--;
}

void	Server::removeClient(int bytesRead, size_t i, std::map<int, std::string> &clientBuffers)
{
	printErrorBytesRead(bytesRead, i);
	removeUserFromAllChannels(i);
	disconnectClient(i, clientBuffers);
}

/*
	1. Extracts single command (i.e. message) from clientBuffer
	2. Associates client with message (i.e. client.getSocket())
	3. Sends command to be ultimately run in makeSelectAndRunCommand()
*/
void	Server::processClientBuffer(size_t i, std::map<int, std::string> &clientBuffers)
{
	std::string &clientBuffer = clientBuffers[_pollfds[i].fd];	// Check for complete messages (assuming '\n' as a delimiter)

	size_t		pos;
	bool		clientDisconnected = false;
	
	while ((pos = clientBuffer.find('\n')) != std::string::npos)
	{
		std::string message = clientBuffer.substr(0, pos);				// Extract the complete message
		clientBuffer.erase(0, pos + 1);									// Remove the processed message
		LOG_CLIENT(message);

		for (auto &client : _clients)
		{
			if (client.getSocket() == _pollfds[i].fd) 
			{
				if (this->makeSelectAndRunCommand(message.c_str(), client) == 1)
				{
					std::cout << "Client disconnected, socket " << _pollfds[i].fd << std::endl;
					disconnectClient(i, clientBuffers);
					clientDisconnected = true;
					break ;
				}
			}
		}
		if (clientDisconnected)
			break ;
	}
}

/*
	Checks for
	i. Event in Client Socket
	ii. Receives string (containing command) from Client
	iii. Parses string (into Msg Object)
	iv. Runs Command
*/
void	Server::checkClientSockets(std::map<int, std::string> &clientBuffers)
{
	for (size_t i = 1; i < _pollfds.size(); i++)		//Loop through all Client sockets
	{
		if (_pollfds[i].revents & POLLIN)				// The client has sent data to the server
		{
			char buffer[1024] = {0};
			int bytesRead = recv(_pollfds[i].fd, buffer, sizeof(buffer), 0);
			if (bytesRead <= 0) 
			{
				removeClient(bytesRead, i, clientBuffers);
				continue ;
			}
			clientBuffers[_pollfds[i].fd] += std::string(buffer); 	//Append received data to the client's buffer

			processClientBuffer(i, clientBuffers);
		}
	}
}
