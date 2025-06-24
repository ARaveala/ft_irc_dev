#include "Client.h"

Client::Client(const int &fd, const std::string &nickname, const std::string &username, const std::string &ipaddress) : 
		fd_(fd), registered_(false), has_correct_password_(false), rejected_(false), nickname_(nickname), username_(username),
		ip_address_(ipaddress),	mode_i_(false), mode_local_op_(false), away_status_(false)
{
}

Client::~Client()
{
}

int	const &Client::getFd() const
{
	return this->fd_;
}

std::string	const &Client::getNickname() const
{
	return this->nickname_;
}

std::string	const &Client::getOldNickname() const
{
	return this->old_nickname_;
}

std::string	const &Client::getUsername() const
{
	return this->username_;
}

std::string	const &Client::getHostname() const
{
	return this->hostname_;
}

std::string	const &Client::getRealname() const
{
	return this->realname_;
}

std::string	const &Client::getIpAddress() const
{
	return this->ip_address_;
}

bool	const &Client::getRegisterStatus() const
{
	return this->registered_;
}

bool	const &Client::getModeI() const
{
	return this->mode_i_;
}

bool	const &Client::getModeLocalOp() const
{
	return this->mode_local_op_;
}

std::string const &Client::getAwayMessage() const
{
	return away_message_;
}

std::string const &Client::getClientPrefix() const
{
	return client_prefix_;
}

std::vector<std::weak_ptr<Channel>> const &Client::getChannels() const 
{
    return channels_;
}

bool const &Client::getRejectedStatus() const
{
	return rejected_;
}

bool const &Client::getHasCorrectPassword() const
{
	return has_correct_password_;
}

bool const &Client::isAway() const
{
	return away_status_;
}

void Client::setFd(int const &fd)
{
	this->fd_ = fd;
}

void Client::setNickname(std::string const &nickname)
{
	this->old_nickname_ = this->nickname_;
	this->nickname_ = nickname;
}

void Client::setUsername(std::string const &username)
{
	this->username_ = username;
}

void Client::setHostname(std::string const &hostname)
{
	this->hostname_ = hostname;
}

void Client::setRealname(std::string const &realname)
{
	this->realname_ = realname;
}

void Client::setModeI(bool status)
{
	this->mode_i_ = status;
}

void Client::setModeLocalOp(bool status)
{
	this->mode_local_op_ = status;
}

void	Client::setHasCorrectPassword(bool const &status)
{
	has_correct_password_ = status;
}

void Client::setIpAddress(std::string const &ip_address)
{
	this->ip_address_ = ip_address;
}

void Client::setClientPrefix()
{
	client_prefix_ = nickname_ + "!~" + username_ + "@" + ip_address_;
}

void Client::setAway(bool status, const std::string& message)
{
	away_status_ = status;
	away_message_ = message;
}

void Client::setRejectedStatus(bool const &status)
{
	rejected_ = status;
}

void Client::registerClient()
{
	if (this->registered_ == false)
		this->registered_ = true;
}

void Client::unregisterClient()
{
	if (this->registered_ == true)
		this->registered_ = false;
}

void Client::processBuffer(Server *server_ptr)
{
	size_t pos;
	while ((pos = this->buffer.find("\r\n")) != std::string::npos)
	{
		std::string line = this->buffer.substr(0, pos);
		this->buffer.erase(0, pos + 2); // Remove the \r\n
		if (!line.empty() && line.back() == '\r')
			line.pop_back(); // Remove the trailing \r
		while (line.size() > MAX_MSG_LENGTH - 2)
		{
			std::string part = line.substr(0, MAX_MSG_LENGTH - 2); // 510 bytes max for a message
			line = line.substr(MAX_MSG_LENGTH - 2); // Remove the first 510 bytes
			Message message(part, server_ptr, this->fd_);
			if(message.isValidMessage() == true)
				processCommand(message, server_ptr);
		}
		if (!line.empty())
		{
			Message message(line, server_ptr, this->fd_);
			if(message.isValidMessage() == true)
				processCommand(message, server_ptr);
		}
	}
}

void Client::processCommand(Message &message, Server *server_ptr)
{
    const std::string &command = message.getCommand();
    auto it = server_ptr->getSupportedCommands().find(command);
    if (it != server_ptr->getSupportedCommands().end())
    {
       auto handler = it->second;
       Command command_object(server_ptr);
       (command_object.*handler)(message);
    }
	else
		server_ptr->sendResponse(getFd(), ERR_CMDNOTFOUND(server_ptr->getServerHostname(), getNickname(), command));
}

void Client::appendToBuffer(const std::string &data)
{
	this->buffer += data;
}

bool	Client::joinChannel(const std::shared_ptr<Channel>& channel_ptr)
{
	if (channels_.size() >= CLIENT_MAX_CHANNELS)
		return false;
    channels_.push_back(channel_ptr);
	return true;
}

void Client::leaveChannel(const std::weak_ptr<Channel>& channel_ptr)
{
	auto locked_channel_ptr = channel_ptr.lock();
	if (!locked_channel_ptr)
	{
		std::cerr << ("null ptr in leaveChannel") << std::endl;
		return;
	}
	std:: vector<std::weak_ptr<Channel>>::iterator it = channels_.begin();
	while (it != channels_.end())
    {
        if (it->lock() == locked_channel_ptr)
        {
            it = channels_.erase(it);
            break;
        }
        else
		   ++it;
    }
}