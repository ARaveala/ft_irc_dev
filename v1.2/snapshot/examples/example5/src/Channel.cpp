
#include "Server.hpp"


Channel::Channel(std::string name) : 
		_name						(name),
		_user_limit					(-1),
		_keyRequired				(false),
		_invite_only				(false),
		_topic_requires_operator	(true)
{
	_creationTime = getCurrentEpochTime();
}

int	Channel::getOpCount()
{
	int count = 0;
	for (std::vector<User>::iterator it = _channel_users.begin(); it != _channel_users.end(); it++)
	{
		if (it->operator_permissions)
			count++;
	}
	return (count);
}

int		Channel::getTotalCount()
{
	return (_channel_users.size());
}

User&	Channel::getChannelUserStruct(int index)
{
	return (this -> _channel_users[index]);
}

void Channel::setChannelKey(std::string key)
{
	this->_channel_key = key;
}

void	Channel::setChannelTopic(std::string new_topic, Client &client)
{
	this->_topic = new_topic;
	this->_topicSetter = client.getPrefix();
	this->_topicSetTime = getCurrentTime();
}

void	Channel::addUserIntoChannelUsers(User new_user)
{
	this->_channel_users.push_back(new_user);
}

void	Channel::removeUserFromChannelUsers(int index)
{
	if (index < 0)
	{
		return ;
	}
	this->_channel_users.erase(this ->_channel_users.begin() + index);
}

std::string Channel::getInvitedName(int index)
{
    int		invite_list_length = static_cast<int>(this->_invited.size());

	if (index < 0 || index >= invite_list_length)
    {
        return "";
    }

    return this->_invited[index];
}

void		Channel::addUserToInviteList(std::string nickname)
{
	this -> _invited.push_back(nickname);
}

void	Channel::uninviteUser(int index)
{
	if (index < 0)
	{
		return ;
	}
	this->_invited.erase(this->_invited.begin() + index);
}

void	Channel::setKeyRequired(bool boolean)
{
	this -> _keyRequired = boolean;
}

void	Channel::setInviteOnly(bool boolean)
{
	this -> _invite_only = boolean;
}

void	Channel::setUserLimit(int userLimit)
{
	this -> _user_limit = userLimit;
}

void	Channel::setTopicRequiresOperator(bool boolean)
{
	this -> _topic_requires_operator = boolean;
}
