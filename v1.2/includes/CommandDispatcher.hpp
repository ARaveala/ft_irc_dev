#pragma once
#include <string>
class Server;

class CommandDispatcher {

	public:
		static void handle_message(const std::string message, Server& server, int fd);
};