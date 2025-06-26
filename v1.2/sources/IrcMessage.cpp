// sources/IrcMessage.cpp

#include "IrcMessage.hpp"
// #include "Client.hpp" // No longer directly needed here for nickname management
// #include "Server.hpp" // No longer directly needed here for nickname management
#include "config.h" // For MSG_TYPE_NUM and other constants
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

// Static member function definition for to_lowercase
std::string IrcMessage::to_lowercase(const std::string& s) {
    std::string lower_s = s;
    std::transform(lower_s.begin(), lower_s.end(), lower_s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return lower_s;
}

// Removed the static initialization of _illegal_nicknames.
// Nickname validation (including illegal names and uniqueness) should be handled by the Server.
// const std::set<std::string> IrcMessage::_illegal_nicknames = {
//     "anonymous", "root", "admin", "moderator", "operator", "sysop", "ircop", "services" // Example illegal nicks
// };


// Constructor
IrcMessage::IrcMessage() :
    _bytesSentForCurrentMessage(0),
    _activeMsg(MsgType::NONE)
{
    _msgState.reset(); // Initialize all bits to 0
}

// Destructor
IrcMessage::~IrcMessage() {}

// Parses raw incoming data from the client's buffer.
// Extracts one complete IRC message (up to \r\n) and populates _prefix, _command, _params.
bool IrcMessage::parseMessage(std::string& raw_data) {
    size_t crlf_pos = raw_data.find("\r\n");
    if (crlf_pos == std::string::npos) {
        return false; // No complete message yet
    }

    std::string message_line = raw_data.substr(0, crlf_pos);
    raw_data.erase(0, crlf_pos + 2); // Consume the message and CRLF

    _prefix.clear();
    _command.clear();
    _params.clear();
    _paramsList.clear(); // Clear _paramsList if it's used for temporary storage during parsing

    std::stringstream ss(message_line);
    std::string token;

    // Parse prefix (optional, starts with ':')
    if (!message_line.empty() && message_line[0] == ':') { // Add empty check
        ss >> token; // Read the prefix token
        _prefix = token.substr(1); // Store without ':'
    }

    // Parse command (mandatory)
    ss >> _command;
    std::transform(_command.begin(), _command.end(), _command.begin(), ::toupper); // Commands are case-insensitive

    // Parse parameters
    std::string remaining_line;
    std::getline(ss, remaining_line); // Get the rest of the line (potentially with leading space)

    std::stringstream ps(remaining_line);
    std::string param_token;

    // Skip leading space if present from getline
    if (!remaining_line.empty() && remaining_line[0] == ' ') {
        ps.ignore(); // Consume the first space
    }

    while (ps >> param_token) {
        if (!param_token.empty() && param_token[0] == ':') {
            // This is the last parameter, can contain spaces
            // Get the rest of the stream content, including spaces
            _params.push_back(param_token.substr(1) + ps.str().substr(ps.tellg()));
            break; // Stop parsing, rest is part of this last param
        }
        _params.push_back(param_token);
    }
    return true;
}

// Queues an outgoing message to be sent to the client.
void IrcMessage::queueMessage(const std::string& message) {
    _messageQue.push_back(message); // Message should already include CRLF
}

// Retrieves the current message from the queue for sending (mutable reference).
std::string& IrcMessage::getQueueMessage() {
    return _messageQue.front();
}

// Removes the message at the front of the queue after it has been sent.
void IrcMessage::removeQueueMessage() {
    if (!_messageQue.empty()) {
        _messageQue.pop_front();
        _bytesSentForCurrentMessage = 0; // Reset offset for the next message
    }
}

// Updates the number of bytes sent for the current message.
void IrcMessage::advanceCurrentMessageOffset(size_t bytes_sent) {
    _bytesSentForCurrentMessage += bytes_sent;
}

// Returns the number of remaining bytes in the current message.
size_t IrcMessage::getRemainingBytesInCurrentMessage() const {
    if (_messageQue.empty()) {
        return 0;
    }
    return _messageQue.front().length() - _bytesSentForCurrentMessage;
}

// Resets the offset for the current message.
void IrcMessage::resetCurrentMessageOffset() {
    _bytesSentForCurrentMessage = 0;
}

// Getters for parsed components
const std::string& IrcMessage::getPrefix() const {
    return _prefix;
}

const std::string& IrcMessage::getCommand() const {
    return _command;
}

const std::vector<std::string>& IrcMessage::getParams() const {
    return _params;
}

// Returns the message as a raw string (for debugging/reconstruction)
std::string IrcMessage::toRawString() const {
    std::string raw;
    if (!_prefix.empty()) {
        raw += ":" + _prefix + " ";
    }
    raw += _command;
    for (size_t i = 0; i < _params.size(); ++i) {
        if (i == _params.size() - 1 && _params[i].find(' ') != std::string::npos) {
            raw += " :" + _params[i]; // Last param with spaces
        } else {
            raw += " " + _params[i];
        }
    }
    return raw;
}

// Setters for prefix and command
void IrcMessage::setPrefix(const std::string& prefix) { _prefix = prefix; }
void IrcMessage::setCommand(const std::string& command) { _command = command; }

// Get a specific parameter by index (const version)
const std::string IrcMessage::getParam(unsigned long index) const {
    if (index < _params.size()) {
        return _params[index];
    }
    return ""; // Or throw an out_of_range exception
}

// Queue message at the front (for urgent messages like PING responses)
void IrcMessage::queueMessageFront(const std::string& msg) {
    _messageQue.push_front(msg);
}

// Implementation for getQue()
const std::deque<std::string>& IrcMessage::getQue() const {
    return _messageQue;
}

// Clears the message queue
void IrcMessage::clearQue() {
    _messageQue.clear();
}

// Gets a message parameter by int index (overlaps with getParam, but matching decl)
const std::string IrcMessage::getMsgParam(int index) const {
    if (index >= 0 && static_cast<size_t>(index) < _params.size()) {
        return _params[index];
    }
    return "";
}

// Changes a token param by index (referencing _paramsList, adjust if _params is the source)
void IrcMessage::changeTokenParam(int index, const std::string& newValue) {
    if (index >= 0 && static_cast<size_t>(index) < _paramsList.size()) {
        _paramsList[index] = newValue;
    } else if (index >= 0 && static_cast<size_t>(index) < _params.size()) {
        // If _paramsList is not actively used for mutable token modification,
        // you might want to modify _params directly.
        _params[index] = newValue;
    }
}

// Returns a reference to the parameters vector
const std::vector<std::string>& IrcMessage::getMsgParams() {
    return _params;
}

// --- Removed Nickname Validation and Management from IrcMessage ---
// These functions are now expected to be handled by the Server class or a dedicated utility.
/*
bool IrcMessage::isValidNickname(const std::string& nick) {
    if (nick.empty() || nick.length() > config::MAX_NICK_LENGTH) {
        return false;
    }
    if (!std::isalpha(nick[0]) && nick[0] != '[' && nick[0] != ']' && nick[0] != '\\' && nick[0] != '`' && nick[0] != '_') {
        return false;
    }
    for (size_t i = 1; i < nick.length(); ++i) {
        if (!std::isalnum(nick[i]) && nick[i] != '-' && nick[i] != '[' && nick[i] != ']' && nick[i] != '\\' && nick[i] != '`' && nick[i] != '_') {
            return false;
        }
    }
    if (_illegal_nicknames.count(to_lowercase(nick))) { // to_lowercase is now IrcMessage::to_lowercase
        return false;
    }
    return true;
}

MsgType IrcMessage::check_nickname(std::string nickname, int fd, const std::map<std::string, int>& nick_to_fd) {
    if (nickname.empty()) {
        return MsgType::ERR_NONICKNAMEGIVEN;
    }
    if (!isValidNickname(nickname)) {
        return MsgType::ERR_ERRONEUSNICKNAME;
    }
    if (nick_to_fd.count(to_lowercase(nickname)) && nick_to_fd.at(to_lowercase(nickname)) != fd) {
        return MsgType::ERR_NICKNAMEINUSE; // Corrected enum value
    }
    return MsgType::NONE;
}

std::map<int, std::string>& IrcMessage::get_fd_to_nickname() {
    return _fd_to_nickname; // This map is not a member of IrcMessage. Remove this function.
}

void IrcMessage::remove_fd(int fd, std::map<int, std::string>& fd_to_nick) {
    // This logic should be in Server.
}

std::string IrcMessage::get_nickname(int fd, std::map<int, std::string>& fd_to_nick) const {
    // This logic should be in Server.
    return "";
}

int IrcMessage::get_fd(const std::string& nickname) const {
    // This logic should be in Server.
    return -1;
}
*/

// Sets the active message type and parameters for sending (for MessageBuilder)
void IrcMessage::setType(MsgType msg, std::vector<std::string> sendParams) {
    _activeMsg = msg;
    _params = sendParams; // Overwrite _params with sendParams for outgoing messages
    _msgState.reset(); // Clear any previous state
    _msgState.set(static_cast<size_t>(msg)); // Set the bit corresponding to the message type
}

// Clears all message-related state.
void IrcMessage::clearAllMsg() {
    // Removed clearing of nickname maps, as IrcMessage does not own them.
    // _nickname_to_fd.clear();
    // _fd_to_nickname.clear();
    // nickname_to_fd.clear();
    // fd_to_nickname.clear();

    _paramsList.clear();
    _params.clear();
    _msgState.reset();
    _activeMsg = MsgType::NONE;
    _prefix.clear();
    _command.clear();
    _messageQue.clear(); // Ensure message queue is also cleared
    _bytesSentForCurrentMessage = 0; // Reset offset
}

// Prints the parsed message for debugging.
void IrcMessage::printMessage(const IrcMessage& msg) {
    std::cout << "IrcMessage Details:\n";
    std::cout << "  Prefix: [" << msg.getPrefix() << "]\n";
    std::cout << "  Command: [" << msg.getCommand() << "]\n";
    std::cout << "  Parameters:\n";
    for (const auto& param : msg.getParams()) {
        std::cout << "    - [" << param << "]\n";
    }
    std::cout << "  Active MsgType: " << static_cast<int>(msg._activeMsg) << "\n";
    std::cout << "  Message Queue Size: " << msg.getQue().size() << "\n";
}

// Checks if a specific message type is active (bitset check)
bool IrcMessage::isActive(MsgType type) {
    return _msgState.test(static_cast<size_t>(type));
}

// Returns the currently active message type
MsgType IrcMessage::getActiveMessageType() const {
    return _activeMsg;
}
