#include "Server.hpp"

bool isValidChannelName(const std::string &channel)
{
	if (channel.empty() || channel[0] != '#' || channel.length() > 200 || channel.length() < 2)
		return false;
	for (size_t i = 1; i < channel.length(); i++)
	{
		char c = channel[i];
		if (!std::isalnum(c) && c != '-' && c != '_' && c != '.')
			return false;
	}
	return true;
}

Channel *Server::findChannel(const std::string &name)
{
	for (size_t i = 0 ; i < _channels.size() ; i++)
	{
		if (_channels[i]->getName() == name)
			return _channels[i];
	}
	return nullptr;
}

void Server::sendChannelCreationResponse(const std::string &nick, const std::string &user, const std::string &channel, int fd)
{
	const std::vector<std::string> messages = 
	{
		":" + nick + "!" + user + "@localhost JOIN " + channel,
		":localhost MODE " + channel + " +t",
		":localhost 353 " + nick + " = " + channel + " :@" + nick,
		":localhost 366 " + nick + " " + channel + " :End of /NAMES list",
	};

	for (const auto &msg : messages)
		sendResponse(msg, fd);
}

Channel *Server::createChannel(const std::string &name, const std::shared_ptr<Client> creator, int fd)
{
	if (!isValidChannelName(name))
	{
		sendError("403 " + creator->getNickname() + " " + name + " :No such channel", fd);
		return nullptr;
	}

	Channel *newChannel = new Channel(name);

	newChannel->addClient(creator);
	newChannel->addAdmin(creator);
	_channels.push_back(newChannel);
	
	sendChannelCreationResponse(creator->getNickname(), creator->getUsername(), name, fd);
	return newChannel;
}
