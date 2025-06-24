#include "Client.hpp"


Client::Client(int socket, const std::string& password) : 
					_password			(password),
					_passwordChecked	(false),
					_welcomeSent		(false),
					_socket				(socket)
{

};

std::string Client::getNickname()
{
    return(_nickname);
}

std::string Client::getUsername()
{
	return(_username);
}

std::string Client::getPassword()
{
    return(_password);
}

std::string Client::getHost()
{
    return(_host);
}

std::string Client::getRealname()
{
    return(_realname);
}

std::string Client::getHostname()
{
	return(_hostname);
}

std::string Client::getPrefix()
{
    return _prefix;
}

bool        Client::getPasswordChecked()
{
    return(_passwordChecked);
}

bool        Client::getWelcomeSent()
{
    return(_welcomeSent);
}

int         Client::getSocket()
{
    return(_socket);
}

std::vector<std::string>& Client::getChannelsNames()
{
    return(_channelsNames);
}

void        Client::setNickname(std::string str)
{
    _nickname = str;
}

void        Client::setUsername(std::string str)
{
    _username = str;
}

void        Client::setHost(std::string str)
{
    _host = str;
}

void        Client::setHostname(std::string str)
{
    _hostname = str;
}

void        Client::setRealname(std::string str)
{
    _realname = str;
}

void       Client::setPrefix(std::string prefix)
{
    _prefix = prefix;
}

void        Client::setPasswordChecked(bool value)
{
    _passwordChecked = value;
}

void        Client::setWelcomeSent(bool value)
{
    _welcomeSent = value;
}


void        Client::joinChannel(std::string channelName)
{
    std::vector<std::string>::iterator it = std::find(_channelsNames.begin(), _channelsNames.end(), channelName);

    if (it != _channelsNames.end())		//If channel found
        return;
    else								//If channel NOT found
    {
        _channelsNames.push_back(channelName);
    }
}

/*
    Removes from a list of channels stored on Client.
*/
void       Client::leaveChannel(std::string channelName)
{
	std::vector<std::string>::iterator it = std::find(_channelsNames.begin(), _channelsNames.end(), channelName);

	if (it != _channelsNames.end())		//If channel found
	{
		_channelsNames.erase(it);
	}
}
