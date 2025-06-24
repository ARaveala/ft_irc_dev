#include "Message.h"

/// @brief constructor for receiving a message from client and getting ready to parse it
/// @param raw_message
/// @param server
/// @param clientfd
Message::Message(std::string raw_message, Server *server, int clientfd)
	: raw_message_(raw_message), has_trailer_(false), server_ptr_(server), client_fd_(clientfd), valid_message_(false)
{
	client_ptr_ = server_ptr_->findClientUsingFd(client_fd_);
	if (!client_ptr_)
	{
		debug("Find client in message constructor", FAILED);
		return;
	}
	std::cout << CYAN << "Server received: " << raw_message << "\t"
			  << "Message size = " << raw_message.length() << " byte" << RESET << std::endl;
	valid_message_ = analyzeMessage();
}

Message::~Message()
{
}

bool Message::analyzeMessage()
{
	std::istringstream iss(raw_message_);
	std::string prefix;

	if (raw_message_.front() == ':')
	{
		std::getline(iss, prefix, ' '); // Extract prefix up to the first space
		std::string temp_prefix = ":" + client_ptr_->getNickname();
		if (prefix != temp_prefix)
		{
			std::cerr << "Invalid prefix: " << prefix << std::endl;
			return false;
		}
		prefix_ = prefix;
	}

	std::string command, param;

	iss >> command;
	command_ = command;
	while (iss >> param)
	{
		if (param.front() == ':')
		{
			has_trailer_ = true;
			trailer_ = param.substr(1); // Remove the colon
			break;
		}
		parameters_.push_back(param);
	}

	// Read the rest of the line for the trailer
	if (has_trailer_)
	{
		std::string remaining;
		std::getline(iss, remaining);
		if (!remaining.empty())
			trailer_ += remaining;
	}
	return true;
}

std::string const &Message::getCommand() const
{
	return command_;
}

std::vector<std::string> const &Message::getParameters() const
{
	return parameters_;
}

bool const &Message::isValidMessage() const
{
	return valid_message_;
}

bool const &Message::hasTrailer() const
{
	return has_trailer_;
}

std::string const &Message::getTrailer() const
{
	return trailer_;
}

int const &Message::getClientfd() const
{
	return client_fd_;
}

std::shared_ptr<Client> const &Message::getClientPtr() const
{
	return client_ptr_;
}
