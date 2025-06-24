/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Message.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jwadding <jwadding@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/08 09:49:42 by akuburas          #+#    #+#             */
/*   Updated: 2025/04/23 19:14:49 by jwadding         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* ****************************************************************************/
/*  ROFL:ROFL:ROFL:ROFL 													  */
/*          _^___      										 				  */
/* L     __/   [] \    										 			      */
/* LOL===__        \   			MY ROFLCOPTER GOES BRRRRRR				  	  */
/* L      \________]  					by fdessoy-				  			  */
/*         I   I     			(fdessoy-@student.hive.fi)				  	  */
/*        --------/   										  				  */
/* ****************************************************************************/

# include "../inc/Message.hpp"

// Constructor
Message::Message(std::string rawMessage)
{
	parseIncommingMessage(rawMessage);
}

void Message::parseIncommingMessage(std::string rawMessage)
{
	// Check for CRLF termination
	if (rawMessage.size() < 2 || rawMessage.substr(rawMessage.size() - 2) != "\r\n") {
		throw std::invalid_argument("Invalid message format (no CRLF at the end)");
	}
	rawMessage = rawMessage.substr(0, rawMessage.size() - 2);

	// Parse sections
	size_t pos = 0;
	// Parse prefix
	if(rawMessage[0] == ':')
	{
		size_t spacePos = rawMessage.find(' ');
		if(spacePos == std::string::npos)
			throw std::invalid_argument("Invalid message format (no space after prefix)");
		prefix_ =  rawMessage.substr(1, spacePos - 1);
		pos = spacePos + 1;
	}

	// Parse command
	size_t spacePos = rawMessage.find(' ', pos);
	std::cout << "Command:\n";
	if(spacePos == std::string::npos)
	{
		std::cout << "Only Command:\n";
		command_ = rawMessage.substr(pos);
		return;
	}
	command_ = rawMessage.substr(pos, spacePos - pos);
	pos = spacePos + 1;

	// Parse params and body
	size_t colonPos = rawMessage.find(':', pos);
	if (colonPos != std::string::npos) {
		params_ = rawMessage.substr(pos, colonPos - pos - 1);
		suffix_ = rawMessage.substr(colonPos + 1);
	} else {
		params_ = rawMessage.substr(pos);
	}

}

std::string Message::serialize()
{
	std::string message;

	if (!prefix_.empty())
		message += ":" + prefix_ + " ";

	if (!command_.empty())
		message += command_;

	if (!params_.empty())
		message += " " + params_;

	if (!suffix_.empty())
		message += " :" + suffix_;

	message += "\r\n";
	return message;
}

// Getters
std::string Message::getPrefix() const
{
	return (prefix_);
}

std::string Message::getCommand() const
{
	return (command_);
}

std::string Message::getParams() const
{
	return (params_);
}

std::string Message::getSuffix() const
{
	return (suffix_);
}

std::string Message::getSender() const
{
	return (sender_);
}

std::string Message::getRecipient() const
{
	return (recipient_);
}

