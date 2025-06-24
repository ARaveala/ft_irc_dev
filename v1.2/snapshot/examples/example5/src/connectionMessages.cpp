#include "Server.hpp"

/*
	Check if password correct.
	Return
	1 - if incorrect
	0 - if correct
*/
int		Server::passwordCommand(Msg msg, Client &client)
{
	if (msg.parameters[0] != client.getPassword())
	{
		std::string message_464 = ":ircserver 464 " + client.getPrefix() + " :Password incorrect\r\n";
		send(client.getSocket(), message_464.c_str(), message_464.size(), 0);
		LOG_SERVER(message_464);
		return (1);
	}
	client.setPasswordChecked(true);
	return (0);
}

/*
	Check if nickname is used by another client.
	Return
	1 - if taken
	0 - if nickname available	
*/
int		Server::nickClash(const std::string& nickname, int socket)
{
    for (auto &client : _clients) 
    {
		if (client.getSocket() != socket)
		{
        	if (client.getNickname() == nickname)
            	return (1); // Nickname is taken
		}
    }
    return (0); // Nickname is available
}

/*
	Changes nickname of user.
*/
int		Server::nicknameCommand(Msg msg, Client &client)
{
	if (!client.getWelcomeSent() && client.getNickname().empty())				//a. If New User
		client.setNickname(msg.parameters[0]);
	else if (msg.parameters[0].empty())											//b. If No nickname specified in command
	{
		std::string message_431 = ":ircserver 431 " + client.getNickname() + " :No nickname given\r\n";
		send(client.getSocket(), message_431.c_str(), message_431.size(), 0);
		LOG_SERVER(message_431);
	}
	else if (this->nickClash(msg.parameters[0], client.getSocket()))			//c. If nickname is already in use by other client
	{
		std::string message_433 = ":ircserver 433 " + msg.parameters[0] + " :" + msg.parameters[0] + "\r\n";
		send(client.getSocket(), message_433.c_str(), message_433.size(), 0);
		LOG_SERVER(message_433);
	}
	else																		//d. Change nickname
	{
		std::string old_nick = client.getNickname();
		client.setNickname(msg.parameters[0]);														//i. set new nick

		std::string new_nick = client.getNickname();
		std::string old_prefix = old_nick + "!" + client.getUsername() + "@" + client.getHost();
		std::string new_prefix = new_nick + "!" + client.getUsername() + "@" + client.getHost();
		client.setPrefix(new_prefix);																//ii. set new prefix
		std::string nick_message = ":" + old_prefix + " NICK :" + new_nick + "\r\n";				//iii. Inform user of nick change
		send(client.getSocket(), nick_message.c_str(), nick_message.size(), 0);
		LOG_SERVER(nick_message);
	
		for (auto &channel : _channel_names) 											//iv. a) [in all channels] - change channel users name to new nick 
		{
			bool userFound = false;
			for (auto &user : channel.getChannelUsers())
			{
				if (user.nickname == old_nick)
				{
					user.nickname = new_nick;											//iv. b) Change to new nick
					userFound = true;
				}
			}
			if (userFound)
			{
				std::string message = ":" + old_prefix + " NICK :" + new_nick + "\r\n";
				broadcastToChannel(channel, message, client, 1);						//iv. c) Broadcast new nick
				break;
			}
		}
	}
	return (0);
}

/*
	1. Checks if user is authenticated (i.e. password check).

	2. Sets
	-Username
	-Hostname
	-Host
	-Real name
	(- also client prefix)
	
	3. Sends welcome messages.
*/
int		Server::userCommand(Msg msg, Client &client)
{
	if (client.getPasswordChecked())			//Password is already set
	{
		client.setUsername(msg.parameters[0]);
		client.setHostname(msg.parameters[1]);
		client.setHost(msg.parameters[2]);
		client.setRealname(msg.trailing_msg);
		std::string message_001 = ":ircserver 001 " + client.getNickname() + " :Welcome to the IRC network " + client.getNickname() + "!" + client.getUsername() + "@" + client.getHost() + "\r\n";
		send(client.getSocket(), message_001.c_str(), message_001.size(), 0);
		LOG_SERVER(message_001);
		std::string message_002 = ":ircserver 002 " + client.getNickname() + " :Your host is ircserver, running version 1.0\r\n";
		send(client.getSocket(), message_002.c_str(), message_002.size(), 0);
		LOG_SERVER(message_002);
		std::string message_003 = ":ircserver 003 " + client.getNickname() + " :This server was created " + this->getStartTimeStr() + "\r\n";
		send(client.getSocket(), message_003.c_str(), message_003.size(), 0);
		LOG_SERVER(message_003);
		std::string message_004 = ":ircserver 004 " + client.getNickname() + " ircserver 1.0 ro itkol\r\n";
		send(client.getSocket(), message_004.c_str(), message_004.size(), 0);
		LOG_SERVER(message_004);
		std::string message_005 = ":ircserver 005 " + client.getNickname() + " CHANMODES=i,t,k,o,l :are supported by this server\r\n";
		send(client.getSocket(), message_005.c_str(), message_005.size(), 0);
		LOG_SERVER(message_005);
		if (this->nickClash(client.getNickname(), client.getSocket()))
		{
			std::string nick_message1 = ":" + client.getNickname() + " NICK ";
			client.setNickname(client.getNickname() + "_" + std::to_string(client.getSocket()));
			std::string nick_message2 = client.getNickname() + "\r\n";
			std::string nick_message = nick_message1 + nick_message2;
			send(client.getSocket(), nick_message.c_str(), nick_message.size(), 0);
			LOG_SERVER(nick_message);
		}

		std::string setPrefix = client.getNickname() + "!" + client.getUsername() + "@" + client.getHost(); 		// set client prefix
		client.setPrefix(setPrefix);
		client.setWelcomeSent(true);
		return (0);
	}
	else										//Password is NOT set
	{
		std::string message_464 = ":ircserver 464 * :Password needed\r\n";
		send(client.getSocket(), message_464.c_str(), message_464.size(), 0);
		LOG_SERVER(message_464);
		return (1);
	}
}
