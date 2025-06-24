#include "Server.hpp"

std::vector<std::string> split(std::string str, std::string token)
{
	std::vector<std::string> result;
	while (str.size())
	{
		size_t index = str.find(token);
		if (index != std::string::npos)
		{
			result.push_back(str.substr(0, index));
			str = str.substr(index + token.size());
			if (str.size() == 0) 
				result.push_back(str);
		}
		else
		{
			result.push_back(str);
			str = "";
		}
	}
	return result;
}

void		Server::partCommand(Msg msg, Client &client)
{
	std::vector<std::string> channels = split(msg.parameters[0], ",");
	if (channels.size() == 0)
	{
		std::string notice_461 = ":ircserver 461 " + client.getNickname() + " PART :Not enough parameters\r\n";
		send(client.getSocket(), notice_461.c_str(), notice_461.size(), 0);
		LOG_SERVER(notice_461);
		return ;
	}
	for (size_t i = 0; i < channels.size(); i++)
	{
		bool channelExists = false;
		for (auto &channel : _channel_names)
		{
			if (channel.getChannelName() == channels[i])
			{
				channelExists = true;
				if (userExists(client.getNickname(), channels[i]))
				{
					int j = getChannelIndex(channels[i], _channel_names);
					std::string part;
					if (!msg.trailing_msg.empty())
						part = ":" + client.getPrefix() + " PART " + channel.getChannelName() + " :" + msg.trailing_msg + "\r\n";
					else
						part = ":" + client.getPrefix() + " PART " + channel.getChannelName() + "\r\n";
					broadcastToChannel(_channel_names[j], part, client, 1);
					removeUser(client.getNickname(), channel.getChannelName(), part, 0);
					client.leaveChannel(channel.getChannelName());
					break;
				} else {
					std::string notice_442 = ":ircserver 442 " + client.getNickname() + " " + channels[i] + " :You're not on that channel\r\n";
					send(client.getSocket(), notice_442.c_str(), notice_442.size(), 0);
					LOG_SERVER(notice_442);
				}
				break;
			}
		}
		if (!channelExists)
		{
			std::string notice_403 = ":ircserver 403 " + client.getNickname() + " " + channels[i] + " :No such channel\r\n";
			send(client.getSocket(), notice_403.c_str(), notice_403.size(), 0);
			LOG_SERVER(notice_403);
		}
	}
	for (auto &channel : _channel_names)
	{
		if (channel.getChannelUsers().size() == 0)				//Removes channel if it has no users
		{
			int i = getChannelIndex(channel.getChannelName(), _channel_names);
			_channel_names.erase(_channel_names.begin() + i);
		}
	}
	return ;
}