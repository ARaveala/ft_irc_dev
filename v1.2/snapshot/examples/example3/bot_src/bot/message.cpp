#include "Bot.h"
#include "../botmessage/BotMessage.h"

void Bot::readBuffer()
{
	ssize_t readbyte = 0;
	std::vector<std::string> commands;
	char buffer[1024] = {};
	memset(buffer, 0, sizeof(buffer));
	readbyte = recv(server_fd_, buffer, 1024 - 1, 0);
	if (readbyte < 0 && !Bot::signal_)
	{
		std::cerr << "recv function failed" << std::endl;
		return;
	}
	else if (!readbyte)
	{
		std::cout << RED << "Bot lost its connection with the server" << RESET << std::endl;
		reConnection();
		init_bot();
	}
	else
	{
		appendToBuffer(std::string(buffer, readbyte));
		processBuffer();
	}
	if (Bot::signal_)
	{
		close(server_fd_);
		return;
	}
}

void Bot::appendToBuffer(const std::string &data)
{
	buffer_ += data;
}

void Bot::processBuffer()
{
	size_t pos;
	while ((pos = this->buffer_.find("\r\n")) != std::string::npos)
	{
		std::string line = this->buffer_.substr(0, pos);
		this->buffer_.erase(0, pos + 2);
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		BotMessage message(line);
		if (message.isValidMessage() == true)
			processCommand(message);
	}
}

void Bot::processCommand(BotMessage &message)
{
	std::string command;
	try
	{
		int rpl_number = std::stoi(message.getReplyNumber());

		switch (rpl_number)
		{
		case 001:
			this->setRegisterStatus(true);
			return;
		case 433:
			command = "NICK";
			break;
		case 451:
			command = "JOIN";
			break;
		}
	}
	catch (const std::exception &e)
	{
		command = message.getReplyNumber();
	}
	auto it = this->getSupportedCommands().find(command);
	if (it != this->getSupportedCommands().end())
	{
		auto handler = it->second; // Get the function pointer from the map
		BotCommand commandObject(this);
		(commandObject.*handler)(message);
	}
}