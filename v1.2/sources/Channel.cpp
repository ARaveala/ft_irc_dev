#include <iostream>
#include <bitset>
#include <config.h>
#include <algorithm>
#include <cctype>

#include "IrcResources.hpp"
#include "Channel.hpp"
#include "Client.hpp"

Channel::Channel(const std::string& channelName)  : _name(channelName), _topic("not set"){
	std::cout << "Channel '" << _name << "' created." << std::endl;
	_ChannelModes.reset();  //set all modes to off (0)
    _ChannelModes.set(Modes::ChannelMode::TOPIC);  // enable topic protection by default, why to show we can
}
Channel::~Channel() {
    std::cout << "Channel '" << _name << "' destroyed." << std::endl;
}

const std::string& Channel::getName() const {
    return _name;
}

const std::string& Channel::getTopic() const {
    return _topic;
}

std::vector<int> Channel::getAllfds() {
	std::vector<int> fds;
	for (const auto& entry : _ClientModes) {
		if (auto clientPtr = entry.first.lock()) {  // Convert weak_ptr to shared_ptr safely, anny expired pointers will be ignored , oohlalal
			fds.push_back(entry.second.second);  //Retrieve FD from stored pair (bitset, FD)
		}
	}
	return fds;
}
// get end list of nicknames
const std::string Channel::getAllNicknames() {
	std::string list;
	for (const auto& entry : _ClientModes) {
		if (auto clientPtr = entry.first.lock()) {  // Convert weak_ptr to shared_ptr safely, anny expired pointers will be ignored , oohlalal
			list += clientPtr->getNickname() + "!" + clientPtr->getNickname() + "@localhost ";
		}
	}
	return list;
}

std::string Channel::getNicknameFromWeakPtr(const std::weak_ptr<Client>& weakClient) {
    if (auto clientPtr = weakClient.lock()) {  //  Convert weak_ptr to shared_ptr safely
        return clientPtr->getNickname();  //  Get the current nickname live
    }
    return "";  //eturn empty string if the Client no longe
}

// assuming all incoming 'nickname' parameters and stored client nicknames are ALREADY in lowercase.
std::weak_ptr<Client> Channel::getWeakPtrByNickname(const std::string& nickname) {
    // We assume 'nickname' is already in lowercase as per the program's invariant.
    // No need for std::transform here.
    const std::string& target_nickname = nickname; // Use a const reference for clarity

    std::weak_ptr<Client> found_weak_ptr; // To store the weak_ptr of the client we're looking for
    for (auto it = _ClientModes.begin(); it != _ClientModes.end(); ) {
        if (auto clientPtr = it->first.lock()) {
            // We assume clientPtr->getNickname() also returns a lowercase string.
            if (clientPtr->getNickname() == target_nickname) {
                // Found a direct match (case-sensitive due to invariant).
                found_weak_ptr = it->first; // Get the actual weak_ptr key from the map
                ++it; // Move to the next element
            } else {
                ++it;
            }
        } else {
            std::cerr << "CHANNEL WARNING: Found expired weak_ptr in channel '" << _name
                      << "' during nickname lookup. Removing stale entry.\n";
            it = _ClientModes.erase(it); // Erase stale element and advance iterator
        }
    }
    return found_weak_ptr;
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
bool Channel::isValidChannelMode(char modeChar) const {
    return std::find(Modes::channelModeChars.begin(), Modes::channelModeChars.end(), modeChar) != Modes::channelModeChars.end();
}

bool Channel::isValidClientMode(char modeChar) const {
    return std::find(Modes::clientModeChars.begin(), Modes::clientModeChars.end(), modeChar) != Modes::clientModeChars.end();
}

bool Channel::channelModeRequiresParameter(char modeChar) const {
	return ( modeChar == 'k' || modeChar == 'l' || modeChar == 'o');
}

bool Channel::isClientInChannel(const std::string& nickname) const {
	for (const auto& entry : _ClientModes) {
		if (auto clientPtr = entry.first.lock(); clientPtr && clientPtr->getNickname() == nickname) {
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

void Channel::setTopic(const std::string& newTopic) {
    _topic = newTopic;
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

bool Channel::setModeBool(char onoff) {
	return onoff == '+'; 
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
	std::cout<<"starting to change the mode----param at 0"<<params[paramIndex]<<"\n";
	while (paramIndex < params.size())
	{
		if (modeIndex == 0){
				sign = params[paramIndex][modeIndex]; // we take the + or -
				modeFlags = params[paramIndex].substr(1);
				modes += sign; 
			}
		while (modeIndex < modeFlags.size())
		{
			modeChar = modeFlags[modeIndex];//params[paramIndex][modeIndex];
			if (channelModeRequiresParameter(modeChar)) {
				paramIndex++;
			}
			messageData = setChannelMode(modeChar , setModeBool(sign), params[paramIndex]);
			//std::cout<<"whats in message data "<<messageData[0]<<"----\n";			
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
	if (modes.size() > 1) { // not only sign but also mode flag
		messageData.push_back(modes); 
		messageData.push_back(targets); 
	}
	return messageData;
}

std::vector<std::string> Channel::setChannelMode(char modeChar , bool enableMode, const std::string& target) {
	std::cout<<"SET CHANNEL MODE ACTIVATED -------------------------mode char = "<<modeChar<<"\n";

	std::vector<std::string> response;
	bool modeChange = false;
	Modes::ChannelMode cmodeType = Modes::NONE;
	Modes::ClientMode modeType = Modes::CLIENT_NONE;

	if (isValidChannelMode(modeChar)){
		cmodeType = charToChannelMode(modeChar);
		std::cout<<"is mode not changing here "<< static_cast<int>(cmodeType) << std::endl;
	}
	if (isValidClientMode(modeChar)) {
		modeType = charToClientMode(modeChar);
	}
	if (cmodeType == Modes::NONE && modeType == Modes::CLIENT_NONE) {return {};};
	if (cmodeType!= Modes::NONE) {
		if(_ChannelModes.test(cmodeType) != enableMode) {
			_ChannelModes.set(cmodeType, enableMode);
			std::cout<<"mode changed in apply mode----\n";
			modeChange = true;
		}
	}

	//bool shouldReport = !(modeChange && !enableMode) && (cmodeType != Modes::NONE); // || modeType != Modes::CLIENT_NONE);
	bool shouldReport = modeChange || enableMode;

	if (shouldReport) {
			std::cout<<"should report activated ----\n";
			response.push_back(std::string(1, modeChar)); // Mode char
	}

 	switch (cmodeType) {
        case Modes::USER_LIMIT:
            if (enableMode) {
				_ulimit = std::stoul(target);
			} else {
				_ulimit = 0;
			}
			if (shouldReport) { // shouldReport check ensures we only add if we're actually reporting
                response.push_back(target);
			}
			break;

        case Modes::PASSWORD:
            if (enableMode) {
				_password = target;
			} else {
				_password.clear();
			}
			if (shouldReport) { // shouldReport check ensures we only add if we're actually reporting
                response.push_back(target);
            }
			break;
        case Modes::TOPIC:
			break;
        case Modes::INVITE_ONLY:
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
    // - ban list ??
	// - is memeber already joined ??
    return std::nullopt;
}

int Channel::addClient(std::shared_ptr <Client> client) {
	if (!client)
		return -1;
	std::weak_ptr<Client> weakclient = client;
	if (_ClientModes.find(weakclient) == _ClientModes.end()) {
		_ClientModes.insert({weakclient, std::make_pair(std::bitset<config::CLIENT_NUM_MODES>(), client->getFd())});  //Insert new client
		if (client->getChannelCreator() == true) {
			setChannelMode('o', setModeBool('+'), client->getNickname());
			setChannelMode('q', setModeBool('+'), client->getNickname()); // incase we want to know who created the channel
			// if we add any other here we must remeber to set to 0 or 1
			client->setChannelCreator(false); // so we do not step inside here again
		}
		else {
			setChannelMode('o', setModeBool('-'), client->getNickname());
			// make sure these modes are set to 0
			//setClientMode("-q", client->getNickname(), "");
		}
		_clientCount += 1;
	} else {
		std::cout << "Client already exists in channel!" << std::endl;
		return 1;
	}
    return 2; // Return true if insertion happened (Client was not already there)
}

// In sources/Channel.cpp

// Make sure your Channel class has a _ClientModes member like:
// std::map<std::weak_ptr<Client>, int, std::owner_less<std::weak_ptr<Client>>> _ClientModes;
// And that you have a private helper function:
// std::weak_ptr<Client> getWeakPtrByNickname(const std::string& nickname);
// (Or a client pointer, or an iterator to the map entry)

bool Channel::removeClient(const std::string& nickname) {
    // We assume 'nickname' is already in lowercase as per the program's invariant.
    std::cout << "CHANNEL: Attempting to remove client '" << nickname << "' from channel '" << _name << "'\n";

    // 1. Get the weak_ptr for the client from our internal map using the helper.
    //    This helper *must* handle finding the exact (lowercase) nickname and cleaning up expired weak_ptrs.
    std::weak_ptr<Client> client_weak_ptr_to_remove = getWeakPtrByNickname(nickname);

    // 2. Check if the weak_ptr obtained is valid (i.e., not empty).
    if (client_weak_ptr_to_remove.expired()) {
        std::cerr << "CHANNEL ERROR: Client '" << nickname << "' not found or already disconnected from channel '" << _name << "' for removal.\n";
        // Client not found or already gone, return current emptiness status.
        return _ClientModes.empty();
    }

    // 3. Attempt to erase the client's entry using the obtained weak_ptr.
    //    std::map::erase(key) should work correctly with std::weak_ptr keys and std::owner_less.
    size_t removed_count = _ClientModes.erase(client_weak_ptr_to_remove);

    if (removed_count > 0) {
        // Successfully removed the client from _ClientModes.
        std::cout << "CHANNEL: Client '" << nickname << "' successfully removed from channel '" << _name << "'.\n";

        // IMPORTANT: If you have any other channel-specific lists (e.g., separate operator set)
        // that are keyed by weak_ptr, you'd remove the client_weak_ptr_to_remove from those here too.
        // For example:
        // if (_operators.erase(client_weak_ptr_to_remove)) {
        //     std::cout << "CHANNEL: Client was also an operator, removed from _operators list.\n";
        // }
    } else {
        // This case indicates that getWeakPtrByNickname found a valid weak_ptr,
        // but erase() didn't remove anything. This could point to a subtle issue with
        // how weak_ptr equality is handled by map::erase or if the map was modified
        // by another thread (unlikely in a single-threaded epoll loop).
        // Or perhaps a transient state where the weak_ptr became expired *between*
        // getWeakPtrByNickname returning and erase being called.
        std::cerr << "CHANNEL ERROR: Failed to erase client '" << nickname << "' from channel '" << _name << "' "
                  << "(getWeakPtrByNickname returned valid but erase failed).\n";
    }

    // 4. Return true if the channel's member list is now empty.
    return _ClientModes.empty();
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
            std::cout << "DEBUG: Invalid or missing client '" << param << "' for +o.\n";
            return MsgType::NOT_ON_CHANNEL;
        }
    }
    else if (mode == 'l' && sign == '+') {
        try {
            unsigned long limit = std::stoul(param);
            if (limit > 100) {
                std::cout << "DEBUG: Limit too high (" << limit << ")\n";
				return {MsgType::NEED_MORE_PARAMS};
            }
        } catch (...) {
            return {MsgType::NEED_MORE_PARAMS};
        }
    }
    else if (mode == 'k' && sign == '+') {
        if (param.empty()) {
            return {MsgType::NEED_MORE_PARAMS};
        }
    }
    return MsgType::NONE;
}

std::pair<MsgType, std::vector<std::string>>
Channel::modeSyntaxValidator(const std::string& nick, const std::vector<std::string>& params) const {
    size_t idx = 1;
    char sign = ' ';
    while (idx < params.size()) {
        const std::string& token = params[idx];
        if (token.empty() || (token[0] != '+' && token[0] != '-')) {
            std::cout << "DEBUG: Syntax Error: Unexpected token '" << token << "'." << std::endl;
            return {MsgType::NEED_MORE_PARAMS, {nick, "MODE"}};
        }
        sign = token[0];
        for (size_t i = 1; i < token.size(); ++i) {
            char mode = token[i];
            if (!isValidChannelMode(mode) && !isValidClientMode(mode)) {
//                std::cout << "DEBUG: Unknown mode char '" << mode << "'." << std::endl;
                return {MsgType::UNKNOWN_MODE, {std::string(1, mode), nick, getName()}};
            }
            if (!channelModeRequiresParameter(mode)) continue;
            if (idx + 1 >= params.size()) {
//                std::cout << "DEBUG: Missing parameter for mode '" << mode << "'." << std::endl;
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
            ++idx; // consumed parameter
        }
        ++idx;
    }
    return {MsgType::NONE, {}};
}

 std::string Channel::getClientModePrefix(std::shared_ptr<Client> client) const {
    if (!client) {
        return ""; // Or handle as an error if a null shared_ptr is passed
    }

    // Iterate through _ClientModes to find the matching client
    for (const auto& entry : _ClientModes) {
        // Safely lock the weak_ptr and compare with the provided shared_ptr
        if (auto weakClientPtr = entry.first.lock(); weakClientPtr && weakClientPtr == client) {
            const std::bitset<config::CLIENT_NUM_MODES>& modes = entry.second.first;
            std::string prefix = "";

            // Check for operator mode ('@') first, as it takes precedence over voice ('+')
            if (modes.test(Modes::OPERATOR)) { // Assuming Modes::OPERATOR is defined in config.h
                prefix = "@"; 
            }
            // Else, check for voice mode ('+')
            /*else if (modes.test(Modes::VOICE)) { // Assuming Modes::VOICE is defined in config.h
                prefix = "+";
            }*/
            return prefix;
        }
    }
    return ""; // Client not found in this channel
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


void Channel::removeClientByNickname(const std::string& nickname) {
    std::cout << "CHANNEL: Attempting to remove client '" << nickname << "' from channel '" << _name << "' (case-sensitive).\n";

    // Iterate through _ClientModes to find the client by nickname
    // Remember, `_ClientModes` uses `weak_ptr<Client>` as keys, so we iterate and compare nicknames.
    for (auto it = _ClientModes.begin(); it != _ClientModes.end(); ++it) {
        std::shared_ptr<Client> client_sptr = it->first.lock(); // Get shared_ptr from weak_ptr

        // Check if the weak_ptr is still valid AND the nickname matches
        if (client_sptr && client_sptr->getNickname() == nickname) {
            _ClientModes.erase(it); // Remove the entry from the map
            std::cout << "CHANNEL: Successfully removed '" << nickname << "' from channel '" << _name << "'.\n";
            return; // Client found and removed, exit function
        }
    }
    std::cout << "CHANNEL: Client '" << nickname << "' not found in channel '" << _name << "' for removal.\n";
}

// Helper function to check if the channel has any members left
bool Channel::isEmpty() const {
    return _ClientModes.empty();
}

void Channel::addInvite(const std::string& nickname) {
    // Check if already invited to avoid duplicates
    if (!isInvited(nickname)) {
        _invites.push_back(nickname); // Add to the deque
        std::cout << "CHANNEL: Added '" << nickname << "' to invite list for '" << _name << "'. Current invites: " << _invites.size() << std::endl;
    } else {
        std::cout << "CHANNEL: '" << nickname << "' is already on invite list for '" << _name << "'.\n";
    }
}

bool Channel::isInvited(const std::string& nickname) const {
    // Use std::find to check if the nickname exists in the _invites deque
    return std::find(_invites.begin(), _invites.end(), nickname) != _invites.end();
}

void Channel::removeInvite(const std::string& nickname) {
    auto it = std::find(_invites.begin(), _invites.end(), nickname);
    if (it != _invites.end()) {
        _invites.erase(it); // Remove from the deque
        std::cout << "CHANNEL: Removed '" << nickname << "' from invite list for '" << _name << "'. Current invites: " << _invites.size() << std::endl;
    }
}
