#include "Bot.h"

bool Bot::signal_ = false;
Bot::Bot(std::string &server_address, int &port, std::string &password, char **av)
	: server_addr_(server_address), server_port_(port), server_password_(password), register_status_(false)
{
	if (isValidNickname(av[4]))
		nickname_ = av[4];
	else
		throw std::runtime_error("Invalid Nickname format");
	username_ = av[5];
	register_status_ = false;
	readConfigFile();
	supported_commands_.insert(std::pair("JOIN", &BotCommand::handleJoin));
	supported_commands_.insert(std::pair("INVITE", &BotCommand::handleInvite));
	supported_commands_.insert(std::pair("NICK", &BotCommand::handleNick));
	supported_commands_.insert(std::pair("PRIVMSG", &BotCommand::handlePrivmsg));
}

Bot::~Bot()
{
}

void Bot::init_bot()
{
	if (Bot::signal_)
		return;
	createBotSocket();
	poll_fd_.fd = server_fd_;
	poll_fd_.events = POLLIN;
	poll_fd_.revents = 0;
	int event;
	while (!Bot::signal_)
	{
		event = poll(&poll_fd_, POLL_IN, 1000);
		if (event == 0 && this->getRegisterStatus() == false)
			testConnection();
		if (event == -1 && !Bot::signal_)
			throw std::runtime_error("Error poll");
		if (poll_fd_.revents && POLL_IN)
		{
			readBuffer();
		}
	}
	std::string msg = "QUIT :TERMINATED";
	send_response(server_fd_, msg + CRLF);
	close(poll_fd_.fd);
	close(server_fd_);
}

void Bot::createBotSocket()
{
	memset(&addr_info_, 0, sizeof addr_info_);
	addr_info_.ai_family = AF_UNSPEC;
	addr_info_.ai_socktype = SOCK_STREAM;
	getaddrinfo(server_addr_.c_str(), std::to_string(server_port_).c_str(), &addr_info_, &serv_addr_info_);
	server_fd_ = socket(serv_addr_info_->ai_family, serv_addr_info_->ai_socktype, serv_addr_info_->ai_protocol);
	if (server_fd_ < 0)
	{
		std::cerr << "Error creating socket" << std::endl;
		return;
	}
	std::cout << "Bot created a socket successfuly" << std::endl;
	int flags = fcntl(server_fd_, F_GETFL, 0);
	if (flags < 0)
	{
		close(server_fd_); // Close the socket if fcntl fails
		throw std::runtime_error("Error getting socket flags");
	}
	if (fcntl(server_fd_, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		close(server_fd_); // Close the socket if fcntl fails
		throw std::runtime_error("Error setting socket to NON-BLOCKING");
	}
	if (connect(server_fd_, serv_addr_info_->ai_addr, serv_addr_info_->ai_addrlen) < 0)
	{
		if (errno != EINPROGRESS)
		{ // Check if connection is in progress
			std::cerr << "Error connecting to server" << std::endl;
			close(server_fd_); // Close the socket if connect fails
			return;
		}
	}
	std::cout << "Bot connected to server successfuly" << std::endl;
	free(serv_addr_info_);
}

void Bot::testConnection()
{
	if (!this->getRegisterStatus())
	{
		std::string msg = "JOIN";
		send_response(server_fd_, msg + CRLF);
	}
}

void Bot::signalhandler(int signum)
{
	switch (signum)
	{
	case SIGINT:
		std::cout << "SIGINT (Interrupt signal)" << std::endl;
		break;
	case SIGQUIT:
		std::cout << "SIGQUIT (Quit)" << std::endl;
		break;
	}
	signal_ = true;
}

std::string const &Bot::getServerAddr() const
{
	return this->server_addr_;
}

std::string const &Bot::getServerPassword() const
{
	return this->server_password_;
}

int const &Bot::getServerPort() const
{
	return this->server_port_;
}

std::string const &Bot::getNickname() const
{
	return nickname_;
}

std::string const &Bot::getUsername() const
{
	return username_;
}

void Bot::setNickname(std::string const &nickname)
{
	nickname_ = nickname;
}

void Bot::setUsername(std::string const &username)
{
	username_ = username;
}

void Bot::setRegisterStatus(bool const &status)
{
	register_status_ = status;
}

bool const &Bot::getRegisterStatus() const
{
	return register_status_;
}