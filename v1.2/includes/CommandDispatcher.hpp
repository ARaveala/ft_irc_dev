#pragma once

#include <string>
#include <deque>

#include "Channel.hpp"
#include "Server.hpp"

class Server;

class CommandDispatcher {
	private:
		Server* _server;
	public:
		CommandDispatcher(Server* server_ptr);
		~CommandDispatcher();
		void dispatchCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params);
};