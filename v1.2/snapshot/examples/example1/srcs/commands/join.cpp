#include "Server.hpp"

void Server::join(std::string buf, int fd, int index)
{
	if (!validateClientRegistration(fd, index))
		return ;
	
	std::string nick = _clients[index]->getNickname();
	
	std::istringstream iss(buf);
	std::string command, channelsStr, passwordsStr;
	std::vector<std::string> channels;
	std::vector<std::string> passwords;
	
	iss >> command >> channelsStr >> passwordsStr;
	
	std::string channel;
	std::istringstream channelStream(channelsStr);
	while (std::getline(channelStream, channel, ','))
		channels.push_back(channel);

	std::string password;
	std::istringstream passwordStream(passwordsStr);
	while (std::getline(passwordStream, password, ','))
		passwords.push_back(password);

	for (size_t i = 0; i < channels.size(); ++i)
	{
		std::string chName = channels[i];
		if (chName.empty() || chName[0] != '#')
		{
			if (chName.empty())
				sendError("461 " + nick + " JOIN :Not enough parameters", fd);
			else
				sendError("403 " + nick + " " + chName + " :No such channel", fd);
			continue;
		}
		
		Channel *channel = findChannel(chName);
		if (channel == nullptr)
		{
			channel = createChannel(chName, _clients[index], fd);
			if (channel == nullptr)
				continue;
		}
		
		if (channel->getUserLimit() != -1 && channel->getUserCount() >= channel->getUserLimit())
		{
			sendError("471 " + nick + " " + chName + " :Cannot join channel (+l)", fd);
			continue;
		}
		
		if (channel->getInviteOnly() && channel->isInvited(_clients[index]) == -1)
		{
			sendError("473 " + nick + " " + chName + " :Cannot join channel (+i)", fd);
			continue;
		}
		
		if (channel->getIsChannelPassword() && ((i >= passwords.size()) || (!passwords[i].empty() && channel->getPassword() != passwords[i])))
		{
			sendError("475 " + nick + " " + chName + " :Cannot join channel (+k)", fd);
			continue;
		}

		if (!isInChannel(_clients[index], channel))
		{
			channel->addClient(_clients[index]);
			channel->broadcast(":" + nick + "!" + _clients[index]->getUsername() + "@localhost JOIN " + channel->getName() + "\r\n", nullptr, 0);
			if (!channel->getTopic().empty())
				sendResponse(":localhost 332 " + nick + " " + channel->getName() + " :" + channel->getTopic(), fd);
			std::string joinMsg = ":localhost 353 " + nick + " = " + channel->getName() + " :";
			for (const auto& clientPtr : channel->getClients())
			{	
				if (channel->isAdmin(clientPtr) != -1)
					joinMsg += "@";
				if (clientPtr)
					joinMsg += clientPtr->getNickname() + " ";
			}
			joinMsg.pop_back();
			sendResponse(joinMsg, fd);
			sendResponse(":localhost 366 " + nick + " " + channel->getName() + " :End of /NAMES list", fd);
		}
	}
}

