#include "Command.h"

/**
 * @brief Handles the PRIVMSG command.
 *
 * This function is responsible for handling the PRIVMSG command, which is used to send a private message to a user or a channel in an IRC server.
 * It checks if the client is registered, if the recipient and message body are provided, and if the recipient exists.
 * If the recipient is a user, it sends the message to the recipient's client.
 * If the recipient is a channel, it broadcasts the message to all users in the channel.
 * If the recipient does not exist, it sends an error response.
 *
 * @param msg The Message object containing the PRIVMSG command and its parameters.
 */

//https://tools.ietf.org/html/rfc2812#section-3.3.1
// PRIVMSG <msgtarget> <text to be sent>
// <msgtarget> = <to> | <channel>
// <to> = <nickname> | <servername>
// <channel> = "#" <channame> | "&" <servername> | "+" <servername> | "!" <servername>

// PRIVMSG #channel :Hello everyone!
// PRIVMSG user :Hello user!

void Command::handlePrivmsg(const Message &msg)
{
	std::shared_ptr<Client> client_ptr = msg.getClientPtr();
	if (!client_ptr)
	{
		std::cerr << ("null ptr in handlePrivmsg") << std::endl;
		return;
	}
    int client_fd = client_ptr->getFd();

	// Check if the client is registered, which is not necessary for the PRIVMSG command and can be removed
    if (!client_ptr->getRegisterStatus())
	{
        server_ptr_->sendResponse(client_fd, ERR_NOTREGISTERED(server_ptr_->getServerHostname()));
        return;
    }
	// Check if the recipient and message body are provided
    std::vector<std::string> parameters = msg.getParameters();
    if (parameters.empty())
	{
        server_ptr_->sendResponse(client_fd, ERR_NORECIPIENT(server_ptr_->getServerHostname(), client_ptr->getNickname(), "PRIVMSG"));
        return;
    }
	// Check if the recipient exists and is a user or a channel
    if (parameters.size() < 1)
	{
        server_ptr_->sendResponse(client_fd, ERR_NEEDMOREPARAMS(client_ptr->getClientPrefix(), "PRIVMSG"));
        return;
    }

    std::string recipient = parameters[0];         // Private message in IRC is like: <user> <message>
    std::string message_body = msg.getTrailer();   // The message body is the last parameter in the message

	if (message_body.empty())
	{
		server_ptr_->sendResponse(client_fd, ERR_NOTEXTTOSEND(server_ptr_->getServerHostname(), client_ptr->getClientPrefix()));	// No text to send
		return;
	}

    std::shared_ptr<Client> recipient_ptr = server_ptr_->findClientUsingNickname(recipient);
    if (recipient_ptr)
	{
		if (recipient_ptr->isAway()) // if the recipient is away from the server
            server_ptr_->sendResponse(client_fd, RPL_AWAY(server_ptr_->getServerHostname(), client_ptr->getNickname(), recipient_ptr->getNickname(), recipient_ptr->getAwayMessage()));
        server_ptr_->sendResponse(recipient_ptr->getFd(), RPL_PRIVMSG(client_ptr->getClientPrefix(), recipient_ptr->getNickname(), message_body)); // send the message to the recipient
        return;
    }

    std::shared_ptr<Channel> channel_ptr = server_ptr_->findChannel(recipient);
    if (channel_ptr)	// if the recipient is a channel
	{
        if (channel_ptr->getModeN() && !channel_ptr->isUserOnChannel(client_ptr->getNickname()))
		{
            server_ptr_->sendResponse(client_fd, ERR_CANNOTSENDTOCHAN(server_ptr_->getServerHostname(), client_ptr->getNickname(), recipient));
            return;
        }
        channel_ptr->broadcastMessage(client_ptr, RPL_PRIVMSG(client_ptr->getClientPrefix(), channel_ptr->getName(), message_body), server_ptr_); // broadcast the message to all users in the channel except the sender
        return;
    }

    server_ptr_->sendResponse(client_fd, ERR_NOSUCHNICK(server_ptr_->getServerHostname(), client_ptr->getNickname(), recipient));  // if the recipient does not exist in the server or the channel does not exist
}
