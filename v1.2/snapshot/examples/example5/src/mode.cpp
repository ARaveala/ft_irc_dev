#include "Server.hpp"

/*
	Checks if client is an operator in channel.
	Returns
	1 - if operator
	0 - if not an operator
*/
int 	Server::clientStatus(Msg msg, Client &client)
{
	for (auto &channel : _channel_names)
	{
		if (channel.getChannelName() == msg.parameters[0])
		{
			for (auto &users : channel.getChannelUsers())
			{
				if (users.nickname == client.getNickname())
				{
					if (users.operator_permissions)
						return (1); // Operator status
				}
			}
		}
	}
	return (0); // no Operator status
}

Channel* Server::getChannel(std::string channelName)
{
	for (auto &channel : _channel_names)
	{
		if (channel.getChannelName() == channelName)
			return &channel;
	}
	return nullptr;
}

void	Server::userLimitMode(Msg msg, Client &client, Channel *tarChannel)
{
		std::string 	user_limit_string;
		std::string 	limitMsg;

		if (msg.parameters[1] == "-l")
		{
			tarChannel->setUserLimit(-1);
			limitMsg = ":" + client.getPrefix() + " MODE " + msg.parameters[0] + " " + msg.parameters[1] + "\r\n";
		}
		else
		{
			if (msg.parameters[1] == "+l")		// No Limit given
			{
				return ;
			}
			else								// Limit given
			{
				std::string user_limit_string;
				user_limit_string = msg.parameters[1].substr(2);	//Note: substring gets rid of "+l" and allows us to get limit number
				int userLimit;	
				try
				{
					userLimit = std::stoi(user_limit_string);
				}
				catch(const std::exception& e)	//Limit is Not an integer (f.ex. a letter)
				{
					return ;
				}
				if (userLimit <= 0)			//If Limit is zero or below
				{
					return ;
				}

				tarChannel -> setUserLimit(userLimit);			//Set new limit

				msg.parameters[1] = "+l";

				limitMsg = ":" + client.getPrefix() + " MODE " + msg.parameters[0] + " " + msg.parameters[1] + " " + user_limit_string + "\r\n";
				LOG_SERVER(limitMsg);
			}
		}

		broadcastToChannel(*tarChannel, limitMsg, client, 0);
}

int		Server::channelChecks(Msg msg, Client &client)
{
	if (msg.parameters.size() == 0)			//Not enough Parameters
	{
		return (1);
	}
	if (msg.parameters[0][0] != '#')
	{
		/*
			Check if not a Channel
				-> ignore users (f.ex. MODE userA +i)
		*/
		return (1);
	}
	if (msg.parameters.size() == 1)				//Check special command: MODE #Channel
	{
		std::string messages = ":ircserver 324 " + client.getNickname() +  " " + msg.parameters[0] + " +n\r\n";// +n = User can send message to channel, only if they are in the channel.
		send(client.getSocket(), messages.c_str(), messages.size(), 0);
		LOG_SERVER(messages);
		return (1);
	}
	if (channelExists(msg.parameters[0]) == 0)	//Channel doesn't exist
	{
		std::string errMsg = ":ircserver 482 " + client.getNickname() + " " + msg.parameters[0] + " :Channel doesn't exist\r\n";
		send(client.getSocket(), errMsg.c_str(), errMsg.size(), 0);
		LOG_SERVER(errMsg);
		return (1);

	}
	if (clientStatus(msg, client) == 0)			//Check if not a channel operator
	{
		if (msg.parameters[1] != "b")
		{
			std::string errMsg = ":ircserver 482 " + client.getNickname() + " " + msg.parameters[0] + " :You're not a channel operator\r\n";
			send(client.getSocket(), errMsg.c_str(), errMsg.size(), 0);
			LOG_SERVER(errMsg);
			return (1);
		}
	}
	return (0);
}

void	Server::keyMode(Msg msg, Client &client, Channel* tarChannel)
{
	if (msg.parameters[1] == "+k")
	{
		if (msg.parameters.size() == 2) 	//No key given
		{
			return ;
		}
		else if(msg.parameters[2] == "")	//Empty string for a key - not allowed
		{
			std::string errMsg = ":ircserver 525" + client.getNickname() + " " + msg.parameters[0] + " :Key is not well-formed\r\n";
			send(client.getSocket(), errMsg.c_str(), errMsg.size(), 0);
			LOG_SERVER(errMsg);
			return ;
		}
		else if (msg.parameters[2] != "")
		{
			tarChannel->setChannelKey(msg.parameters[2]);
			tarChannel->setKeyRequired(true);
			std::string keyMsg = ":" + client.getPrefix() + " MODE " + msg.parameters[0] + " +k " + msg.parameters[2] + "\r\n";
			broadcastToChannel(*tarChannel, keyMsg, client, 0);
			return ;
		}
	}
	else if (msg.parameters[1] == "-k")
	{
		tarChannel->setChannelKey("");
		tarChannel->setKeyRequired(false);
		std::string keyMsg = ":" + client.getPrefix() + " MODE " + msg.parameters[0] + " -k *" + "\r\n";
		broadcastToChannel(*tarChannel, keyMsg, client, 0);
		return ;
	}
}

void	Server::topicMode(Msg msg, Client &client, Channel* tarChannel)
{
	bool	boolean = (msg.parameters[1] == "+t") ? true : false;
	tarChannel -> setTopicRequiresOperator(boolean);
	std::string topicMsg = ":" + client.getPrefix() + " MODE " + msg.parameters[0] + " " + msg.parameters[1] + "\r\n";
	broadcastToChannel(*tarChannel, topicMsg, client, 0);
}

void	Server::inviteMode(Msg msg, Client &client, Channel* tarChannel)
{
	bool	boolean = (msg.parameters[1] == "+i") ? true : false;
	tarChannel -> setInviteOnly(boolean);
	std::string inviteMsg = ":" + client.getPrefix() + " MODE " + msg.parameters[0] + " " + msg.parameters[1] + "\r\n";
	broadcastToChannel(*tarChannel, inviteMsg, client, 0);
}

void	Server::operatorMode(Msg msg, Client &client, Channel* tarChannel)
{
	if (msg.parameters[2].empty())
	{
		std::string errMsg_461 = ":ircserver 461 " + client.getNickname() + " MODE :Not enough parameters\r\n";
		send(client.getSocket(), errMsg_461.c_str(), errMsg_461.size(), 0);
		LOG_SERVER(errMsg_461);
		return ;
	}
	bool nickExists = false;
	for (size_t i = 0; i < tarChannel->getChannelUsers().size(); i++)
	{
		if (tarChannel->getChannelUserStruct(i).nickname == msg.parameters[2])
		{
			nickExists = true;
			tarChannel->getChannelUserStruct(i).operator_permissions = (msg.parameters[1] == "+o") ? true : false;
			std::string chMsg = ":" + client.getPrefix() + " MODE " + msg.parameters[0] + " " + msg.parameters[1] + " " + msg.parameters[2] + "\r\n";
			broadcastToChannel(*tarChannel, chMsg, client, 0);
			break;
		}
	}
	if (nickExists == false)
	{
		std::string errMsg_441 = ":ircserver 441 " + client.getNickname() + " " + msg.parameters[2] + " " + msg.parameters[0] + " :They aren't on that channel\r\n";
		send(client.getSocket(), errMsg_441.c_str(), errMsg_441.size(), 0);
		LOG_SERVER(errMsg_441);
		return ;
	}
	return ;
}

void	Server::modeCommand(Msg msg, Client &client)
{

	if (channelChecks(msg, client) != 0)
	{
		return ;
	}
	Channel* tarChannel = getChannel(msg.parameters[0]);
	if (tarChannel == nullptr)
	{
		std::string message = ":" + msg.parameters[0] + " :No such channel\r\n";
		send(client.getSocket(), message.c_str(), message.size(), 0);
		LOG_SERVER(message);
		return ;
	}

	if (msg.parameters[1] == "+i" || msg.parameters[1] == "-i")
	{
		inviteMode(msg, client, tarChannel);
	}
	else if (msg.parameters[1] == "+t" || msg.parameters[1] == "-t")
	{
		topicMode(msg, client, tarChannel);
	}
	else if (msg.parameters[1] == "+k" || msg.parameters[1] == "-k")
	{
		keyMode(msg, client, tarChannel);
	}
	else if (msg.parameters[1] == "+o" || msg.parameters[1] == "-o")
	{			
		operatorMode(msg, client, tarChannel);
	}
	else if (msg.parameters[1] == "+l" || msg.parameters[1] == "-l" || (msg.parameters[1][0] == '+' && msg.parameters[1][1] == 'l'))
	{
		userLimitMode(msg, client, tarChannel);
	}
	else if (msg.parameters[1] == "b")
	{
		/* ignore ban list request*/
	}
	else
	{
		std::string errMsg = ":ircserver " + client.getNickname() + " " + msg.parameters[0] + " :Our channel does not support " + msg.parameters[1] + " \r\n";
		send(client.getSocket(), errMsg.c_str(), errMsg.size(), 0);
		LOG_SERVER(errMsg);
	}
}
