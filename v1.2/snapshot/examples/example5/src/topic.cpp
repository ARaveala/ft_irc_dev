#include "Server.hpp"

void Server::topicPrint(std::string channelName, Client &client)
{
	bool found = false;
	for (std::vector<Channel>::iterator it = _channel_names.begin(); it != _channel_names.end(); it++)
	{
		if (it->getChannelName() == channelName)
		{
			found = true;
			std::string channelTopic = it->getChannelTopic();
			if (channelTopic.empty())
			{
				std::string noTopic = ":ircserver 331 " + client.getNickname() + " " + channelName + " :No topic is set\r\n";
				send(client.getSocket(), noTopic.c_str(), noTopic.size(), 0);
				break;
			}
			std::string topicMsg = ":ircserver 332 " + client.getNickname() + " " + channelName + " :" + channelTopic + "\r\n";
			send(client.getSocket(), topicMsg.c_str(), topicMsg.size(), 0);
			LOG_SERVER(topicMsg);
			time_t rawTime = stringToUnixTimeStamp(it->getTopicSetTime());
			rawTime += 3600;
			std::string topicSetByMsg = ":ircserver 333 " + client.getNickname() + " " + channelName + " " + it->getTopicSetter() + " " + std::to_string(rawTime) + "\r\n";
			send(client.getSocket(), topicSetByMsg.c_str(), topicSetByMsg.size(), 0);
			break;
		}
	}
	if (!found)
	{
		std::string noSuchChannel = ":ircserver 403 " + client.getNickname() + " " + channelName + " :No such channel\r\n";
		send(client.getSocket(), noSuchChannel.c_str(), noSuchChannel.size(), 0);
		LOG_SERVER(noSuchChannel);
	}
}

void	Server::topicCommand(Msg msg, Client &client)
{
	if (msg.trailing_msg.size() == 0) 
	{
		topicPrint(msg.parameters[0], client);
	}
	else
	{
		int i = getChannelIndex(msg.parameters[0], _channel_names);
		if (i == -1)
		{
			std::string notice = ":ircserver 403 " + client.getNickname() + " " + msg.parameters[0] + " :No such channel\r\n";
			send(client.getSocket(), notice.c_str(), notice.size(), 0);
			LOG_SERVER(notice);
			return ;
		}
		if (_channel_names[i].getTopicRequiresOperator() && clientStatus(msg, client) == 0)
		{
			std::string errMsg_482 = ":ircserver 482 " + client.getNickname() + " " + msg.parameters[0] + " :You're not a channel operator\r\n";
			send(client.getSocket(), errMsg_482.c_str(), errMsg_482.size(), 0);
			LOG_SERVER(errMsg_482);
			return ;
		}
		_channel_names[i].setChannelTopic(msg.trailing_msg, client);
		std::string topicMsg = ":" + client.getPrefix() + " TOPIC " + _channel_names[i].getChannelName() + " :" + msg.trailing_msg + "\r\n";
		broadcastToChannel(_channel_names[i], topicMsg, client, 0);
	}
	return ;
}

