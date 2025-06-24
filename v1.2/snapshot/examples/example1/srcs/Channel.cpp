#include "Server.hpp"

Channel::Channel(const std::string &name)
	: _inviteOnly(false), _topicRestrictions(true), _isChannelPassword(false),
	_userLimit(-1), _userCount(0), _name(name), _topic("")
{
	_creationTime = std::to_string(std::time(nullptr));
	std::cout << "Channel " << getName() << " has been created" << std::endl;
}

Channel::~Channel()
{
	std::cout << "Channel " << getName() << " destroyed." << std::endl;
}

void Channel::addClient(const std::shared_ptr<Client>& client)
{ 
	if (getUserLimit() == -1 || (int)_clients.size() < getUserLimit())
	{
		_clients.push_back(client);
		incrementUserCount();
		return ;
	}
}

void Channel::removeClient(const std::shared_ptr<Client>& client)
{
	for (size_t i = 0; i < _clients.size(); i++)
	{
		if (_clients[i] == client)
		{
			_clients.erase(_clients.begin() + i);
			decrementUserCount();
			break;
		}
	}
}

/*
	Returns -1 if client is not client of the channel
*/
int Channel::isClient(const std::shared_ptr<Client>& client) const
{
	for (size_t i = 0; i < _clients.size(); i++)
	{
		if (_clients[i] == client)
			return i;
	}
	return -1;
}

void Channel::addAdmin(const std::shared_ptr<Client>& admin)
{
	if (isAdmin(admin) != -1)
		return;
	broadcastAdmins(": " + admin->getNickname() + " was added as admin of " + getName() + "\r\n");
	_admins.push_back(admin);
}

void Channel::removeAdmin(const std::shared_ptr<Client>& admin)
{
	for (size_t i = 0; i < _clients.size(); i++)
	{
		_admins.erase(_admins.begin() + i);
		broadcastAdmins(": " + admin->getNickname() + " was removed as admin of " + getName() + "\r\n");
		break;
	}
}

/*
	Returns -1 if client is not admin of the channel
*/
int Channel::isAdmin(const std::shared_ptr<Client>& client) const
{
	for (size_t i = 0; i < _admins.size(); i++)
	{
		if (_admins[i] == client)
			return i;
	}
	return -1;
}

void Channel::broadcast(const std::string &msg, const std::shared_ptr<Client>& sender, int flag)
{
	for (size_t i = 0; i < _clients.size(); i++)
	{
		if (sender && _clients[i] != sender)
			send(_clients[i]->getSocket(), msg.c_str(), msg.length(), 0);
		else if (flag == 0)
			send(_clients[i]->getSocket(), msg.c_str(), msg.length(), 0);
	}
}

void Channel::broadcastAdmins(const std::string &msg)
{
	for (size_t i = 0; i < _admins.size(); i++)
		send(_admins[i]->getSocket(), msg.c_str(), msg.length(), 0);
}

void Channel::addInvited(const std::shared_ptr<Client>& client)
{
	if (isInvited(client) >= 0)
		return;
	_invited.push_back(client);
	broadcastAdmins(": " + client->getNickname() + " has been invited to " + getName() + "\r\n");
}

void Channel::removeInvited(const std::shared_ptr<Client>& client)
{
	for (size_t i = 0; i < _invited.size(); i++)
	{
		if (_invited[i] == client)
		{	_invited.erase(_invited.begin() + i); break;}
	}
}

/*
	Returns -1 if client is not invited to the channel
*/
int Channel::isInvited(const std::shared_ptr<Client>& client) const
{
	for (size_t i = 0; i < _invited.size(); i++)
	{
		if (_invited[i] == client)
			return i;
	}
	return -1;
}

std::ostream& operator<<(std::ostream& os, const Channel& channel)
{
	os << "\n";
	os << "Channel: " << channel.getName() << "\n";
	os << "Topic: " << channel.getTopic() << "\n";
	os << "Password: " << channel.getPassword() << "\n";
	os << "User Limit: " << (channel.getUserLimit()) << "\n";
	os << "Invite Only: " << (channel.getInviteOnly() ? "Yes" : "No") << "\n";
	os << "Topic Restrictions: " << (channel.getTopicRestrictions() ? "Yes" : "No") << "\n";
	os << "Password Protected: " << (channel.getIsChannelPassword() ? "Yes" : "No") << "\n";

	os << "Admins:\n";
	for (const std::shared_ptr<Client>& admin : channel.getAdmins())
		os << "  - " << admin->getNickname() << "\n";

	os << "Clients:\n";
	for (const std::shared_ptr<Client>& client : channel.getClients())
		os << "  - " << client->getNickname() << "\n";

	os << "Invited Clients:\n";
	for (const std::shared_ptr<Client>& invited : channel.getInvited())
		os << "  - " << invited->getNickname() << "\n";

	return os;
}

void Channel::setChannelKey(bool plussign, std::string modeParam)
{
	if (plussign)
	{
		_isChannelPassword = true;
		_password = modeParam;
	}
	else
	{
		_isChannelPassword = false;
		_password = "*";
	}
}

std::string Channel::getChannelModes()
{
	std::string modes = "";
	if (_inviteOnly)
		modes += 'i';
	if (_isChannelPassword)
		modes += 'k';
	if (_userLimit != -1)
		modes += 'l';
	if (_topicRestrictions)
		modes += 't';
	if (modes.empty())
		return nullptr;
	return modes;
}
