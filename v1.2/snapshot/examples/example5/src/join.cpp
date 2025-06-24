#include "Server.hpp"

/*
	1. Makes a new channel user with...
		i. Nickname
		ii. Permissions (operator (= true/false))
	2. Adds user into vector of channel users
*/
void	Server::addChannelUser(Channel &channel, Client &client, bool operator_permissions)
{
	//Make new Channel User
	User	new_user;
	new_user.nickname = client.getNickname();
	new_user.operator_permissions = operator_permissions;

	//Add into channel Users
	channel.addUserIntoChannelUsers(new_user);
	client.joinChannel(channel.getChannelName());
}

void	Server::createChannel(Msg msg, Client &client)
{
	Channel		new_channel(msg.parameters[0]);

	addChannelUser(new_channel, client, true);
	this->_channel_names.push_back(new_channel);
}

void Server::joinChannelMessage(std::string channelName, Client &client)
{
	int i = getChannelIndex(channelName, this->_channel_names);

	std::string message = ":" + client.getNickname() + "!" + client.getUsername() + "@" + client.getHost() + " JOIN " + channelName + "\r\n";
	broadcastToChannel(this->_channel_names[i], message, client, 0);

	message = ":ircserver MODE " + channelName + " +t\r\n";
	send(client.getSocket(), message.c_str(), message.size(), 0);

	std::string topic = this->_channel_names[i].getChannelTopic();
	if (!topic.empty())
		topicPrint(channelName, client);

	std::string	result;
    const std::vector<User>& channel_users = _channel_names[i].getChannelUsers();

    for (size_t i = 0; i < channel_users.size(); ++i) 
	{
		if (channel_users[i].operator_permissions)
			result += "@" + channel_users[i].nickname;
		else
			result += channel_users[i].nickname;

		if (i != channel_users.size() - 1) 
			result += " ";  // Add a space between names except the last one
    }
	message = ":ircserver 353 " + client.getNickname() + " @ " + channelName + " :" + result + "\r\n";
	send(client.getSocket(), message.c_str(), message.size(), 0);
	LOG_SERVER(message);
	message = ":ircserver 366 " + client.getPrefix() + " " + channelName + " :End of /NAMES list" + "\r\n";
	send(client.getSocket(), message.c_str(), message.size(), 0);
	LOG_SERVER(message);
	message = ":ircserver 329 " + client.getNickname() + " " + channelName + " " + this->_channel_names[i].getChannelTime() + "\r\n";
	send(client.getSocket(), message.c_str(), message.size(), 0);
	LOG_SERVER(message);
}

void		Server::joinCommand(Msg msg, Client &client)
{
	int i = getChannelIndex(msg.parameters[0], this->_channel_names);

	if (i == -1)	//No channel found
		createChannel(msg, client);
	else 			//Join channel
	{
		if (channelJoinChecks(this ->_channel_names[i], msg, client))
			return ;
		addChannelUser(this->_channel_names[i], client, false);
	}
	joinChannelMessage(msg.parameters[0], client);
}
