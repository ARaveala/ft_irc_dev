#include <iostream>
#include <bitset>
#include "config.h" // Assuming config.h is in includes/
#include <algorithm>
#include <cctype>
#include <ctime> // For time(NULL)

#include "IrcResources.hpp" // For MsgType enum if used here
#include "Channel.hpp"
#include "Client.hpp" // For Client class

// Helper function to convert string to lowercase (if not globally available)
static std::string toLower(const std::string& input) {
    std::string output = input;
    std::transform(output.begin(), output.end(), output.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return output;
}

Channel::Channel(const std::string& channelName)
    : _name(channelName),
      _topic(""), // Initialize topic as empty
      _password(""), // Initialize password as empty
      _topicSetter(""), // Initialize topic setter as empty
      _topicSetTime(0) // Initialize topic set time to 0
{
	std::cout << "Channel '" << _name << "' created.\n";
	_ChannelModes.reset();  //set all modes to off (0)
    // By default, topic settable by anyone, no key, no limit, not invite-only.
    // If you want topic protection by default:
    _ChannelModes.set(Modes::TOPIC); // Enable topic protection by default
    _topicSetTime = time(NULL); // Set creation time as topic set time initially
}

Channel::~Channel() {
    std::cout << "Channel '" << _name << "' destroyed.\n";
}

const std::string& Channel::getName() const {
    return _name;
}

const std::string& Channel::getTopic() const {
    return _topic;
}

const unsigned long& Channel::getClientCount() const {
    return _clientCount;
}

std::vector<int> Channel::getAllfds() const {
	std::vector<int> fds;
	for (const auto& entry : _ClientModes) {
		if (auto clientPtr = entry.first.lock()) {  // Convert weak_ptr to shared_ptr safely
			fds.push_back(clientPtr->getFd());  // Retrieve FD directly from Client object
		}
	}
	return fds;
}

const std::string Channel::getAllNicknames() const {
	std::string list;
	for (const auto& entry : _ClientModes) {
		if (auto clientPtr = entry.first.lock()) {
			list += clientPtr->getNickname() + " ";
		}
	}
	if (!list.empty()) {
		list.pop_back(); // Remove trailing space
	}
	return list;
}

std::weak_ptr<Client> Channel::getWeakPtrByNickname(const std::string& nickname) {
    for (const auto& entry : _ClientModes) {
        if (auto clientPtr = entry.first.lock()) {
            if (toLower(clientPtr->getNickname()) == toLower(nickname)) {
                return entry.first; // Return the weak_ptr if nickname matches
            }
        }
    }
    return std::weak_ptr<Client>(); // Return empty weak_ptr if not found
}

// Implement getAllClients() as requested by Server.cpp
const std::map<std::weak_ptr<Client>, std::pair<std::bitset<config::CLIENT_NUM_MODES>, int>, WeakPtrCompare>& Channel::getAllClients() const {
    return _ClientModes;
}


std::bitset<config::CLIENT_NUM_MODES>& Channel::getClientModes(const std::string nickname) {
    for (auto& entry : _ClientModes) {
        if (auto clientPtr = entry.first.lock()) {
            if (toLower(clientPtr->getNickname()) == toLower(nickname)) {
                return entry.second.first; // Return reference to the bitset
            }
        }
    }
    // Handle error: Client not found. This should ideally not happen if calls are validated.
    // For now, returning a static empty bitset for safety, but consider throwing or logging.
    static std::bitset<config::CLIENT_NUM_MODES> empty_modes;
    return empty_modes;
}

bool Channel::CheckChannelMode(Modes::ChannelMode comp) const {
    return _ChannelModes.test(comp);
}

std::string Channel::getChannelModeString() const {
    std::string modes_str = "+";
    if (_ChannelModes.test(Modes::USER_LIMIT)) modes_str += "l";
    if (_ChannelModes.test(Modes::INVITE_ONLY)) modes_str += "i";
    if (_ChannelModes.test(Modes::KEY_NEEDED)) modes_str += "k";
    if (_ChannelModes.test(Modes::TOPIC)) modes_str += "t";
    if (modes_str == "+") return ""; // Return empty if no modes are set
    return modes_str;
}

const std::string& Channel::getTopicSetter() const {
    return _topicSetter;
}

std::time_t Channel::getTopicTime() const {
    return _topicSetTime;
}


// Channel mode management
void Channel::setMode(Modes::ChannelMode mode, bool enable) {
    if (enable) {
        _ChannelModes.set(mode);
    } else {
        _ChannelModes.reset(mode);
    }
}

void Channel::setClientMode(const std::string& nickname, Modes::ClientMode mode, bool enable) {
    for (auto& entry : _ClientModes) {
        if (auto clientPtr = entry.first.lock()) {
            if (toLower(clientPtr->getNickname()) == toLower(nickname)) {
                if (enable) {
                    entry.second.first.set(mode);
                } else {
                    entry.second.first.reset(mode);
                }
                std::cout << "Channel: Set client " << nickname << " mode " << mode << " to " << (enable ? "true" : "false") << " in channel " << _name << "\n";
                return;
            }
        }
    }
    std::cerr << "Error: Client " << nickname << " not found in channel " << _name << " to set mode.\n";
}

const std::string& Channel::getKey() const {
    return _password;
}

void Channel::setKey(const std::string& key) {
    _password = key;
}

// Utility methods
std::string Channel::getCurrentModes() const {
    std::string modes_str = "+";
    // Check channel modes and append their characters
    if (_ChannelModes.test(Modes::USER_LIMIT)) modes_str += "l";
    if (_ChannelModes.test(Modes::INVITE_ONLY)) modes_str += "i";
    if (_ChannelModes.test(Modes::KEY_NEEDED)) modes_str += "k";
    if (_ChannelModes.test(Modes::TOPIC)) modes_str += "t";

    // This method typically returns channel modes. If it's meant for client modes, adjust.
    return modes_str;
}

// Client management in channel
std::optional<std::pair<MsgType, std::vector<std::string>>> Channel::canClientJoin(const std::string& nickname, const std::string& password) {
    if (_ChannelModes.test(Modes::INVITE_ONLY) && !isInvited(nickname)) {
        // Corrected MsgType enum member
        return std::make_optional(std::make_pair(MsgType::ERR_INVITEONLYCHAN, std::vector<std::string>{nickname, _name}));
    }
    if (_ChannelModes.test(Modes::KEY_NEEDED) && _password != password) {
        // Corrected MsgType enum member
        return std::make_optional(std::make_pair(MsgType::ERR_BADCHANNELKEY, std::vector<std::string>{nickname, _name}));
    }
    // Corrected signedness comparison: cast _ClientLimit to unsigned long
    if (_ClientLimit > 0 && _clientCount >= static_cast<unsigned long>(_ClientLimit)) {
        // Corrected MsgType enum member
        return std::make_optional(std::make_pair(MsgType::ERR_CHANNELISFULL, std::vector<std::string>{nickname, _name}));
    }
    return std::nullopt; // Client can join
}

int Channel::addClient(std::shared_ptr <Client> client) {
    if (client && !isClientInChannel(client->getNickname())) {
        _ClientModes[client] = {std::bitset<config::CLIENT_NUM_MODES>(), client->getFd()};
        if (_clientCount == 0) { // First client becomes operator
            _ClientModes[client].first.set(Modes::OPERATOR);
            std::cout << "Channel: " << client->getNickname() << " is now operator of " << _name << "\n";
        }
        _clientCount++;
        std::cout << "Channel: " << client->getNickname() << " joined " << _name << ". Current clients: " << _clientCount << "\n";
        return 0; // Success
    }
    std::cerr << "Error: Client already in channel or null client pointer.\n";
    return -1; // Failure
}

void Channel::setTopic(const std::string& newTopic, const std::string& setter_nickname) {
    _topic = newTopic;
    _topicSetter = setter_nickname;
    _topicSetTime = time(NULL);
    std::cout << "Channel: Topic for " << _name << " set to '" << _topic << "' by " << _topicSetter << " at " << _topicSetTime << "\n";
}

bool Channel::removeClient(const std::string& nickname) {
    for (auto it = _ClientModes.begin(); it != _ClientModes.end(); ++it) {
        if (auto clientPtr = it->first.lock()) {
            if (toLower(clientPtr->getNickname()) == toLower(nickname)) {
                _ClientModes.erase(it);
                _clientCount--;
                std::cout << "Channel: " << nickname << " removed from " << _name << ". Current clients: " << _clientCount << "\n";
                // If the channel is now empty, it should be removed from the server's list.
                // This function signals that by returning true.
                return _clientCount == 0;
            }
        }
    }
    std::cerr << "Error: Client " << nickname << " not found in channel " << _name << " to remove.\n";
    return false; // Client not found or not removed
}

void Channel::removeClientByNickname(const std::string& nickname) {
    removeClient(nickname); // Re-use the existing logic
}


bool Channel::isClientInChannel(const std::string& nickname) const {
    for (const auto& entry : _ClientModes) {
        if (auto clientPtr = entry.first.lock()) {
            if (toLower(clientPtr->getNickname()) == toLower(nickname)) {
                return true;
            }
        }
    }
    return false;
}

bool Channel::isClientOperator(const std::string& nickname) const {
    for (const auto& entry : _ClientModes) {
        if (auto clientPtr = entry.first.lock()) {
            if (toLower(clientPtr->getNickname()) == toLower(nickname)) {
                return entry.second.first.test(Modes::OPERATOR);
            }
        }
    }
    return false;
}

bool Channel::isValidChannelMode(char modeChar) const {
    for (char c : Modes::channelModeChars) {
        if (c == modeChar) return true;
    }
    return false;
}

bool Channel::isValidClientMode(char modeChar) const {
    // Corrected namespace: clientModeChars is in clientPrivModes namespace
    for (char c : clientPrivModes::clientPrivModeChars) {
        if (c == modeChar) return true;
    }
    return false;
}

bool Channel::channelModeRequiresParameter(char modeChar) const {
    return (modeChar == 'k' || modeChar == 'l' || modeChar == 'o' || modeChar == 'v');
}

std::pair<MsgType, std::vector<std::string>> Channel::initialModeValidation( const std::string& ClientNickname, size_t paramsSize) {
    // This function likely needs more context of what it's validating.
    // Placeholder logic.
    if (paramsSize < 1) { // Example: if mode command requires at least one parameter
        // Corrected MsgType enum member
        return {MsgType::ERR_NEEDMOREPARAMS, {ClientNickname, "MODE"}};
    }
    return {MsgType::NONE, {}};
}

std::pair<MsgType, std::vector<std::string>> Channel::modeSyntaxValidator( const std::string& requestingClientNickname, const std::vector<std::string>& params ) const {
    // This function likely needs more context of what it's validating.
    // Placeholder logic.
    if (params.empty()) {
        // Corrected MsgType enum member
        return {MsgType::ERR_NEEDMOREPARAMS, {requestingClientNickname, "MODE"}};
    }
    return {MsgType::NONE, {}};
}

bool Channel::isEmpty() const {
    return _clientCount == 0;
}

void Channel::addInvited(const std::string& nickname) {
    // Check if already invited to avoid duplicates
    if (!isInvited(nickname)) {
        _invites.push_back(nickname); // Add to the deque
        std::cout << "CHANNEL: Added '" << nickname << "' to invite list for '" << _name << "'. Current invites: " << _invites.size() << "\n";
    } else {
        std::cout << "CHANNEL: '" << nickname << "' is already on invite list for '" << _name << "'.\n";
    }
}

bool Channel::isInvited(const std::string& nickname) const {
    // Use std::find to check if the nickname exists in the _invites deque
    return std::find(_invites.begin(), _invites.end(), nickname) != _invites.end();
}

void Channel::removeInvited(const std::string& nickname) {
    auto it = std::find(_invites.begin(), _invites.end(), nickname);
    if (it != _invites.end()) {
        _invites.erase(it); // Remove from the deque
        std::cout << "CHANNEL: Removed '" << nickname << "' from invite list for '" << _name << "'.\n";
    } else {
        std::cout << "CHANNEL: '" << nickname << "' not found on invite list for '" << _name << "'.\n";
    }
}
