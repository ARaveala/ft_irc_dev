#include "Server.hpp"

void Server::list(std::string buf, int fd, int index)
{
	std::istringstream iss(buf);
	std::string command, chName;
	std::vector<std::string> channels;
	iss >> command >> chName;

	sendResponse(":localhost 321 " + _clients[index]->getNickname() + " Channel :Users  Name", fd);
	if (chName.empty())
	{
		for (size_t i = 0; i < _channels.size(); ++i)
		{
			std::string response = ":localhost 322 " + _clients[index]->getNickname() + " " +  _channels[i]->getName() + " " + std::to_string(_channels[i]->getUserCount()) + " :" +_channels[i]->getTopic();
			sendResponse(response, fd);
		}
		sendResponse(":localhost 321 " + _clients[index]->getNickname() + " :End of /LIST", fd);
		return;
	}
	
	std::string channel;
	std::istringstream channelStream(chName);
	while (std::getline(channelStream, channel, ','))
		channels.push_back(channel);
	
	size_t j = 0;
	do {
		for (size_t i = 0; i < _channels.size(); ++i)
		{
			if (_channels[i]->getName() == channels[j])
			{
				std::string response = ":localhost 322 " + _clients[index]->getNickname() + " " +  _channels[i]->getName() + " " + std::to_string(_channels[i]->getUserCount()) + " :" +_channels[i]->getTopic();
				sendResponse(response, fd);
				j++;
			}
		}
	} while (j < channels.size());
	sendResponse(":localhost 321 " + _clients[index]->getNickname() + " :End of /LIST", fd);
}
