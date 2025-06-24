#include "Server.hpp"

void Server::invite(std::string buf, int fd, int index)
{
	if (!validateClientRegistration(fd, index))
		return ;
	
	std::istringstream iss(buf);
	std::string command, inviter, chName;
	iss >> command >> inviter >> chName;
	
	std::string nick = _clients[index]->getNickname();

	if (inviter.empty() || chName.empty() || searchByNickname(inviter) == -1)
	{
		if (inviter.empty() || chName.empty())
			sendError("461 " + nick + " INVITE :Not enough parameters", fd);
		else
			sendError("401 " + nick + " " + inviter + " :No such nick/channel", fd);
		return;
	}

	Channel* channel = findChannel(chName);
	if (!channel || channel->isAdmin(_clients[index]) == -1)
	{
		if (!channel)
			sendError("403 " + nick + " " + chName + " :No such channel", fd);
		else
			sendError("482 " + nick + " " + chName + " :You're not channel operator", fd);
		return;
	}

	if (channel->isClient(_clients[searchByNickname(inviter)]) >= 0)
	{
		sendError("443 " + nick + " " + inviter + " " + chName + " :is already on channel", fd);
		return ;
	}

	sendResponse(":localhost 341 " + nick + " " + inviter + " " + chName, fd);
	sendResponse(":" + nick + "!" + _clients[index]->getUsername() + "@localhost INVITE " + inviter + " " + chName, _clients[searchByNickname(inviter)]->getSocket());
	if (channel->isInvited(_clients[searchByNickname(inviter)]) < 0)
		channel->addInvited(_clients[searchByNickname(inviter)]);
}