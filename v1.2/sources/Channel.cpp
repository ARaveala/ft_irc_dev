#include <iostream>
#include <bitset>
#include <config.h>
#include <algorithm>
#include <cctype>

#include "IrcResources.hpp"
#include "Channel.hpp"
#include "Client.hpp"

static std::string toLower(const std::string& input) {
	std::string output = input;
    std::transform(output.begin(), output.end(), output.begin(),
	[](unsigned char c) { return std::tolower(c); });
    return output;
}

Channel::Channel(const std::string& channelName)  : _name(channelName), _topic("not set"){
	std::cout << "Channel '" << _name << "' created." << std::endl;
	_ChannelModes.reset();
    _ChannelModes.set(Modes::ChannelMode::TOPIC);
}
Channel::~Channel() {
	std::cout << "Channel '" << _name << "' destroyed." << std::endl;
}

void Channel::setOperatorCount(int value) {
    if (_operatorCount == 0){
        return;
    }
    _operatorCount += value;
    if (_operatorCount < 0) _operatorCount = 0; // extra safeguard
};

const std::string& Channel::getName() const {return _name;}
const std::string& Channel::getTopic() const {return _topic;}
void Channel::setTopic(const std::string& newTopic) {_topic = newTopic;}
bool Channel::setModeBool(char onoff){return onoff == '+';}
bool Channel::isEmpty() const {return _ClientModes.empty();}
bool Channel::isInvited(const std::string& nickname) const { return std::find(_invites.begin(), _invites.end(), nickname) != _invites.end();}// Use std::find to check if the nickname exists in the _invites deque
bool Channel::isValidChannelMode(char modeChar) const { return std::find(Modes::channelModeChars.begin(), Modes::channelModeChars.end(), modeChar) != Modes::channelModeChars.end();}
bool Channel::isValidClientMode(char modeChar) const { return std::find(Modes::clientModeChars.begin(), Modes::clientModeChars.end(), modeChar) != Modes::clientModeChars.end();}
bool Channel::channelModeRequiresParameter(char modeChar) const {return ( modeChar == 'k' || modeChar == 'l' || modeChar == 'o');}


std::vector<int> Channel::getAllfds() {
	std::vector<int> fds;
	for (const auto& entry : _ClientModes) {
		if (auto clientPtr = entry.first.lock()) {
			fds.push_back(entry.second.second);
		}
	}
	return fds;
}
// get end list of nicknames
const std::string Channel::getAllNicknames() {
	std::string list;
	for (const auto& entry : _ClientModes) {
		if (auto clientPtr = entry.first.lock()) {
			list += clientPtr->getNickname() + "!" + clientPtr->getNickname() + "@localhost ";
		}
	}
	return list;
}

std::string Channel::getNicknameFromWeakPtr(const std::weak_ptr<Client>& weakClient) {
    if (auto clientPtr = weakClient.lock()) {
        return clientPtr->getNickname();
	}
    return "";
}					
						
std::bitset<config::CLIENT_NUM_MODES>& Channel::getClientModes(const std::string nickname)
{
	std::cout << "Total Clients: " << _ClientModes.size() << std::endl;
	for (auto& entry : _ClientModes) {
		std::cout<<"whats thje name we looking at now = ["<<entry.first.lock()->getNickname()<<"]\n";
		if (auto clientPtr = entry.first.lock(); clientPtr && clientPtr->getNickname() == nickname) {
			
			return entry.second.first;  // return the matching weak_ptr
		}
	}
	// we could substitute with our own  throw here
	throw std::runtime_error("Client not found get client modes!");
	//return ;  // return empty weak_ptr if no match is found
}

bool Channel::isClientInChannel(const std::string& nickname) const {
	for (const auto& entry : _ClientModes) {
		if (auto clientPtr = entry.first.lock(); clientPtr && toLower(clientPtr->getNickname()) == nickname) {
			return true;
		}
	}
	return false;
}
					
std::string Channel::getCurrentModes() const {				
	std::string activeModes = "+";
	for (size_t i = 0; i < Modes::channelModeChars.size(); ++i) {
        if (_ChannelModes.test(i)) {
			activeModes += Modes::channelModeChars[i];
        }
    }
    return activeModes;
}

Modes::ClientMode Channel::charToClientMode(const char& modeChar) {
	switch (modeChar) {
		case 'o': return Modes::OPERATOR;
		case 'q': return Modes::FOUNDER;
		default : return Modes::CLIENT_NONE;
	}
}

Modes::ChannelMode Channel::charToChannelMode(const char& modeChar) {
	switch (modeChar) {
		case 'i': return Modes::INVITE_ONLY;
		case 'k': return Modes::PASSWORD;
		case 'l': return Modes::USER_LIMIT;
		case 't': return Modes::TOPIC;
		default : return Modes::NONE;
	}
}

std::vector<std::string> Channel::applymodes(std::vector<std::string> params)
{
	std::string modes;
	std::string targets;

	size_t paramIndex = 1;
	size_t modeIndex = 0;
	std::string modeFlags; 
	char modeChar;
	char sign;
	std::vector<std::string> messageData;
	while (paramIndex < params.size())
	{
		if (modeIndex == 0){
				sign = params[paramIndex][modeIndex]; 
				modeFlags = params[paramIndex].substr(1);
				modes += sign; 
			}
		while (modeIndex < modeFlags.size())
		{
			modeChar = modeFlags[modeIndex];
			if (channelModeRequiresParameter(modeChar)) {
				paramIndex++;
			}
			messageData = setChannelMode(modeChar , setModeBool(sign), params[paramIndex]);
			if (modeChar == 'o') {
			    if (setModeBool(sign)) {
			        ++_operatorCount;
					_wasOpRemoved = false;
			    } else {
			        --_operatorCount;
			        _wasOpRemoved = true;
			    }
			}		
			if (!messageData.empty()) {
				modes += messageData[0] + " ";
				if (messageData.size() > 1)
					targets += messageData[1] + " ";
				messageData.clear();
			}
			modeIndex++; 
		}
		paramIndex++;
	}
	if (modes.size() > 1) {
		messageData.push_back(modes); 
		messageData.push_back(targets); 
	}
	return messageData;
}
// what happnes if u limit is set to lower than there are clients in channel
// ulimit is not working just as it should, 
std::vector<std::string> Channel::setChannelMode(char modeChar , bool enableMode, const std::string& target) {
	std::vector<std::string> response;
	bool modeChange = false;
	Modes::ChannelMode cmodeType = charToChannelMode(modeChar);
	Modes::ClientMode modeType = charToClientMode(modeChar);
	if (cmodeType == Modes::NONE && modeType == Modes::CLIENT_NONE) {return {};};
	if (cmodeType!= Modes::NONE && _ChannelModes.test(cmodeType) != enableMode) {
		_ChannelModes.set(cmodeType, enableMode);
		modeChange = true;
	}

	bool shouldReport = modeChange || enableMode;
	if (shouldReport) {
			response.push_back(std::string(1, modeChar)); // Mode char
	}
 	switch (cmodeType) {
        case Modes::USER_LIMIT:
            if (enableMode) {
				//overflow check since max clients for whole server is like 550. cap at that ?
				_ulimit = std::stoul(target);
			} else {
				_ulimit = 0;
			}
			if (shouldReport) {
                response.push_back(target);
			}
			break;

        case Modes::PASSWORD:
            if (enableMode) {
				_password = target;
			} else {
				_password.clear();
			}
			if (shouldReport) {
                response.push_back(target);
            }
			break;
        default:
            break;
    }
	 if (modeType != Modes::CLIENT_NONE) {
		if(getClientModes(target).test(modeType) != enableMode) {
			getClientModes(target).set(modeType, enableMode);
		}
		response.push_back(std::string(1, modeChar)); // Mode char
        response.push_back(target);  // The user being affected
    }
    return response;
}

// using optional here instead as we can just retun nullptr 
std::optional<std::pair<MsgType, std::vector<std::string>>>
Channel::canClientJoin(const std::string& nickname, const std::string& value) {
    if (_ChannelModes.test(Modes::INVITE_ONLY)) {
        auto it = std::find(_invites.begin(), _invites.end(), nickname);
        if (it != _invites.end()) {
            _invites.erase(it);
        } else {
            return std::make_pair(MsgType::INVITE_ONLY, std::vector<std::string>{nickname, getName()});
        }
    }
    if (_ChannelModes.test(Modes::PASSWORD)) {
        if (value != _password) {
			return std::make_pair(MsgType::INVALID_PASSWORD, std::vector<std::string>{nickname, getName()});
        }
    }
	if (_ChannelModes.test(Modes::USER_LIMIT)) {
		if (_clientCount + 1 > _ulimit) {
            return std::make_pair(MsgType::CHANNEL_FULL, std::vector<std::string>{nickname, getName()});
		}
	}
    return std::nullopt;
}

int Channel::addClient(std::shared_ptr <Client> client) {
	if (!client)
		return -1;
	std::weak_ptr<Client> weakclient = client;
	if (_ClientModes.find(weakclient) == _ClientModes.end()) {
		_ClientModes.insert({weakclient, std::make_pair(std::bitset<config::CLIENT_NUM_MODES>(), client->getFd())});
		if (client->getChannelCreator() == true) {
			setChannelMode('o', setModeBool('+'), client->getNickname());
			setChannelMode('q', setModeBool('+'), client->getNickname()); // incase we want to know who created the channel
			setOperatorCount(1);
			client->setChannelCreator(false);
		}
		else {
			setChannelMode('o', setModeBool('-'), client->getNickname());
			setChannelMode('q', setModeBool('+'), client->getNickname());
		}
		_clientCount += 1;
	} else {
		LOG_DEBUG("Client already exists in channel!");
		return 1;
	}
    return 2;
}

std::pair<MsgType, std::vector<std::string>> Channel::initialModeValidation(
        const std::string& ClientNickname,
        size_t paramsSize)  {
        if (paramsSize == 1) {
            if (!isClientInChannel(ClientNickname)) {
                return {MsgType::NOT_ON_CHANNEL, {ClientNickname, getName()}};
            }
            return {MsgType::RPL_CHANNELMODEIS, {ClientNickname, getName(), getCurrentModes(), ""}};
        }
        if (!isClientInChannel(ClientNickname)) {
            return {MsgType::NOT_ON_CHANNEL, {ClientNickname, getName()}};
        }
        if (!getClientModes(ClientNickname).test(Modes::OPERATOR)) {
            return {MsgType::NOT_OPERATOR, {ClientNickname, getName()}};
        }
        return {MsgType::NONE, {}};
}


MsgType Channel::checkModeParameter(const std::string& nick, char mode, const std::string& param, char sign) const {
	(void)nick;
	if (mode == 'o') {
        if (param.empty() || !isClientInChannel(param)) {
			LOG_DEBUG("Invalid or missing client " + param + " for +o.");
            return MsgType::NOT_ON_CHANNEL;
        }
    }
    else if (mode == 'l' && sign == '+') {
        try {
            unsigned long limit = std::stoul(param);
            if (limit > 100) {
				LOG_DEBUG("Limit too high (" + std::to_string(limit) + ")");
				return MsgType::NEED_MORE_PARAMS;
            }
        } catch (...) {
			return MsgType::NEED_MORE_PARAMS;
        }
    }
    else if (mode == 'k' && sign == '+') {
		if (param.empty()) {
			return MsgType::NEED_MORE_PARAMS;
        }
    }
    return MsgType::NONE;
}

std::pair<MsgType, std::vector<std::string>>
Channel::promoteFallbackOperator(const std::shared_ptr<Client>& removingClient, bool isLeaving) {
	LOG_DEBUG("promoteFallbackoperator operator count = "+ std::to_string(_operatorCount) + " clientmode size = " + std::to_string(_ClientModes.size()));
	if (!isLeaving && _operatorCount > 0) {
		LOG_DEBUG("promoteFallbackoperator-------- returned NONE :: operator count too big");
		return {MsgType::NONE, std::vector<std::string>()};
	};
	if (isLeaving && _operatorCount == 1) {
		LOG_DEBUG("promoteFallbackoperator-------- returned NONE :: operator count too big");
		return {MsgType::NONE, std::vector<std::string>()};
	};
	if (_ClientModes.size() == 1) {
		LOG_DEBUG("promoteFallbackoperator-------- returned NONE :: client size");
		return {MsgType::NONE, std::vector<std::string>()};
	};
    for (auto it = _ClientModes.begin(); it != _ClientModes.end(); ++it) {
		std::shared_ptr<Client> candidate = it->first.lock();
        if (!candidate || candidate == removingClient) continue;
        it->second.first.set(Modes::OPERATOR, true);
		return {MsgType::CHANNEL_MODE_CHANGED, {removingClient->getNickname(), removingClient->getUsername(), getName(), "+o", candidate->getNickname()}};
	}
	LOG_DEBUG("promoteFallbackoperator-------- returned NONE");
	return {MsgType::NONE, std::vector<std::string>()};
}

std::pair<MsgType, std::vector<std::string>>
Channel::modeSyntaxValidator(const std::string& nick, const std::vector<std::string>& params) const {
	size_t idx = 1;
    char sign = ' ';
    while (idx < params.size()) {
		const std::string& token = params[idx];
        if (token.empty() || (token[0] != '+' && token[0] != '-')) {
			LOG_DEBUG("Syntax Error: Unexpected token = " + token);
            return {MsgType::NEED_MORE_PARAMS, {nick, "MODE"}};
        }
        sign = token[0];
        for (size_t i = 1; i < token.size(); ++i) {
			char mode = token[i];
            if (!isValidChannelMode(mode) && !isValidClientMode(mode)) {
				return {MsgType::UNKNOWN_MODE, {std::string(1, mode), nick, getName()}};
            }
            if (!channelModeRequiresParameter(mode)) continue;
            if (idx + 1 >= params.size()) {
				return {MsgType::NEED_MORE_PARAMS, {nick, "MODE"}};
            }
			
            const std::string& param = params[idx + 1];
            MsgType checkResult = checkModeParameter(nick, mode, param, sign);
            if (checkResult != MsgType::NONE) {
				if (checkResult == MsgType::NOT_ON_CHANNEL){
					return {checkResult, {param, params[0]}};
				}
				return {checkResult, {nick, "MODE", std::string(1, mode), param}};
            }
            ++idx;
        }
        ++idx;
    }
    return {MsgType::NONE, {}};
}

 std::string Channel::getClientModePrefix(std::shared_ptr<Client> client) const {
	if (!client) { return ""; }
    for (const auto& entry : _ClientModes) {
        if (auto weakClientPtr = entry.first.lock(); weakClientPtr && weakClientPtr == client) {
            const std::bitset<config::CLIENT_NUM_MODES>& modes = entry.second.first;
            std::string prefix = "";
            if (modes.test(Modes::OPERATOR)) {
                prefix = "@"; 
            }
            return prefix;
        }
    }
    return "";
}


/* Impact and Important Reminder for Case-Sensitivity:

If you are strictly case-sensitive for nicknames and channel names:

Consistency is paramount: Every single place where a nickname or channel name is stored, retrieved, or compared must use the exact same casing.

When a client sets their NICK or USER.
When a client JOINs a channel.
When you look up clients in Server::_clients.
When you look up channels in Server::_channels.
When you store client nicknames within Channel structures (e.g., for isClientInChannel, isClientOperator, removeClient).
When you process commands like PRIVMSG, NOTICE, WHOIS, MODE, KICK, PART, QUIT, TOPIC, etc.
User Experience: Be aware that most IRC clients and many servers are case-insensitive for nicknames and channel names (or at least provide some normalization). Your server will behave differently, and users might find it less intuitive if "Nick" and "nick" are treated as two different users, or "#Channel" and "#channel" as two different channels. However, if this is a project constraint, then it's a known trade-off.

Proceed with this version of isClientOperator. It will correctly implement the logic given your team's chosen approach to case sensitivity.

*/
//NEWNEW add this and call where relevant
/*void Channel::cleanExpiredClients() {
    for (auto it = _ClientModes.begin(); it != _ClientModes.end(); ) {
        if (it->first.expired()) {
            it = _ClientModes.erase(it);
        } else {
            ++it;
        }
    }
}*/

//TOLOWER
bool Channel::removeClientByNickname(const std::string& nickname) {
    
	for (auto it = _ClientModes.begin(); it != _ClientModes.end(); ++it) {
        std::shared_ptr<Client> client_sptr = it->first.lock();
        if (client_sptr && toLower(client_sptr->getNickname()) == nickname) {
            _ClientModes.erase(it);
			_clientCount--;
			LOG_NOTICE("removeClientByNickname: " + nickname + " Removed from channel " + _name);
            return _ClientModes.empty();
        }
    }
	LOG_ERROR("removeClientByNickname: Could not find client " + nickname + " to remove from " + _name);
    return  _ClientModes.empty(); // Still return status in all cases
}

void Channel::addInvite(const std::string& nickname) {
    if (!isInvited(nickname)) {
        _invites.push_back(nickname);
        std::cout << "CHANNEL: Added '" << nickname << "' to invite list for '" << _name << "'. Current invites: " << _invites.size() << std::endl;
    } else {
        std::cout << "CHANNEL: '" << nickname << "' is already on invite list for '" << _name << "'.\n";
    }
}

void Channel::removeInvite(const std::string& nickname) {
    auto it = std::find(_invites.begin(), _invites.end(), nickname);
    if (it != _invites.end()) {
        _invites.erase(it);
        std::cout << "CHANNEL: Removed '" << nickname << "' from invite list for '" << _name << "'. Current invites: " << _invites.size() << std::endl;
    }
}
