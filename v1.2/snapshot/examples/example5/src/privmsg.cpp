#include "Server.hpp"

/*
	Sends message to everyone in Channel (except sender).
*/
void	Server::userMessageToChannel(Channel channel, int sender_socket, std::string message)
{
	std::vector<User>	users;
	int					socket;

	users = channel.getChannelUsers();
	// printChannels();
	for (size_t i = 0; i < users.size(); i++)
	{
		socket = getClientSocket(users[i].nickname);
		if (socket != -2 && socket != sender_socket)
		{
			send(socket, message.c_str(), message.size(), 0);
			LOG_SERVER(message);
		}
	}
}

void		Server::channelMessage(Msg msg, Client &client)
{
	int i = getChannelIndex(msg.parameters[0], _channel_names);
	if (i != -1)
	{
		std::string message = ":" + client.getPrefix() + " " + msg.command + " " + msg.parameters[0] + " "  + ":" +  msg.trailing_msg + "\r\n";
		userMessageToChannel(_channel_names[i], client.getSocket(), message);			
	}
	else
	{
		std::cout << "Channel to send message to not found" << std::endl;
	}
}

/*
	Sends direct message to specified user.
*/
void		Server::directMessage(Msg msg, Client &client)
{
	std::string other_client_user_name = msg.parameters[0];

	int other_client_socket = getClientSocket(other_client_user_name);
	if (other_client_socket == -2)
	{
		std::string message = ":ircserver 401 " + client.getNickname() + " " + msg.parameters[0] + " :No such nick/channel\r\n";
		send(client.getSocket(), message.c_str(), message.size(), 0);
		LOG_SERVER(message);
	}
	else		//other client exists
	{
		std::string message = ":" + client.getPrefix() + " " + msg.command + " " + msg.parameters[0] + " "  + ":" +  msg.trailing_msg + "\r\n";
		send(other_client_socket, message.c_str(), message.size(), 0);
		LOG_SERVER(message);
	}
}

/*
	Sends message to either Channel or as a Direct Message to user.
*/
void		Server::privmsgCommand(Msg msg, Client &client)
{
	if (msg.parameters[0][0] == '#')	//if Channel
	{
		std::string channel_name = msg.parameters[0];

		if (userExists(client.getNickname(), channel_name))		
			channelMessage(msg, client);
		else
			std::cout << "User is not on channel" << std::endl;
	}
	else
	{
		directMessage(msg, client);
	}
	return ;
}

