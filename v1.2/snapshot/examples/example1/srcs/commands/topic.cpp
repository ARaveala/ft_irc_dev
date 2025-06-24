#include "Server.hpp"

void Channel::setTopic(const std::string &topic, const std::shared_ptr<Client>& admin)
{
	_topic = topic;
	std::string notification = "Channel topic changed by " + admin->getNickname();
	broadcastAdmins(notification);
	broadcast("The topic has been updated to: " + topic, nullptr, 0);
}

void Channel::broadcastTopic(const std::string &chName, const std::string &topic)
{
	_topic = topic;
	for (size_t i = 0; i < _clients.size(); i++)
	{
		std::string msg = ":localhost 332 " + _clients[i]->getNickname() + " " + chName + " :" + topic + "\r\n";
		send(_clients[i]->getSocket(), msg.c_str(), msg.length(), 0);
	}
}

void Server::topic(std::string buf, int fd, int index)
{
	if (!validateClientRegistration(fd, index))
		return ;

	std::istringstream iss(buf);
	std::string command, chName, newTopic;
	iss >> command >> chName;

	Channel *channel = findChannel(chName);
	if (chName.empty() || !channel)
	{
		if (chName.empty())
			sendError("461 " + _clients[index]->getNickname() + " " + chName + " :Not enough parameters", fd);
		else
			sendError("403 " + _clients[index]->getNickname() + " " + chName + " :No such channel", fd);
		return;
	}

	if (channel->isClient(_clients[index]) == -1)
	{
		sendError("442 " + _clients[index]->getNickname() + " " + chName + " :You're not on that channel", fd);
		return;
	}

	if (channel->getTopicRestrictions() && channel->isAdmin(_clients[index]) == -1)
	{
		sendError("482 "  + _clients[index]->getNickname() + " " + chName + " :You're not channel operator", fd);
		return;
	}

	std::getline(iss, newTopic);
	if (!newTopic.empty())
	{
		if (newTopic.size() > 0 && newTopic[1] == ':')
			newTopic = newTopic.substr(2);
		channel->broadcastTopic(channel->getName(), newTopic);
	}
	else
		sendResponse(":localhost 331 " + _clients[index]->getNickname() + " " + channel->getName() + " :No topic is set", fd);
}


