#include <CommandDispatcher.hpp>
#include <iostream>
#include "Server.hpp"
#include "Client.hpp"
#include "IrcMessage.hpp"
#include <cctype>
#include <regex>
#include "IrcResources.hpp"
#include "MessageBuilder.hpp"
#include "config.h"


// these where static but apparanetly that is not a good approach 
CommandDispatcher::CommandDispatcher(Server* server_ptr) :  _server(server_ptr){
	if (!_server) {
        throw std::runtime_error("CommandDispatcher initialized with nullptr Server!");
    }
	// what if __server == null?
}

CommandDispatcher::~CommandDispatcher() {}

#include <sys/socket.h>
#include <iomanip>
void CommandDispatcher::dispatchCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params)
{
	int client_fd = client->getFd();
    if (!client) {
        std::cerr << "Error: Client pointer is null in dispatchCommand()" << std::endl;
        return;
    }
	client->getMsg().printMessage(client->getMsg());
	std::string command = client->getMsg().getCommand();
	const std::string& nickname = client->getNickname();
	if (command == "CAP" && !client->getHasSentCap()) {
		_server->handleCapCommand(nickname, client->getMsg().getQue(), client->getHasSentCap());
	}
	if (command == "QUIT") {
		_server->handleQuit(client);
		return ;
	}
	
	/*	    PART
	    LEAVE
	    TOPIC
	    NAMES
	    LIST
	    INVITE
	    PARAMETER NICKNAME
		
*/
	/*if (command == "KICK") {
			
	}
	if (command == "INVITE") {
			
	}*/

	if (command == "USER" && !client->getHasSentUser()) {
		// if (params.size() == 1/2)
		// 	chnage username to param, follow same logic as nickname? 
		client->getMsg().queueMessage("USER " + client->getClientUname() + " 0 * :" + client->getfullName() +"\r\n");
		client->setHasSentUser();
	}
	if (command == "NICK") {
		_server->handleNickCommand(client, _server->get_fd_to_nickname(), _server->get_nickname_to_fd(), params[0]);
		return ; 
	}
	if (!client->getHasRegistered() && client->getHasSentNick() && client->getHasSentUser()) {
		client->setHasRegistered();
		client->getMsg().queueMessage(MessageBuilder::generatewelcome(client->getNickname()));
		_server->updateEpollEvents(client_fd, EPOLLOUT, true);
	}
	if (command == "PING"){
		client->getMsg().queueMessage("PONG :localhost\r\n");
		_server->updateEpollEvents(client->getFd(), EPOLLOUT, true);
		return;
		//Client->set_failed_response_counter(-1);
		//resetClientTimer(Client->get_timer_fd(), config::TIMEOUT_CLIENT);
	}
	if (command == "PONG"){
		client->getMsg().queueMessage("PING :localhost\r\n");		
		_server->updateEpollEvents(client->getFd(), EPOLLOUT, true);
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

		std::cout<<"JOIN CAUGHT LETS HANDLE IT \n";
		if (!params[0].empty())
		{
			_server->handleJoinChannel(client, params);
		}
		else
		{
			if (client->getHasSentCap() == true)
			{
				if (!_server->get_channels_map().empty())
				{
        			for (auto it = _server->get_channels_map().begin(); it != _server->get_channels_map().end(); ++it)
        			{
        			    auto channel = it->second;
        			    std::string channelName = channel->getName();
        			    std::string topic = channel->getTopic();
        			    size_t userCount = channel->getClientCount(); // Or however you store this
					
        			    std::string listLine = ":localhost 322 " + client->getNickname() + " " +
        			                           channelName + " " +
        			                           std::to_string(userCount) + " :" +
        			                           "topic for channel = " + topic + "\r\n";
					
        			    client->getMsg().queueMessage(listLine);
        			}
				}
				client->getMsg().queueMessage(":localhost 323 " + client->getNickname() + " :End of channel list\r\n");
				_server->updateEpollEvents(client->getFd(), EPOLLOUT, true);

			}
		}
	}

	/**
	 * @brief 
	 * 
	 * sudo tcpdump -A -i any port <port number>, this will show what irssi sends before being reecived on
	 * _server, irc withe libera chat handles modes like +io +o user1 user2 but , i cant find a way to do that because
	 * irssi is sending the flags and users combined together into 1 string, where does flags end and user start? 
	 * 
	 * so no option but to not allow that format !
	 * 
	 */
	if (command == "MODE") {
		_server->handleModeCommand(client, params);
		// · i: Set/remove Invite-only channel
        //· t: Set/remove the restrictions of the TOPIC command to channel operators
        //· k: Set/remove the channel key (password)
		//· o: Give/take channel operator privilege
		//· l: Set/remove the user limit to channel
	}
	if (client->getMsg().getCommand() == "PRIVMSG")  {
		if (!params[0].empty()) // && 1 !empty
		{
			std::string contents = ":" + client->getNickname()  + " PRIVMSG " + params[0] + " " + params[1] +"\r\n";
			if (params[0][0] == '#')
			{
				if (_server->channelExists(params[0]) == true) {
					// MessageBuilder 
					// is client in channel 
					_server->broadcastMessage(contents, client,_server->get_Channel(params[0]), true, nullptr);
				}
			}
			else
			{
				int fd = _server->get_nickname_to_fd().find(params[0])->second;
				// check against end()
				if (fd < 0) {
					// no user found by name
					// no username provided
					std::cout<<"no user here by that name \n"; 
					return ;
				}
				_server->broadcastMessage(contents, client, nullptr, true, _server->get_Client(fd));				
			}
		}		
	}
	if (command == "WHOIS") {
		_server->handleWhoIs(client, params[0]);
	}
}