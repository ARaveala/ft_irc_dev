#include "Server.hpp"


int		Server::commandSelector(Msg msg, Client &client)
{
	if (msg.command == "CAP")
	{
		return (0);
	}
	if (msg.command == "PASS")
	{
		if (Server::passwordCommand(msg, client) != 0)
		{
			return (1);
		}
	}
	else if (msg.command == "NICK")
	{
		Server::nicknameCommand(msg, client);
	}
	else if (msg.command == "USER")
	{
		if (Server::userCommand(msg, client) != 0)
			return (1);
	}
	else if  (msg.command == "PING")
	{
		std::string response = "PONG " + msg.parameters[0] + "\r\n";
        send(client.getSocket(), response.c_str(), response.size(), 0);
		LOG_SERVER(response);
	}
	else if (client.getPasswordChecked() == false)
	{	
		return (1);
	}
	else if  (msg.command == "PRIVMSG")
	{
		privmsgCommand(msg, client);
	}
	else if  (msg.command == "KICK")
	{
		kickCommand(msg, client);
	}
	else if  (msg.command == "INVITE")
	{
		inviteCommand(msg, client);
	}
	else if (msg.command == "PART")
	{
		partCommand(msg, client);
	}
	else if  (msg.command == "TOPIC")
	{
		topicCommand(msg, client);
	}
	else if  (msg.command == "MODE")
	{
		modeCommand(msg, client);
	}	
	else if (msg.command == "JOIN")
	{
		joinCommand(msg, client);
	}
	else if (msg.command == "WHO")
	{
		whoCommand(msg, client);
	}
	else if (msg.command[0] == ':')
	{
		std::string response = "Please no Prefixes in Commands. Server No like.\r\n";
        send(client.getSocket(), response.c_str(), response.size(), 0);
		LOG_SERVER(response);
	}
	else
	{
		/*	Ignore Commands that are Unknown.	*/
	}
	return (0);
}

/*
	1. Parses Irssi protocol strings to Msg Object
	2. Runs command (in Msg) through commandSelector
	(Can handle multiple commands sent at once.)
*/
int    Server::makeSelectAndRunCommand(std::string messages, Client &client)
{
	std::vector<Msg>     msgs;

	this -> makeMessages(msgs, messages);

	for (size_t i = 0; i < msgs.size(); i++)
	{
		if (this->commandSelector(msgs[i], client) == 1)
		{
			return (1);								//Password issue occurred
		}
	}
	return (0);
}
