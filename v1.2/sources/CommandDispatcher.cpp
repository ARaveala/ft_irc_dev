// CommandDispatcher.cpp (or wherever dispatchCommand is implemented)

#include "CommandDispatcher.hpp"
#include "Server.hpp" // Ensure Server.hpp is included
#include "Client.hpp" // Ensure Client.hpp is included
#include "Channel.hpp" // Ensure Channel.hpp is included
#include "MessageBuilder.hpp" // Ensure MessageBuilder.hpp is included
#include <iostream>
#include <vector>
#include <string>
#include <memory> // For shared_ptr
#include <algorithm> // For std::transform
#include <cctype> // For ::tolower

// Constructor (if not already defined)
CommandDispatcher::CommandDispatcher(Server* server) : _server(server) {}

void CommandDispatcher::dispatchCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params)
{
    if (!client) {
        std::cerr << "Error: Client pointer is null in dispatchCommand()" << std::endl;
        return;
    }

    // For debugging: print the raw parsed message
    client->getMsg().printMessage(client->getMsg());

    std::string command = client->getMsg().getCommand();
    const std::string& nickname = client->getNickname(); // Get current nickname (could be default or set)

    // --- PHASE 1: Handle Commands Always Allowed (Pre-Registration) ---
    // These commands can be sent by any client, regardless of registration status.
    // They are typically used for establishing connection, initial capabilities, or disconnecting.

    if (command == "PASS") {
        _server->handlePassCommand(client, params);
        // IMPORTANT: If handlePassCommand decides to disconnect the client (e.g., wrong password or policy violation),
        // it sets the client's _quit flag. We immediately remove the client here.
        if (client->getQuit()) { // Check the _quit flag set by client->disconnect()
            _server->remove_Client(client->getFd()); // Force immediate removal
            return; // Stop processing for this client immediately
        }
        // If PASS was accepted, continue to registration attempt.
    }
    else if (command == "CAP") {
        // CAP command is often sent multiple times (LS, REQ, END). Handle it.
        _server->handleCapCommand(nickname, client->getMsg().getQue(), client->getHasSentCap());
    }
    else if (command == "NICK") {
        if (params.empty()) {
            client->sendMessage(":" + _server->getServerName() + " 461 " + nickname + " NICK :Not enough parameters");
            return;
        }
        _server->handleNickCommand(client, _server->get_fd_to_nickname(), _server->get_nickname_to_fd(), params[0]);
    }
    else if (command == "USER") {
        if (params.size() < 4) { // Changed to < 4 as per standard USER format
            client->sendMessage(":" + _server->getServerName() + " 461 " + nickname + " USER :Not enough parameters");
            return;
        }
        client->setClientUname(params[0]);
        std::string fullName = "";
        for (size_t i = 3; i < params.size(); ++i) { // Start from index 3 for realname
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
        return; // Client will be removed, stop further processing.
    }

    // --- PHASE 2: Attempt Client Registration ---
    // After processing any of the above commands, check if the client can now be registered.
    if (!client->getHasRegistered() && client->getHasSentNick() && client->getHasSentUser()) {
        client->setHasRegistered(); // This checks password too if required

        if (client->getHasRegistered()) {
            // Successfully registered, send welcome messages
            client->getMsg().queueMessage(MessageBuilder::buildWelcomeMessage(client->getNickname())); // Corrected call here
            // Add other welcome messages (002, 003, 004, MOTD) here
            // Example calls:
            // client->sendMessage(MessageBuilder::generateYourHost(_server->getServerName(), client->getNickname()));
            // client->sendMessage(MessageBuilder::generateCreated(_server->getServerName(), client->getNickname(), "some_creation_date"));
            // client->sendMessage(MessageBuilder::generateMyInfo(_server->getServerName(), client->getNickname(), "available_user_modes", "available_channel_modes"));
            // client->sendMessage(MessageBuilder::generateMOTDStart(client->getNickname(), _server->getServerName()));
            // client->sendMessage(MessageBuilder::generateMOTD("Welcome to ft_irc!", client->getNickname()));
            // client->sendMessage(MessageBuilder::generateMOTDEnd(client->getNickname()));

            _server->updateEpollEvents(client->getFd(), EPOLLOUT, true);
        } else {
            // Failed to register even after NICK/USER (implies password mismatch if required)
            // This condition specifically handles password failure if NICK/USER were already sent.
            if (client->getPasswordRequiredForRegistration() && !client->getIsAuthenticatedByPass()) {
                client->sendMessage(":" + _server->getServerName() + " 464 " + nickname + " :Password incorrect");
                _server->remove_Client(client->getFd()); // Remove on password mismatch
                return; // Client removed.
            }
            // If there are other reasons setHasRegistered returns false (e.g., bad NICK/USER logic *within* that func)
            // then it will fall through to the ERR_NOTREGISTERED check below.
        }
    }

    // --- PHASE 3: Enforce Registration for All Other Commands ---
    // If the client is NOT yet registered after PHASE 1 & 2, any other command is disallowed.
    if (!client->getHasRegistered()) {
        client->sendMessage(":" + _server->getServerName() + " 451 " + nickname + " :You have not registered");
        return; // Stop processing this command.
    }

    // --- PHASE 4: Dispatch Commands for Registered Clients Only ---
    // Only commands below this point should require the client to be registered.
    if (command == "KICK"){
        std::cout << "COMMAND DISPATCHER: " << command << " command recieved. Calling Server::handleKickCommand." << std::endl;
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
            auto it = _server->get_nickname_to_fd().find(IrcMessage::to_lowercase(target)); // Use IrcMessage::to_lowercase
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
