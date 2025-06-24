#include "Client.hpp"
#include "Server.hpp"
#include <iostream> // debuggind
#include "IrcMessage.hpp"

void Client::setCommandMap(Server &server)
{
	_commandMap["QUIT"] = [&]() {
		_msg.readyQuit(
			server.getChannelsToNotify(),
			[&](std::deque<std::string>& channels) { prepareQuit(channels); },
			getFd(),
			[&](int fd) { server.remove_Client(fd); }
		);
	};
}


void Client::executeCommand(const std::string& command) {
	auto it = _commandMap.find(command);
	if (it != _commandMap.end()) {
		it->second();
	} else {
		std::cout << "Unknown command: " << command << std::endl;
	}
}
