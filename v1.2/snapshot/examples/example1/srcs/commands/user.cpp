#include "Server.hpp"

/*
	Registers and welcomes user to the server on success
	checks if user has authenticated and regirsted to the server
*/
void Server::user(std::string buf, int fd, int index)
{
	if (!_clients[index] || !_clients[index]->getIsAuthenticated() || _clients[index]->getIsRegistered())
	{
		if (_clients[index] && !_clients[index]->getIsAuthenticated())
			sendError("491 : Password required to connect", fd);
		return;
	}

	std::string user = buf.substr(5);
	if (user.empty() || _clients[index]->getNickname().empty())
	{
		if (user.empty())
			sendError("451 :You have not registered", fd);
		return;
	}

	sendResponse(
		":localhost 001 " + _clients[index]->getNickname() + 
		" :- - - - - - - - - - - - - - - - - - - - - -\n"
		" ______   \n"
		"|      \\       (\\_/)\n"
		"|       \\     (o.o )  Welcome\n"
		"|  MAIL  |    ( :   \\  to our IRC\n"
		"|  BOX   |    (\\ /   )    server " + _clients[index]->getNickname() + "!\n"
		"|________|    \n"
		"- - - - - - - - - - - - - - - - - - - - -\n"
		"          \n"
		"Available commands:\n"
		"¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨\n"
		" -> \x02/nick\x02 <nickname> - Change your nickname\n"
		" -> \x02/msg\x02 <nickname> <message> - Send a private message\n"
		" -> \x02/join\x02 <#channel> - Join a channel\n"
		"    -> \x02/topic\x02 <#channel> <topic> - Set a topic for the channel\n"
		"    -> \x02/mode\x02 <#channel> <mode> (mode parameters) - Set a mode for the channel\n"
		"    -> \x02/kick\x02 <#channel> <nickname> (reason) - Kick a user from the channel\n"
		"    -> \x02/invite\x02 <nickname> <#channel> - Invite a user to the channel\n"
		" -> \x02/quit\x02 - Disconnect from the server\n"
		" -> \x02/list\x02 - Shows a list of channels\n"
		" -> \x02/window\x02 <index> - Change window to index\n"
	, fd);

	size_t spacePos = user.find(' ');
	if (spacePos != std::string::npos)
		user = user.substr(0, spacePos);

	_clients[index]->setUsername(user);
	_clients[index]->setIsRegistered(true);
	std::cout << "Connecting client " << _clients[index]->getNickname() << std::endl;
}
