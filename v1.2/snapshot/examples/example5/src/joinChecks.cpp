
#include "Server.hpp"

int		Channel::getNumberOfChannelUsers()
{
	std::vector<User>	channel_users;
	int					num_of_channel_users;

	channel_users 			= this -> getChannelUsers();
	num_of_channel_users 	= channel_users.size();

	return (num_of_channel_users);
}

bool		Channel::doesChannelHavePassword()
{
	if (this->_keyRequired == false)
	{
		return (false);
	}
	else
	{
		return (true);
	}
}

/*
	Returns true, if Number of Channel Users equals or exceeds Channel User Limit.
	Returns false,
		i if that is not the case
		ii. if no Channel Limit set (i.e. user_limit = -1).
*/
bool		Channel::isChannelFull()
{
	int user_limit 				= this -> getUserLimit();
	int	num_of_channel_users 	= this -> getNumberOfChannelUsers();

	if (user_limit == -1 || user_limit > num_of_channel_users)
	{
		return (false);
	}
	else
	{
		return (true);
	}
}

/*
	Checks if
	i. Channel is full
	ii. Channel has password
	iii. User is invited
*/
int		Server::channelJoinChecks(Channel channel, Msg msg, Client &client)
{
	std::string message;
	
	if (channel.isChannelFull() == true)
	{
		message = ":ircserver 471 " + client.getNickname() +  " " + msg.parameters[0] + " :Cannot join channel (+l) - channel is full, try again later\r\n";
		send(client.getSocket(), message.c_str(), message.size(), 0);
		LOG_SERVER(message);
		return (1) ;
	}

	if (channel.doesChannelHavePassword() == true)
	{
		if (msg.parameters.size() <= 1)						 //No password parameter passed.
		{
			message = ":ircserver 475 " + client.getNickname() + " " + msg.parameters[0] + " :Cannot join channel (+k) - no password entered\r\n";
			send(client.getSocket(), message.c_str(), message.size(), 0);
			LOG_SERVER(message);
			return (1);
		}

		std::string 	password = msg.parameters[1];
		if (channel.getChannelKey() == password) 			//Password Correct
		{

		}
		else 												//Password Incorrect
		{
			message = ":ircserver 475 " + client.getNickname() + " " + msg.parameters[0] + " :Cannot join channel (+k) - password incorrect\r\n";
			send(client.getSocket(), message.c_str(), message.size(), 0);
			LOG_SERVER(message);
			return (1);
		}
	}

	if (channel.isChannelInviteOnly() == true)
	{
		for (auto &it : channel.getInvitedList())
		{
			if (it == client.getNickname())
			{
				return (0);				//User was Invited
			}
		}

		message  = ":ircserver 473 " + client.getNickname() + " " + msg.parameters[0] + " :Cannot join channel (+i) - you must be invited\r\n";
		send(client.getSocket(), message.c_str(), message.size(), 0);
		LOG_SERVER(message);
		return (1);						//User was NOT Invited
	}
	return (0);
}
