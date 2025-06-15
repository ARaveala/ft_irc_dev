#pragma once
#include <string>
#include "Channel.hpp"
#include <deque>
class Server;

class CommandDispatcher {

	private:
		Server* _server;
		std::deque<std::shared_ptr<Channel>> _channelsToNotify;
	public:
		CommandDispatcher(Server* server_ptr);
		~CommandDispatcher();
		void dispatchCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params);
		void manageQuit(const std::string& quitMessage, std::deque<std::shared_ptr<Channel>> channelsToNotify);
		//std::deque<std::shared_ptr<Channel>> getChannelsToNotify() { return _channelsToNotify;};
		//void addChannelToNotify() { _channelsToNotify.push_back();};
	// helpers 
		// mode
		bool initialModeValidation(std::shared_ptr<Client> client, const std::vector<std::string>& params, ModeCommandContext& context);
};