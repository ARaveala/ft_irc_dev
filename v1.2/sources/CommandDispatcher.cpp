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

// ... (existing includes and other CommandDispatcher methods) ...

void CommandDispatcher::dispatchCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params)
{
    if (!client) {
        std::cerr << "Error: Client pointer is null in dispatchCommand()" << std::endl;
        return;
    }

    // For debugging: print the raw parsed message
    // client->getMsg().printMessage(client->getMsg()); // Keep this if you want debug logs

    std::string command = client->getMsg().getCommand();
    const std::string& nickname = client->getNickname(); // Get current nickname

    // --- PHASE 1: Handle Commands Always Allowed (Pre-Registration) ---
    // These commands are processed regardless of registration status.
    // NICK, USER, CAP, PING, PONG do NOT return here, allowing flow to PHASE 2 for registration attempt.
    // PASS and QUIT will explicitly return if they result in client disconnection.

    if (command == "PASS") {
        _server->handlePassCommand(client, params);
        // If handlePassCommand disconnected the client (e.g., wrong password/policy), stop.
        if (client->getQuit()) {
            _server->remove_Client(client->getFd());
            return;
        }
    }
    else if (command == "CAP") {
        _server->handleCapCommand(nickname, client->getMsg().getQue(), client->getHasSentCap());
    }
    else if (command == "NICK") {
        if (params.empty()) {
            client->sendMessage(":" + _server->getServerName() + " 461 " + nickname + " NICK :Not enough parameters");
            return; // Stop if not enough parameters for NICK
        }
        // handleNickCommand does not disconnect, so flow continues to PHASE 2
        _server->handleNickCommand(client, _server->get_fd_to_nickname(), _server->get_nickname_to_fd(), params[0]);
    }
    else if (command == "USER") {
        if (params.size() < 4) { // Standard USER format: <username> <mode> <unused> :<realname>
            client->sendMessage(":" + _server->getServerName() + " 461 " + nickname + " USER :Not enough parameters");
            return; // Stop if not enough parameters for USER
        }
        client->setClientUname(params[0]);
        std::string fullName = "";
        for (size_t i = 3; i < params.size(); ++i) { // Combine realname parts from param 3 onwards
            if (i > 3) fullName += " ";
            fullName += params[i];
        }
        client->setFullName(fullName);
        client->setHasSentUser();
    }
    else if (command == "PING"){
        _server->handlePing(client);
    }
    else if (command == "PONG"){
        _server->handlePong(client);
    }
    else if (command == "QUIT") {
        _server->handleQuit(client);
        return; // QUIT always results in client removal, so exit immediately.
    }

    // --- PHASE 2: Attempt Client Registration ---
    // This attempts to register the client if NICK, USER (and correct PASS if required) are met.
    if (!client->getHasRegistered() && client->getHasSentNick() && client->getHasSentUser()) {
        client->setHasRegistered(); // This method itself checks password requirements

        if (client->getHasRegistered()) {
            // Successfully registered! Send welcome messages.
            client->getMsg().queueMessage(MessageBuilder::generatewelcome(client->getNickname()));
            // Add other required welcome messages (e.g., 002, 003, 004, MOTD)
            // client->sendMessage(MessageBuilder::generateYourHost(...));
            // client->sendMessage(MessageBuilder::generateCreated(...));
            // client->sendMessage(MessageBuilder::generateMyInfo(...));
            // client->sendMessage(MessageBuilder::generateMOTDStart(...));
            // client->sendMessage(MessageBuilder::generateMOTD(...));
            // client->sendMessage(MessageBuilder::generateMOTDEnd(...));

            _server->updateEpollEvents(client->getFd(), EPOLLOUT, true); // Ensure messages are sent
        } else {
            // Registration failed after NICK/USER were sent (implies password issue if required)
            if (client->getPasswordRequiredForRegistration() && !client->getIsAuthenticatedByPass()) {
                client->sendMessage(":" + _server->getServerName() + " 464 " + nickname + " :Password incorrect");
                _server->remove_Client(client->getFd()); // Disconnect on password mismatch
                return; // Client removed.
            }
        }
    }

    // --- PHASE 3: Enforce Registration for All Other Commands ---
    // If the client is NOT fully registered at this point,
    // any command not explicitly handled in PHASE 1 is disallowed.
    if (!client->getHasRegistered()) {
        client->sendMessage(":" + _server->getServerName() + " 451 " + nickname + " :You have not registered");
        return; // Stop processing this command as the client is not registered.
    }

    // --- PHASE 4: Dispatch Commands for Registered Clients Only ---
    // All commands in this block require the client to be fully registered.
    // Use 'else if' to ensure only one command is dispatched per event.
    if (command == "KICK"){
        std::cout << "COMMAND DISPATCHER: " << command << " command received. Calling Server::handleKickCommand." << std::endl;
        _server->handleKickCommand(client, params);
    }
    else if (command == "LEAVE" || command == "PART"){
        std::cout << "COMMAND DISPATCHER: " << command << " command received. Calling Server::handlePartCommand.\n";
        _server->handlePartCommand(client, params);
    }
    else if (command == "TOPIC"){
        std::cout << "COMMAND DISPATCHER: " << command << " command received. Calling Server::handleTopicCommand.\n";
        _server->handleTopicCommand(client, params);
    }
    else if (command == "INVITE"){
        std::cout << "COMMAND DISPATCHER: " << command << " command received. Calling Server::handleInviteCommand.\n";
        _server->handleInviteCommand(client, params);
    }
    else if (command == "JOIN"){
        std::cout<<"JOIN CAUGHT LETS HANDLE IT \n";
        if (!params[0].empty()) {
            _server->handleJoinChannel(client, params);
        } else { // JOIN without parameters means list channels (RFC 2812, section 3.2.1)
            if (!_server->get_channels_map().empty()) {
                for (auto it = _server->get_channels_map().begin(); it != _server->get_channels_map().end(); ++it) {
                    auto channel = it->second;
                    std::string channelName = channel->getName();
                    std::string topic = channel->getTopic();
                    size_t userCount = channel->getClientCount();

                    std::string listLine = ":" + _server->getServerName() + " 322 " + client->getNickname() + " " +
                                           channelName + " " +
                                           std::to_string(userCount) + " :" +
                                           "topic for channel = " + topic;
                    client->sendMessage(listLine);
                }
            }
            client->sendMessage(":" + _server->getServerName() + " 323 " + client->getNickname() + " :End of channel list");
        }
    }
    else if (command == "MODE") {
        _server->handleModeCommand(client, params);
    }
    else if (command == "PRIVMSG")  {
        if (params.size() < 2 || params[0].empty() || params[1].empty()) {
             client->sendMessage(":" + _server->getServerName() + " 461 " + nickname + " PRIVMSG :Not enough parameters");
             return;
        }

        std::string target = params[0];
        std::string message_content_part = params[1];

        if (target[0] == '#') {
            std::shared_ptr<Channel> channel = _server->get_Channel(target);
            if (!channel) {
                client->sendMessage(":" + _server->getServerName() + " 403 " + client->getNickname() + " " + target + " :No such channel");
                return;
            }
            if (!channel->isClientInChannel(client->getNickname())) {
                client->sendMessage(":" + _server->getServerName() + " 404 " + client->getNickname() + " " + target + " :Cannot send to channel");
                return;
            }
            std::string full_msg_to_broadcast = ":" + client->getNickname() + " PRIVMSG " + target + " :" + message_content_part;
            _server->broadcastMessage(full_msg_to_broadcast, client, channel, true, nullptr);
        }
        else { // Assume it's a user
            auto it = _server->get_nickname_to_fd().find(toLower(target));
            if (it == _server->get_nickname_to_fd().end()) {
                client->sendMessage(":" + _server->getServerName() + " 401 " + client->getNickname() + " " + target + " :No such nick/channel");
                return ;
            }
            std::shared_ptr<Client> target_client = _server->get_Client(it->second);
            if (!target_client) {
                client->sendMessage(":" + _server->getServerName() + " 401 " + client->getNickname() + " " + target + " :No such nick/channel");
                return ;
            }
            std::string full_msg_to_send = ":" + client->getNickname() + " PRIVMSG " + target + " :" + message_content_part;
            target_client->sendMessage(full_msg_to_send);
        }
    }
    else if (command == "WHOIS") {
        if (params.empty()) {
            client->sendMessage(":" + _server->getServerName() + " 461 " + nickname + " WHOIS :Not enough parameters");
            return;
        }
        _server->handleWhoIs(client, params[0]);
    }
    else { // Unknown command for registered clients
        client->sendMessage(":" + _server->getServerName() + " 421 " + client->getNickname() + " " + command + " :Unknown command");
    }
}




// This is the roll back version 
// void CommandDispatcher::dispatchCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params)
// {
//     // ... (initial client null check, debug prints) ...
// 	int client_fd = client->getFd();
//     if (!client) {
//         std::cerr << "Error: Client pointer is null in dispatchCommand()" << std::endl;
//         return;
//     }
// 	client->getMsg().printMessage(client->getMsg());
// 	std::string command = client->getMsg().getCommand();
// 	const std::string& nickname = client->getNickname();
	
//     // --- PHASE 1: Handle Commands Always Allowed (Pre-Registration) ---
// 	// command == "PASS" get the password and accept or dewcline based on getpassword()
//     if (command == "PASS") {
//         _server->handlePassCommand(client, params);
//         // IMPORTANT: If handlePassCommand decides to disconnect the client (e.g., wrong password or policy violation),
//         // it sets the client's _quit flag. We immediately remove the client here.
//         if (client->getQuit()) { // Check the _quit flag set by client->disconnect()
//             _server->remove_Client(client->getFd()); // Force immediate removal
//             return; // Stop processing for this client immediately
//         }
//         // If PASS was accepted, continue to registration attempt (Phase 2).
//     }
//     // ... (rest of existing Phase 1 commands: CAP, NICK, USER, PING, PONG, QUIT) ...

//     // --- PHASE 2: Attempt Client Registration ---
//     // ... (existing code for this phase) ...

//     // --- PHASE 3: Enforce Registration for All Other Commands ---
//     // ... (existing code for this phase) ...

//     // --- PHASE 4: Dispatch Commands for Registered Clients Only ---
//     // ... (existing code for this phase) ...

// 	if (command == "CAP" && !client->getHasSentCap()) {
// 		_server->handleCapCommand(nickname, client->getMsg().getQue(), client->getHasSentCap());
// 	}
// 	if (command == "QUIT") {
// 		_server->handleQuit(client);
// 		return ;
// 	}

// 	if (command == "USER" && !client->getHasSentUser()) {
// 		client->setClientUname(params[0]);
// 		client->setRealname(params[3]);
// 		client->setHasSentUser();
// 	}
// 	if (command == "NICK") {
// 		_server->handleNickCommand(client, _server->get_nickname_to_fd(), params[0]);
// 	}
// 	if (!client->getHasRegistered() && client->getHasSentNick() && client->getHasSentUser()) {
// 		client->setHasRegistered();
// 		client->getMsg().queueMessage(MessageBuilder::generatewelcome(client->getNickname()));
// 		_server->updateEpollEvents(client_fd, EPOLLOUT, true);
// 	}
// 	if (command == "PING"){
// 		_server->handlePing(client);
// 		return;
// 	}
// 	if (command == "PONG"){
// 		_server->handlePong(client);
// 		return;

// 	}
// 	if (command == "KICK"){
// 		std::cout << "COMMAND DISPATCHER: " << command << " command recieved. Calling Server::handleKickCommand." << std::endl;
// 		_server->handleKickCommand(client, params);
// 		return;
// 	}
// 	if (command == "LEAVE" || command == "PART"){
// 		std::cout << "COMMAND DISPATCHER: " << command << " command received. Calling Server::handlePartCommand.\n";
//         _server->handlePartCommand(client, params);
//         return;
// 	}

// 	if (command == "TOPIC"){
// 		std::cout << "COMMAND DISPATCHER: " << command << " command received. Calling Server::handleTopicCommand.\n";
//         _server->handleTopicCommand(client, params);
// 		return;
// 	}

// 	if (command == "INVITE"){
// 		std::cout << "COMMAND DISPATCHER: " << command << " command received. Calling Server::handleInviteCommand.\n";
//         _server->handleInviteCommand(client, params);
// 		return;
// 	}

//     if (command == "JOIN"){

// 		std::cout<<"JOIN CAUGHT LETS HANDLE IT \n";
// 		if (!params[0].empty())
// 		{
// 			_server->handleJoinChannel(client, params);
// 		}
// 		else
// 		{
// 			if (client->getHasSentCap() == true)
// 			{
// 				if (!_server->get_channels_map().empty())
// 				{
//         			for (auto it = _server->get_channels_map().begin(); it != _server->get_channels_map().end(); ++it)
//         			{
//         			    auto channel = it->second;
//         			    std::string channelName = channel->getName();
//         			    std::string topic = channel->getTopic();
//         			    size_t userCount = channel->getClientCount(); // Or however you store this
					
//         			    std::string listLine = ":localhost 322 " + client->getNickname() + " " +
//         			                           channelName + " " +
//         			                           std::to_string(userCount) + " :" +
//         			                           "topic for channel = " + topic + "\r\n";
					
//         			    client->getMsg().queueMessage(listLine);
//         			}
// 				}
// 				client->getMsg().queueMessage(":localhost 323 " + client->getNickname() + " :End of channel list\r\n");
// 				_server->updateEpollEvents(client->getFd(), EPOLLOUT, true);

// 			}
// 		}
// 	}

// 	/**
// 	 * @brief 
// 	 * 
// 	 * sudo tcpdump -A -i any port <port number>, this will show what irssi sends before being reecived on
// 	 * _server, irc withe libera chat handles modes like +io +o user1 user2 but , i cant find a way to do that because
// 	 * irssi is sending the flags and users combined together into 1 string, where does flags end and user start? 
// 	 * 
// 	 * so no option but to not allow that format !
// 	 * 
// 	 */
// 	if (command == "MODE") {
// 		_server->handleModeCommand(client, params);
// 	}
// 	if (client->getMsg().getCommand() == "PRIVMSG")  {
// 		if (!params[0].empty()) // && 1 !empty
// 		{
// 			std::string contents = ":" + client->getNickname()  + " PRIVMSG " + params[0] + " " + params[1] +"\r\n";
// 			if (params[0][0] == '#')
// 			{
// 				if (!_server->validateChannelExists(client, params[0], client->getNickname())) { return;}
// 				// is client in channel 
// 				_server->broadcastMessage(contents, client,_server->get_Channel(params[0]), true, nullptr);
// 			}
// 			else
// 			{
// 				int fd = _server->get_nickname_to_fd().find(params[0])->second;
// 				// check against end()
// 				if (fd < 0) {
// 					// no user found by name
// 					// no username provided
// 					std::cout<<"no user here by that name \n"; 
// 					return ;
// 				}
// 				_server->broadcastMessage(contents, client, nullptr, true, _server->get_Client(fd));				
// 			}
// 		}		
// 	}
// 	if (command == "WHOIS") {
// 		_server->handleWhoIs(client, params[0]);
// 	}
// }