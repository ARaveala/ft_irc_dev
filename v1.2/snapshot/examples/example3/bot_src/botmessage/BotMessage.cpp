#include "BotMessage.h"

/// @brief constructor for receiving a message from client and getting ready to parse it
/// @param raw_message
/// @param server
/// @param clientfd
BotMessage::BotMessage(std::string raw_message)
	: raw_message_(raw_message), valid_message_(false)
{
	std::cout << CYAN << "Received from Server: " << raw_message << "\t"
			  << "Message size = " << raw_message.length() << " byte" << RESET << std::endl;
	valid_message_ = analyzeMessage();
}

BotMessage::~BotMessage()
{
}

bool BotMessage::analyzeMessage()
{
	std::istringstream iss(raw_message_);
	std::string prefix;

	if (raw_message_.front() == ':')
	{
		std::getline(iss, prefix, ' '); // Extract prefix up to the first space
		prefix_ = prefix;
	}

	std::string reply_num, param;

	iss >> reply_num;
	reply_num_ = reply_num;
	while (iss >> param)
	{
		if (param.front() == ':')
		{
			if (param.size() > 1)
			{
				trailer_ = param;
				while (iss >> param)
					trailer_ += " " + param;
				trailer_.erase(trailer_.begin());
			}
			break;
		}
		parameters_.push_back(param);
	}

	return true;
}

void BotMessage::printMessageContents()
{
	std::cout << "Printing contents:\n";
	std::cout << "Prefix: " << prefix_ << "\n";
	std::cout << "Reply number: " << reply_num_ << "\n";
	std::cout << "Params:\n";
	for (auto param : parameters_)
		std::cout << param << "\n";
	std::cout << "Trailer trash: " << trailer_ << std::endl;
}

std::string const &BotMessage::getReplyNumber() const
{
	return reply_num_;
}

std::vector<std::string> const &BotMessage::getParameters() const
{
	return parameters_;
}

bool BotMessage::isValidMessage()
{
	return valid_message_;
}

std::string const &BotMessage::getTrailer() const
{
	return trailer_;
}

std::string const &BotMessage::getPrefix() const
{
	return prefix_;
}
