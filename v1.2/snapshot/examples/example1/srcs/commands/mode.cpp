#include "Server.hpp"

void Server::mode(std::string buf, int fd, int index)
{
	if (!validateClientRegistration(fd, index))
		return ;

	std::istringstream iss(buf);
	std::string command, chName, modeString, modeParam;
	iss >> command >> chName >> modeString;

	std::string nick = _clients[index]->getNickname();
	Channel *channel = findChannel(chName);
	if (chName.empty() || (chName[0] == '#' && !channel))
	{
		if (chName.empty())
			sendError("461 " + nick + " MODE :Not enough parameters", fd);
		else if (!channel)
			sendError("403 " + nick + " " + chName + " :No such channel", fd);
		return ;
	}

	if (modeString.empty() && chName[0] == '#')
	{
		sendResponse(":localhost 324 " + nick + " " + chName + " +" + channel->getChannelModes(), fd);
		sendResponse(":localhost 329 " + nick + " " + chName + " " + channel->getCreationTime(), fd);
		return;
	}

	if (chName[0] != '#')
	{
		if (modeString == "+i")
			return ;
		if (searchByUsername(chName) == -1)
			sendError("502 " + nick + " :Cant change mode for other users", fd);
		else
			sendError("403 " + nick + " " + chName + " :No such channel", fd);
		return ;
	}

	if (modeString == "b")
		return ;
	
	bool plussign = true;
	char modeSign = modeString[0];
	if (modeSign == '+')
		plussign = true;
	else if (modeSign == '-')
		plussign = false;
	else
	{
		sendError("502 " + nick + " :Cant change mode for other users", fd);
		return ;
	}

	if ((modeString[0] == '+' && (modeString[1] == 'k' || modeString[1] == 'l')) || modeString[1] == 'o')
	{
		std::getline(iss, modeParam);
		if (!modeParam.empty())
			modeParam = modeParam.substr(1);
		else
		{	
			if (modeString[1] != 'k')
				sendError("461 " + nick + " MODE :Not enough parameters", fd);
			return ;
		}
	}

	if (channel && channel->isAdmin(_clients[index]) == -1)
	{
		sendError("482 " + nick + " " + chName + " :You're not channel operator", fd);
		return ;
	}

	std::shared_ptr<Client> target = getClient(modeParam);
	std::string resMsg = ":localhost 324 " + nick + " " + chName + " " + modeSign;
	if (modeString[2])
	{
		modeString = modeString.substr(2);
		sendError("472 " + nick + " " + modeString + " :is unknown mode char to me", fd);
		return ;
	}
	char mode = modeString[1];
	switch (mode)
	{
		case 'i':
			if (plussign != channel->getInviteOnly())
			{
				channel->setInviteOnly(plussign);
				channel->broadcast(resMsg + mode + "\r\n", nullptr, 0);
			}
			break;
		case 't':
			if (plussign != channel->getTopicRestrictions())
			{
				channel->setTopicRestrictions(plussign);
				channel->broadcast(resMsg + mode + "\r\n", nullptr, 0);
			}
			break;
		case 'k':
			if (!plussign && plussign == channel->getIsChannelPassword())
				return ;
			channel->setChannelKey(plussign, modeParam);
			if (!plussign)
				modeParam = "*";
			channel->broadcast(resMsg + mode  + " " + modeParam + "\r\n", nullptr, 0);
			break;
		case 'o':
			if (!target)
			{
				sendError("401 " + nick + " " + modeParam + " :No such nick/channel", fd);
				return;
			}
			if (channel->isClient(target) == -1)
			{
				sendError("441 " + nick + " " + modeParam + " " + chName + " :They aren't on that channel", fd);
				return;
			}
			if (plussign && channel->isAdmin(target) == -1)
				channel->addAdmin(target);
			else if (!plussign && channel->isAdmin(target) >= 0)
				channel->removeAdmin(target);
			channel->broadcast(resMsg + mode + " " + modeParam + "\r\n", nullptr, 0);
			break;
		case 'l':
			if (!plussign && channel->getUserLimit() == -1)
				return ;
			if (!modeParam.empty())
			{
				try {
					int value = std::stoi(modeParam);
					if (value < 1)
						return ;
				} catch (...) {
					return ;
				}
			}
			if (plussign)
				channel->setUserLimit(std::stoi(modeParam));
			else
				channel->setUserLimit(-1);
			if (modeParam[0] == '+')
				modeParam = modeParam.substr(1);
			channel->broadcast(resMsg + mode + " " + modeParam + "\r\n", nullptr, 0);
			break;
		default:
		{
			sendError("472 " + nick + " " + mode + " :is unknown mode char to me", fd);
			break ;
		}
	}
}




