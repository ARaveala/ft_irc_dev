#include "Server.hpp"

void Server::kick(std::string buf, int fd, int index)
{
	if (!validateClientRegistration(fd, index))
		return ;
	
	std::istringstream iss(buf);
	std::string command, kick, chName, reason;
	iss >> command >> chName >> kick;
	std::string nick = _clients[index]->getNickname();

	if (kick.empty())
		sendError("461 " + nick + " KICK :Not enough parameters", fd);
		
	size_t reasonStart = buf.find(':', buf.find(kick));
	if (reasonStart != std::string::npos)
	{
		reasonStart++;
		reason = buf.substr(reasonStart);
	} 
	else
		reason = nick;
		

	int clientIndex = searchByNickname(kick);
	if (clientIndex == -1)
	{
		sendError("401 " + nick + " " + kick + " :No such nick/channel", fd);
		return;
	}

	Channel *channel = findChannel(chName);
	if (chName.empty() || !channel || channel->isAdmin(_clients[index]) == -1
		|| channel->isClient(_clients[clientIndex]) == -1)
	{
		if (chName.empty())
			sendError("461 " + nick + " KICK :Not enough parameters", fd);
		else if (!channel)
			sendError("403 " + chName + " :No such channel", fd);
		else if (channel->isAdmin(_clients[index]) == -1)
			sendError("482 " + nick + " " + chName + " :You're not channel operator", fd);
		else
			sendError("441 " + nick + " " + kick + " " + chName + " :They aren't on that channel", fd);
		return;
	}

	channel->broadcast(":" + _clients[index]->getNickname() + "!" + _clients[index]->getUsername() + "@" + std::to_string(_clients[clientIndex]->getIpAddress()) + " KICK " + channel->getName() + " " + _clients[clientIndex]->getNickname() + " :" + reason + "\r\n", nullptr, 0);
	if (channel->isAdmin(_clients[clientIndex]) != -1)
		channel->removeAdmin(_clients[clientIndex]);
	channel->removeClient(_clients[clientIndex]);
}