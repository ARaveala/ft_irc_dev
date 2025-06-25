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
	// command == "PASS" get the password and accept or dewcline based on getpassword()

	// if server doesn't require password, don't expect one
	// if client has connected one time, don't ask password
	// if client sends wrong password, reject connection and close the socket

	if (command == "CAP" && !client->getHasSentCap()) {
		_server->handleCapCommand(nickname, client->getMsg().getQue(), client->getHasSentCap());
	}
	if (command == "QUIT") {
		_server->handleQuit(client);
		return ;
	}

	if (command == "USER" && !client->getHasSentUser()) {
		client->getMsg().queueMessage("USER " + client->getClientUname() + " 0 * :" + client->getFullName() +"\r\n");
		client->setHasSentUser();
	}
	if (command == "NICK") {
		_server->handleNickCommand(client, _server->get_fd_to_nickname(), _server->get_nickname_to_fd(), params[0]);
		return ; 
	}

	// special pieces of code to check we can pass GO
	if (!client->getHasRegistered() && client->getHasSentNick() && client->getHasSentUser()) {
		client->setHasRegistered();
		client->getMsg().queueMessage(MessageBuilder::generatewelcome(client->getNickname()));
		_server->updateEpollEvents(client_fd, EPOLLOUT, true);
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

// CommandDispatcher.cpp (or wherever dispatchCommand is implemented)

// void CommandDispatcher::dispatchCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params)
// {
//     // int client_fd = client->getFd(); // Removed this line as client_fd is no longer needed directly
//     if (!client) {
//         std::cerr << "Error: Client pointer is null in dispatchCommand()" << std::endl;
//         return;
//     }

//     // For debugging: print the raw parsed message
//     client->getMsg().printMessage(client->getMsg());

//     std::string command = client->getMsg().getCommand();
//     const std::string& nickname = client->getNickname(); // Get current nickname (could be default or set)

//     // --- Core Registration Logic and Essential Commands (Order is Crucial) ---
//     // These commands can be sent by an unregistered client.

//     // 1. Handle PASS command: MUST be processed before NICK/USER if a password is required by the server.
//     //    If the password is wrong, handlePassCommand will disconnect the client.
//     if (command == "PASS") {
//         _server->handlePassCommand(client, params);
//         // After handlePassCommand, the client might have been disconnected (e.g., due to wrong password).
//         // Check if the client is still valid in the server's client map to prevent use-after-free or crashes.
//         // A more robust check might involve checking a client->isDisconnected() flag.
//         if (_server->get_Client(client->getFd()) == nullptr) { // Check if client still exists in server's map
//             return; // Client was disconnected, stop processing.
//         }
//     }
//     // 2. Handle CAP command (often sent before NICK/USER/PASS for capabilities negotiation)
//     else if (command == "CAP" && !client->getHasSentCap()) {
//         _server->handleCapCommand(nickname, client->getMsg().getQue(), client->getHasSentCap());
//     }
//     // 3. Handle NICK command: Essential for registration.
//     else if (command == "NICK") {
//         if (params.empty()) {
//             client->sendMessage(":" + _server->getServerName() + " 461 " + nickname + " NICK :Not enough parameters");
//             return;
//         }
//         _server->handleNickCommand(client, _server->get_fd_to_nickname(), _server->get_nickname_to_fd(), params[0]);
//         return ; // Nick command logic handled, exit this dispatch
//     }
//     // 4. Handle USER command: Essential for registration.
//     else if (command == "USER") {
//         // USER command parameters: <username> <mode> <unused> :<realname>
//         if (params.size() >= 4) {
//             client->setClientUname(params[0]); // Set username
//             // Combine remaining parameters for the full real name (after the colon for realname)
//             std::string fullName = "";
//             for (size_t i = 3; i < params.size(); ++i) {
//                 if (i > 3) fullName += " "; // Add space between parts of realname
//                 fullName += params[i];
//             }
//             client->setFullName(fullName); // Set full name (requires `setFullName` method in Client)
//             client->setHasSentUser(); // Mark that USER command has been sent
//         } else {
//              client->sendMessage(":" + _server->getServerName() + " 461 " + nickname + " USER :Not enough parameters");
//         }
//     }
//     // 5. Handle PING/PONG: Can happen at any time to check connection.
//     else if (command == "PING"){
//         _server->handlePing(client);
//         return; // PING command logic handled, exit this dispatch
//     }
//     else if (command == "PONG"){
//         _server->handlePong(client);
//         return; // PONG command logic handled, exit this dispatch
//     }
//     // 6. Handle QUIT: Allows a client to disconnect at any time.
//     else if (command == "QUIT") {
//         _server->handleQuit(client);
//         return; // Client will be removed, stop further processing for this client.
//     }
//     // --- End Core Registration Logic for Unregistered Clients ---

//     // --- Attempt Client Registration (after any of the above commands have been processed) ---
//     // This block tries to fully register the client if all necessary registration commands
//     // (NICK, USER, and optionally PASS) have been sent and validated.
//     // This `if` block is crucial for ensuring the client moves from un-registered to registered state.
//     if (!client->getHasRegistered() && client->getHasSentNick() && client->getHasSentUser()) {
//         // Call setHasRegistered. This method internally checks `getPasswordRequiredForRegistration`
//         // and `getIsAuthenticatedByPass` to determine if full registration can proceed.
//         client->setHasRegistered();

//         if (client->getHasRegistered()) { // Check if setHasRegistered successfully registered the client
//             // If successfully registered, send welcome messages (RPL_WELCOME, etc.)
//             client->getMsg().queueMessage(MessageBuilder::generatewelcome(client->getNickname()));
//             // Example of other welcome messages (you might need to implement these in MessageBuilder):
//             // current_datetime should be replaced with a real timestamp
//             // client->getMsg().queueMessage(MessageBuilder::generateYourHost(_server->getServerName(), client->getNickname()));
//             // client->getMsg().queueMessage(MessageBuilder::generateCreated(_server->getServerName(), client->getNickname(), "current_datetime"));
//             // client->getMsg().queueMessage(MessageBuilder::generateMyInfo(_server->getServerName(), client->getNickname(), "your_user_modes", "your_channel_modes"));
//             // client->getMsg().queueMessage(MessageBuilder::generateMOTDStart(client->getNickname(), _server->getServerName()));
//             // client->getMsg().queueMessage(MessageBuilder::generateMOTD("Welcome to our awesome ft_irc server!", client->getNickname()));
//             // client->getMsg().queueMessage(MessageBuilder::generateMOTDEnd(client->getNickname()));

//             _server->updateEpollEvents(client->getFd(), EPOLLOUT, true); // Use client->getFd() directly
//         } else {
//             // If setHasRegistered did *not* register the client, it means something is still missing.
//             // In the context of NICK and USER already being sent, this almost certainly means the password
//             // was required but either not sent or incorrect.
//             if (client->getPasswordRequiredForRegistration() && !client->getIsAuthenticatedByPass()) {
//                 client->sendMessage(":" + _server->getServerName() + " 464 " + nickname + " :Password incorrect");
//                 _server->remove_Client(client->getFd()); // Disconnect on password mismatch during final registration check
//                 return; // Client removed, stop processing
//             }
//             // If there's another reason for not registering (e.g., NICK/USER logic issues),
//             // subsequent commands will hit the ERR_NOTREGISTERED check below.
//         }
//     }

//     // --- Enforce Registration for All Other Commands ---
//     // If the client is NOT yet registered, they are not allowed to send most IRC commands.
//     // Only the essential registration-related commands and PING/PONG/QUIT are allowed.
//     if (!client->getHasRegistered()) {
//         // Check if the current command is one of the allowed registration/essential commands.
//         // If not, send ERR_NOTREGISTERED (451) and stop processing.
//         if (command != "PASS" && command != "NICK" && command != "USER" &&
//             command != "CAP" && command != "PING" && command != "PONG" && command != "QUIT") {
//             client->sendMessage(":" + _server->getServerName() + " 451 " + nickname + " :You have not registered");
//             return; // Stop processing this command as the client is not registered.
//         }
//     }

//     // --- Dispatch Other Commands (only if the client is fully registered) ---
//     // If the code reaches here, the client is either successfully registered,
//     // or is in the process of registration (and the command was one of the allowed ones above).
//     // The previous `if (!client->getHasRegistered())` block ensures that only
//     // allowed commands proceed for unregistered clients.
//     // Therefore, if we hit any of these commands, the client MUST be registered.

//     if (command == "KICK"){
//         std::cout << "COMMAND DISPATCHER: " << command << " command recieved. Calling Server::handleKickCommand." << std::endl;
//         _server->handleKickCommand(client, params);
//         return;
//     }
//     if (command == "LEAVE" || command == "PART"){
//         std::cout << "COMMAND DISPATCHER: " << command << " command received. Calling Server::handlePartCommand.\n";
//         _server->handlePartCommand(client, params);
//         return;
//     }
//     if (command == "TOPIC"){
//         std::cout << "COMMAND DISPATCHER: " << command << " command received. Calling Server::handleTopicCommand.\n";
//         _server->handleTopicCommand(client, params);
//         return;
//     }
//     if (command == "INVITE"){
//         std::cout << "COMMAND DISPATCHER: " << command << " command received. Calling Server::handleInviteCommand.\n";
//         _server->handleInviteCommand(client, params);
//         return;
//     }
//     if (command == "JOIN"){

//         std::cout<<"JOIN CAUGHT LETS HANDLE IT \n";
//         if (!params[0].empty())
//         {
//             _server->handleJoinChannel(client, params);
//         }
//         else
//         {
//             if (client->getHasSentCap() == true) // This condition might be too strict for JOIN alone, usually JOIN is for listing channels if no param
//             {
//                 if (!_server->get_channels_map().empty())
//                 {
//                     for (auto it = _server->get_channels_map().begin(); it != _server->get_channels_map().end(); ++it)
//                     {
//                         auto channel = it->second;
//                         std::string channelName = channel->getName();
//                         std::string topic = channel->getTopic();
//                         size_t userCount = channel->getClientCount(); // Or however you store this

//                         std::string listLine = ":localhost 322 " + client->getNickname() + " " +
//                                                channelName + " " +
//                                                std::to_string(userCount) + " :" +
//                                                "topic for channel = " + topic + "\r\n";

//                         client->getMsg().queueMessage(listLine);
//                     }
//                 }
//                 client->getMsg().queueMessage(":localhost 323 " + client->getNickname() + " :End of channel list\r\n");
//                 _server->updateEpollEvents(client->getFd(), EPOLLOUT, true);

//             }
//         }
//     }
//     // Add other commands as needed below this point.
//     // For example: LIST, LUSERS, NAMES, etc.
//     // else if (command == "PRIVMSG") {
//     //     _server->handlePrivMsgCommand(client, params);
//     // }
//     // You might also need a default "unknown command" response.
//     // For example:
//     // else {
//     //     client->sendMessage(":" + _server->getServerName() + " 421 " + client->getNickname() + " " + command + " :Unknown command");
//     // }


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

// void CommandDispatcher::dispatchCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params)
// {
//     if (!client) {
//         std::cerr << "Error: Client pointer is null in dispatchCommand()" << std::endl;
//         return;
//     }

//     // For debugging: print the raw parsed message
//     client->getMsg().printMessage(client->getMsg());

//     std::string command = client->getMsg().getCommand();
//     const std::string& nickname = client->getNickname(); // Get current nickname (could be default or set)

//     // --- PHASE 1: Handle Commands Always Allowed (Pre-Registration) ---
//     // These commands can be sent by any client, regardless of registration status.
//     // They are typically used for establishing connection, initial capabilities, or disconnecting.

//     if (command == "PASS") {
//         _server->handlePassCommand(client, params);
//         // After handlePassCommand, the client might have been disconnected (e.g., due to wrong password).
//         // Check if the client is still valid in the server's client map to prevent use-after-free or crashes.
//         if (_server->get_Client(client->getFd()) == nullptr) {
//             return; // Client was disconnected, stop processing.
//         }
//     }
//     else if (command == "CAP") {
//         // CAP command is often sent multiple times (LS, REQ, END). Handle it if not already sent.
//         _server->handleCapCommand(nickname, client->getMsg().getQue(), client->getHasSentCap());
//     }
//     else if (command == "NICK") {
//         if (params.empty()) {
//             client->sendMessage(":" + _server->getServerName() + " 461 " + nickname + " NICK :Not enough parameters");
//             return;
//         }
//         _server->handleNickCommand(client, _server->get_fd_to_nickname(), _server->get_nickname_to_fd(), params[0]);
//     }
//     else if (command == "USER") {
//         if (params.size() >= 4) {
//             client->setClientUname(params[0]);
//             std::string fullName = "";
//             for (size_t i = 3; i < params.size(); ++i) {
//                 if (i > 3) fullName += " ";
//                 fullName += params[i];
//             }
//             client->setFullName(fullName);
//             client->setHasSentUser();
//         } else {
//              client->sendMessage(":" + _server->getServerName() + " 461 " + nickname + " USER :Not enough parameters");
//         }
//     }
//     else if (command == "PING"){
//         _server->handlePing(client);
//     }
//     else if (command == "PONG"){
//         _server->handlePong(client);
//     }
//     else if (command == "QUIT") {
//         _server->handleQuit(client);
//         return; // Client will be removed, stop further processing.
//     }

//     // --- PHASE 2: Attempt Client Registration ---
//     // After processing any of the above commands, check if the client can now be registered.
//     // This check should only trigger the registration process once.
//     if (!client->getHasRegistered() && client->getHasSentNick() && client->getHasSentUser()) {
//         client->setHasRegistered(); // This checks password too if required

//         if (client->getHasRegistered()) {
//             // Successfully registered, send welcome messages
//             client->getMsg().queueMessage(MessageBuilder::generatewelcome(client->getNickname()));
//             // Add other welcome messages (002, 003, 004, MOTD) here
//             // Example calls:
//             // client->getMsg().queueMessage(MessageBuilder::generateYourHost(_server->getServerName(), client->getNickname()));
//             // client->getMsg().queueMessage(MessageBuilder::generateCreated(_server->getServerName(), client->getNickname(), "some_creation_date"));
//             // client->getMsg().queueMessage(MessageBuilder::generateMyInfo(_server->getServerName(), client->getNickname(), "available_user_modes", "available_channel_modes"));
//             // client->getMsg().queueMessage(MessageBuilder::generateMOTDStart(client->getNickname(), _server->getServerName()));
//             // client->getMsg().queueMessage(MessageBuilder::generateMOTD("Welcome to ft_irc!", client->getNickname()));
//             // client->getMsg().queueMessage(MessageBuilder::generateMOTDEnd(client->getNickname()));

//             _server->updateEpollEvents(client->getFd(), EPOLLOUT, true);
//         } else {
//             // Failed to register even after NICK/USER (implies password mismatch if required)
//             if (client->getPasswordRequiredForRegistration() && !client->getIsAuthenticatedByPass()) {
//                 client->sendMessage(":" + _server->getServerName() + " 464 " + nickname + " :Password incorrect");
//                 _server->remove_Client(client->getFd());
//                 return; // Client removed.
//             }
//             // If there's another reason for not registering (e.g., NICK/USER validation problems),
//             // it will be caught by the ERR_NOTREGISTERED check below.
//         }
//     }

//     // --- PHASE 3: Enforce Registration for All Other Commands ---
//     // If the client is NOT yet registered after PHASE 1 & 2, any other command is disallowed.
//     if (!client->getHasRegistered()) {
//         client->sendMessage(":" + _server->getServerName() + " 451 " + nickname + " :You have not registered");
//         return; // Stop processing this command.
//     }

//     // --- PHASE 4: Dispatch Commands for Registered Clients Only ---
//     // Only commands below this point should require the client to be registered.
//     if (command == "KICK"){
//         std::cout << "COMMAND DISPATCHER: " << command << " command recieved. Calling Server::handleKickCommand." << std::endl;
//         _server->handleKickCommand(client, params);
//     }
//     else if (command == "LEAVE" || command == "PART"){
//         std::cout << "COMMAND DISPATCHER: " << command << " command received. Calling Server::handlePartCommand.\n";
//         _server->handlePartCommand(client, params);
//     }
//     else if (command == "TOPIC"){
//         std::cout << "COMMAND DISPATCHER: " << command << " command received. Calling Server::handleTopicCommand.\n";
//         _server->handleTopicCommand(client, params);
//     }
//     else if (command == "INVITE"){
//         std::cout << "COMMAND DISPATCHER: " << command << " command received. Calling Server::handleInviteCommand.\n";
//         _server->handleInviteCommand(client, params);
//     }
//     else if (command == "JOIN"){
//         std::cout<<"JOIN CAUGHT LETS HANDLE IT \n";
//         if (!params[0].empty()) {
//             _server->handleJoinChannel(client, params);
//         } else { // JOIN without parameters means list channels (RFC 2812, section 3.2.1)
//             if (!_server->get_channels_map().empty()) {
//                 for (auto it = _server->get_channels_map().begin(); it != _server->get_channels_map().end(); ++it) {
//                     auto channel = it->second;
//                     std::string channelName = channel->getName();
//                     std::string topic = channel->getTopic();
//                     size_t userCount = channel->getClientCount();

//                     std::string listLine = ":" + _server->getServerName() + " 322 " + client->getNickname() + " " +
//                                            channelName + " " +
//                                            std::to_string(userCount) + " :" +
//                                            "topic for channel = " + topic; // No \r\n here, sendMessage adds it

//                     client->sendMessage(listLine); // Use sendMessage for consistency
//                 }
//             }
//             client->sendMessage(":" + _server->getServerName() + " 323 " + client->getNickname() + " :End of channel list"); // Use sendMessage
//         }
//     }
//     else if (command == "MODE") {
//         _server->handleModeCommand(client, params);
//     }
//     else if (command == "PRIVMSG")  {
//         if (params.size() < 2 || params[0].empty() || params[1].empty()) {
//              client->sendMessage(":" + _server->getServerName() + " 461 " + nickname + " PRIVMSG :Not enough parameters");
//              return;
//         }

//         std::string target = params[0];
//         std::string message_content_part = params[1];

//         if (target[0] == '#') {
//             std::shared_ptr<Channel> channel = _server->get_Channel(target);
//             if (!channel) {
//                 client->sendMessage(":" + _server->getServerName() + " 403 " + client->getNickname() + " " + target + " :No such channel");
//                 return;
//             }
//             if (!channel->isClientInChannel(client->getNickname())) {
//                 client->sendMessage(":" + _server->getServerName() + " 404 " + client->getNickname() + " " + target + " :Cannot send to channel");
//                 return;
//             }
//             std::string full_msg_to_broadcast = ":" + client->getNickname() + " PRIVMSG " + target + " :" + message_content_part;
//             _server->broadcastMessage(full_msg_to_broadcast, client, channel, true, nullptr); // Skip sender
//         }
//         else { // Assume it's a user
//             auto it = _server->get_nickname_to_fd().find(toLower(target));
//             if (it == _server->get_nickname_to_fd().end()) {
//                 client->sendMessage(":" + _server->getServerName() + " 401 " + client->getNickname() + " " + target + " :No such nick/channel");
//                 return ;
//             }
//             std::shared_ptr<Client> target_client = _server->get_Client(it->second);
//             if (!target_client) {
//                 client->sendMessage(":" + _server->getServerName() + " 401 " + client->getNickname() + " " + target + " :No such nick/channel");
//                 return ;
//             }
//             std::string full_msg_to_send = ":" + client->getNickname() + " PRIVMSG " + target + " :" + message_content_part;
//             target_client->sendMessage(full_msg_to_send);
//         }
//     }
//     else if (command == "WHOIS") {
//         if (params.empty()) {
//             client->sendMessage(":" + _server->getServerName() + " 461 " + nickname + " WHOIS :Not enough parameters");
//             return;
//         }
//         _server->handleWhoIs(client, params[0]);
//     }
//     else { // Unknown command for registered clients
//         client->sendMessage(":" + _server->getServerName() + " 421 " + client->getNickname() + " " + command + " :Unknown command");
//     }
// }