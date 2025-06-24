#include "../server/Server.h"
#include "Command.h"
#include "../debug/debug.h"

Command::Command(Server *server_ptr) : server_ptr_(server_ptr)
{
}

Command::~Command()
{
}

/**
 * @brief sends back a PONG when client sends a PING
 * when client sends a PING it contains a token, which has to be sent back to the client
 * this is done in order to track the latency between client and server
 * TODO: keeping track of PING PONG status, time of receiving message to the client class?
 * @param msg
 */
void Command::handlePing(const Message &msg)
{
	auto client_ptr = msg.getClientPtr();
	int client_fd = client_ptr->getFd();
	std::vector<std::string> parameters = msg.getParameters();
	if (parameters.empty())
	{
		server_ptr_->sendResponse(client_fd, ERR_NEEDMOREPARAMS(client_ptr->getClientPrefix(), "PING"));
	}
	else
		server_ptr_->sendResponse(client_fd, PONG(server_ptr_->getServerHostname(), parameters.front())); // latter parameter is the token received from client
}

bool Command::channelExists(std::string const &channel_name)
{
	return server_ptr_->findChannel(channel_name) != nullptr;
}

void Command::handleCap(const Message &msg)
{
	(void)msg;
	return;
}

std::vector<std::string> Command::split(const std::string &s, char delim)
{
	std::vector<std::string> result;
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim))
		result.push_back(item);
	return result;
}

bool Command::isValidChannelName(const std::string& channel_name) const
{
	// Regex to match valid channel names
	std::regex pattern("^[&#\\+!][^ ,\\x07]{1,49}$"); // Adjusted for max length of 50 and disallowed characters
	return std::regex_match(channel_name, pattern);
}