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
	std::string command = client->getMsg().getCommand();

	if (!_server->validateRegistrationTime(client)) {return ;}
	int client_fd = client->getFd();
    if (!_server->validateClientNotEmpty(client)) {return;}
	client->getMsg().printMessage(client->getMsg());
	const std::string& nickname = client->getNickname();
	if (command == "PASS") {
		_server->handlePassword(client, client_fd, params[0]);
	}

	if (command == "CAP" && !client->getHasSentCap()) {
		_server->handleCapCommand(nickname, client->getMsg().getQue(), client->getHasSentCap());
	}

	if (command == "QUIT") {
		_server->handleQuit(client);
		return ;
	}

	//handler needed
	if (command == "USER" && !client->getHasSentUser()) {
		client->setClientUname(params[0]);
		client->setRealname(params[3]);
		client->setHasSentUser();
		_server->tryRegisterClient(client);
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
		std::cout << "COMMAND DISPATCHER: " << command << " command recieved. Calling Server::handleKickCommand." << std::endl;
		_server->handleKickCommand(client, params);
		return;
	}

	if (command == "LEAVE" || command == "PART"){
		std::cout << "COMMAND DISPATCHER: " << command << " command received. Calling Server::handlePartCommand.\n";
        _server->handlePartCommand(client, params);
        return;
	}

	if (command == "TOPIC"){
		std::cout << "COMMAND DISPATCHER: " << command << " command received. Calling Server::handleTopicCommand.\n";
        _server->handleTopicCommand(client, params);
		return;
	}

	if (command == "INVITE"){
		std::cout << "COMMAND DISPATCHER: " << command << " command received. Calling Server::handleInviteCommand.\n";
        _server->handleInviteCommand(client, params);
		return;
	}

    if (command == "JOIN"){
		std::cout << "COMMAND DISPATCHER: " << command << " command received. Calling Server::handleJoinChannel.\n";
		_server->handleJoinChannel(client, params);
	}

	if (command == "MODE") {
		std::cout << "COMMAND DISPATCHER: " << command << " command received. Calling Server::handleModeCommand.\n";
		_server->handleModeCommand(client, params);
	}

	if (command == "PRIVMSG")  {
		std::cout << "COMMAND DISPATCHER: " << command << " command received.\n";
		if (!params[0].empty())
		{
			std::string contents = MessageBuilder::buildPrivMessage(client->getNickname(), client->getUsername(), params[0], params[1]);//":" + client->getNickname()  + " PRIVMSG " + params[0] + " " + params[1] +"\r\n";
			
			if (params[0][0] == '#')
			{
				std::cout<<"DEBUGGIN PRIVMESSAGE CHANNEL EDITION :: before validate channel\n";
				if (!_server->validateChannelExists(client, params[0], client->getNickname())) { return;}
				_server->broadcastMessage(contents, client,_server->get_Channel(params[0]), true, nullptr);
			}
			else
			{
				std::shared_ptr<Client> target = _server->getClientByNickname(params[0]);
				if (!_server->validateTargetExists(client, target, client->getNickname(), params[0])) { return ;}
				int fd = target->getFd();
				_server->broadcastMessage(contents, client, nullptr, true, _server->get_Client(fd));				
			}
		}		
	}
	if (command == "WHOIS") {
		std::cout << "COMMAND DISPATCHER: " << command << " command received. Calling Server::handleWhoIs.\n";
		_server->handleWhoIs(client, params[0]);
	}
	
}