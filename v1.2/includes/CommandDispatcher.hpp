#pragma once

#include <string>
#include <deque>

#include "Channel.hpp"
#include "Server.hpp"

class Server;

class CommandDispatcher {
	private:
		Server* _server;
		//std::deque<std::shared_ptr<Channel>> _channelsToNotify;
	public:
		CommandDispatcher(Server* server_ptr);
		~CommandDispatcher();
		void dispatchCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params);
};