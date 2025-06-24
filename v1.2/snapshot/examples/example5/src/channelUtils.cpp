#include "Server.hpp"

/*
	Debug method
*/
void	Server::printChannelUsers(Channel channel)
{
	for (size_t i = 0; i < channel.getChannelUsers().size(); i++)
	{
		std::cout << "User[" << i << "]: " <<  channel.getChannelUserStruct(i).nickname  << " (op = " <<  channel.getChannelUserStruct(i).operator_permissions << ")" << std::endl;
	}	
}

/*
	Debug method
*/
void		Server::printChannels()
{
	for (size_t i = 0; i < this->_channel_names.size(); i++)
	{
		std::cout << "Channel[" << i << "]: "<< _channel_names[i].getChannelName() << std::endl;
		printChannelUsers(_channel_names[i]);
	}
}

/*
	Checks if a user exists in a channel.
	Returns
		1, if exists.
		0, if doesn't.
*/
int		Server::userExists(std::string user, std::string channel)
{
	int i = getChannelIndex(channel, _channel_names);
	for (auto &userToCheck : _channel_names[i].getChannelUsers())
	{
		if (userToCheck.nickname == user)
			return (1);
	}
	return (0);
}

/*
	Checks if channel exists,
	Returns
	1 - if exists
	0 - if it doesn't exist
*/
int		Server::channelExists(std::string channel)
{
	for (auto &it : _channel_names)
	{
		if (it.getChannelName() == channel)
			return (1);
	}
	return (0);
}

/*
	Returns index of channel in vector.
	- Returns 0 or greater, if channel exists.
	- Returns -1, if channel doesn't exist. 
*/
int		getChannelIndex(std::string channel_name, std::vector<Channel> channel_names)
{
	size_t i;

	for (i = 0; i < channel_names.size(); i++)
	{
		if (channel_name == channel_names[i].getChannelName())
		{
			return (i);
		}
	}
	return (-1);
}

/*
	Sends a message to everyone in a specific channel
*/
void	Server::broadcastToChannel(Channel &channel, std::string message, Client &client, int check)
{
	std::vector<User> users;
	int socket;

	users = channel.getChannelUsers();

	for (size_t i = 0; i < users.size(); i++)
	{
			socket = getClientSocket(users[i].nickname);
			if (socket != -2)
			{
				if (check == 1)
				{
					if (client.getSocket() != socket)
						send(socket, message.c_str(), message.size(), 0);
				}
				else
					send(socket, message.c_str(), message.size(), 0);
				LOG_SERVER(message);
			}
	}
}
	
