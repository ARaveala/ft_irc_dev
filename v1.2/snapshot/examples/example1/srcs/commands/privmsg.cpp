#include "Server.hpp"

void Server::privmsg(std::string buf, int fd, int index)
{
	if (!validateClientRegistration(fd, index))
		return ;

	buf.replace(buf.find("PRIVMSG "), 8, "");
	size_t pos = buf.find(" :");
	
	std::string name = buf.substr(0, pos);
	std::string msg = buf.substr(pos + 2);
	std::string response = "";
	
	if (name.empty())
	{
		sendError("411 :No recipient given PRIVMSG", fd);
		return;
	}
	
	if (searchByNickname(name) != -1 && name[0] != '#')
	{
		if (_clients[index]->getNickname() != name)
			response += ":" + _clients[index]->getNickname() + " ";
		response += "PRIVMSG " + name + " :" + msg;
		sendResponse(response, _poll.getFds()[searchByNickname(name) + 1].fd);
	}
	else if (name[0] == '#')
	{
		int channelIndex = getChannelIndex(name);
		if (channelIndex == -1)
		{
			sendError("403 " + name + " :No such channel", fd);
			return;
		}
		Channel *channel = findChannel(name);
		if (channel->isClient(_clients[index]) == -1)
		{
			sendError("404 " + _clients[index]->getNickname() + " " + name + " :Cannot send to nick/channel", fd);
			return;
		}
		std::string message = ":" + _clients[index]->getNickname() + " PRIVMSG " + name + ' ' + msg + "\r\n";
		_channels[channelIndex]->broadcast(message, _clients[index], 1);
	}
	else
		sendError("401 " + _clients[index]->getNickname() + " " + name + " :No such nick/channel", fd);
}
