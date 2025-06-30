#include <cctype>
#include <CommandDispatcher.hpp>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sys/socket.h>

#include "Client.hpp"
#include "config.h"
#include "IrcMessage.hpp"
#include "IrcResources.hpp"
#include "MessageBuilder.hpp"
#include "Server.hpp"


// these where static but apparanetly that is not a good approach 
CommandDispatcher::CommandDispatcher(Server* server_ptr) :  _server(server_ptr){
	if (!_server) {
        throw std::runtime_error("CommandDispatcher initialized with nullptr Server!");
    }
	// what if __server == null?
}

CommandDispatcher::~CommandDispatcher() {}

void CommandDispatcher::dispatchCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params)
{
	
	if (!_server->validateRegistrationTime(client)) {return ;}
	if (!_server->validateClientNotEmpty(client)) {return;}
	int client_fd = client->getFd();
    std::string command = client->getMsg().getCommand();
	client->getMsg().printMessage(client->getMsg());
	if (command == "PASS") {
		_server->handlePassword(client, client_fd, params[0]);
	}
	if (command == "CAP" && !client->getHasSentCap()) {
		_server->handleCapCommand(client);
	}
	if (command == "QUIT") {
		_server->handleQuit(client);
		return ;
	}
	if (command == "USER" && !client->getHasSentUser()) {
		_server->handleUser(client, params);
	}
	if (command == "NICK") {
		_server->handleNickCommand(client, _server->get_nickname_to_fd(), params[0]);
	}
	if (command == "PING"){
		_server->handlePing(client);
		return;
	}
	if (command == "PONG"){
		_server->handlePong(client);
		return;
	}
	if (command == "KICK"){
		_server->handleKickCommand(client, params);
		return;
	}
	if (command == "LEAVE" || command == "PART"){
        _server->handlePartCommand(client, params);
        return;
	}
	if (command == "TOPIC"){
        _server->handleTopicCommand(client, params);
		return;
	}
	if (command == "INVITE"){
        _server->handleInviteCommand(client, params);
		return;
	}
    if (command == "JOIN"){
		_server->handleJoinChannel(client, params);
	}
	if (command == "MODE") {
		_server->handleModeCommand(client, params);
	}
	if (command == "PRIVMSG")  {
		_server->handlePrivMsg(params, client);
	}
	if (command == "WHOIS") {
		_server->handleWhoIs(client, params[0]);
	}
	
}