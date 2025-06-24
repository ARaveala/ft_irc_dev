
#include "Server.hpp"

void	Server::whoCommand(Msg msg, Client &client)
{
	int i = getChannelIndex(msg.parameters[0], _channel_names);
	const std::vector<User>& channel_users = _channel_names[i].getChannelUsers();

	for (size_t i = 0; i < channel_users.size(); ++i) 
	{
		for (auto it : _clients)
		{
			if (it.getNickname() == channel_users[i].nickname)
			{
				std::string message_352 = ":ircserver 352 " + client.getNickname() + " " + msg.parameters[0] + " " + it.getUsername() + " " + it.getHostname() + " ircserver " + it.getNickname() + " :" + it.getRealname() + "\r\n";
				send(client.getSocket(), message_352.c_str(), message_352.size(), 0);
				LOG_SERVER(message_352);
			}
		}
	}
	std::string message315 = ":ircserver 315 " + client.getNickname() + " " + msg.parameters[0] + " :End of /WHO list\r\n";
	send(client.getSocket(), message315.c_str(), message315.size(), 0);
	LOG_SERVER(message315);
	}

