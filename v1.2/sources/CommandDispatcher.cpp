#include <cctype>
#include <CommandDispatcher.hpp>
#include <iostream>

#include "Client.hpp"
#include "IrcMessage.hpp"
#include "Server.hpp"

CommandDispatcher::CommandDispatcher(Server* server_ptr) :  _server(server_ptr){
	if (!_server) {
        throw std::runtime_error("CommandDispatcher initialized with nullptr Server!");
    }
}

CommandDispatcher::~CommandDispatcher() {}

void CommandDispatcher::dispatchCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params)
{
	
	if (!_server->validateRegistrationTime(client)) {return ;}
	if (!_server->validateClientNotEmpty(client)) {return;}
	int client_fd = client->getFd();
    std::string command = client->getMsg().getCommand();
	client->getMsg().printMessage(client->getMsg());
	if (command == "PASS") { _server->handlePassword(client, client_fd, params[0]); return;}
	else if (command == "CAP" && !client->getHasSentCap()) { _server->handleCapCommand(client);}
	else if (command == "QUIT") { _server->handleQuit(client);}
	else if (command == "USER" && !client->getHasSentUser()) { _server->handleUser(client, params);}
	else if (command == "NICK") { _server->handleNickCommand(client, _server->get_nickname_to_fd(), params[0]);}
	else if (command == "PING") { _server->handlePing(client);}
	else if (command == "PONG") { _server->handlePong(client);}
	else if (command == "KICK") { _server->handleKickCommand(client, params);}
	else if (command == "LEAVE" || command == "PART") { _server->handlePartCommand(client, params);}
	else if (command == "TOPIC") { _server->handleTopicCommand(client, params);}
	else if (command == "INVITE") { _server->handleInviteCommand(client, params);}
    else if (command == "JOIN") { _server->handleJoinChannel(client, params);}
	else if (command == "MODE") { _server->handleModeCommand(client, params);}
	else if (command == "PRIVMSG") { _server->handlePrivMsg(params, client);}
	else if (command == "WHOIS") { _server->handleWhoIs(client, params[0]);}
	
}