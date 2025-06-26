// sources/Client.cpp

#include "Client.hpp"
#include "Server.hpp" // Needed for CommandDispatcher to access Server methods
#include "Channel.hpp" // Needed for Channel interactions
#include "IrcMessage.hpp" // For IrcMessage operations
#include "config.h" // For config constants
#include <iostream> // For std::cout, std::cerr
#include <algorithm> // For std::find, std::transform
#include <cctype>    // For std::tolower, std::isprint
#include <unistd.h>  // For close
#include <ctime>     // For time(NULL)

// Constructor accepting client_fd and timer_fd (for basic creation)
Client::Client(int fd, int timerfd) :
    _fd(fd),
    _timer_fd(timerfd),
    signonTime(time(NULL)),
    lastActivityTime(time(NULL)),
    _isOperator(false),
    _msg(), // CORRECTED: Call default constructor for IrcMessage - no (fd) argument
    _isAuthenticatedByPass(false),          // Initialize to false by default (corrected: added underscore)
    _passwordRequiredForRegistration(false) // Default to false if no serverPasswordRequired is passed (corrected: added underscore)
{
    setDefaults(); // Initialize default nickname, etc.
}

// Main constructor used by Server, accepting serverPasswordRequired
Client::Client(int fd, int timerfd, bool serverPasswordRequired) :
    _fd(fd),
    _timer_fd(timerfd),
    signonTime(time(NULL)),
    lastActivityTime(time(NULL)),
    _isOperator(false),
    _msg(), // CORRECTED: Call default constructor for IrcMessage - no (fd) argument
    _isAuthenticatedByPass(false),              // Client starts unauthenticated (corrected: added underscore)
    _passwordRequiredForRegistration(serverPasswordRequired) // Initialize based on server's setting (corrected: added underscore)
{
    setDefaults(); // Initialize default nickname, etc.
}

Client::~Client() {
    // Destructor might log or perform final cleanup if needed
    std::cout << "Client FD " << _fd << " (" << _nickName << ") destroyed.\n";
}

// --- CORE GETTERS ---
int Client::getFd() const { return _fd; }
int Client::get_timer_fd() const { return _timer_fd; }
const std::string& Client::getNickname() const { return _nickName; }
std::string& Client::getNicknameRef() { return _nickName; }
std::string Client::getClientUname() const { return _username; }
std::string Client::getFullName() const { return _fullName; } // Implementation for getFullName
const std::string& Client::getUsername() const { return _username; }
const std::string& Client::getHostname() const { return _hostname; }
bool Client::isOperator() const { return _isOperator; }
bool Client::getChannelCreator() const { return _channelCreator; }
long Client::getIdleTime() const { return time(NULL) - lastActivityTime; }
time_t Client::getSignonTime() const { return signonTime; }
bool Client::getQuit() const { return _quit; }
bool Client::getHasSentCap() const { return _hasSentCap; }
bool Client::getHasSentNick() const { return _hasSentNick; }
bool Client::getHasSentUser() const { return _hasSentUser; }
bool Client::getHasRegistered() const { return _registered; }
bool Client::get_acknowledged() const { return _acknowledged; }
bool Client::get_pendingAcknowledged() const { return _pendingAcknowledged; }
int Client::get_failed_response_counter() const { return _failed_response_counter; }
const std::map<std::string, std::weak_ptr<Channel>>& Client::getJoinedChannels() const { return _joinedChannels; }

// --- PASSWORD-RELATED GETTERS/SETTERS (referencing underscored private members) ---
bool Client::getIsAuthenticatedByPass() const { return _isAuthenticatedByPass; }
bool Client::getPasswordRequiredForRegistration() const { return _passwordRequiredForRegistration; }
void Client::setIsAuthenticatedByPass(bool status) { _isAuthenticatedByPass = status; }

// --- SETTERS ---
void Client::set_failed_response_counter(int count) { _failed_response_counter = count; }
void Client::setQuit() { _quit = true; }
void Client::set_nickName(const std::string newname) { _nickName = newname; } // Changed clear() then assign, just assign
void Client::setHasSentCap() { _hasSentCap = true; }
void Client::setHasSentNick() { _hasSentNick = true; }
void Client::setHasSentUser() { _hasSentUser = true; }

// Refined setHasRegistered implementation (from Step 4)
void Client::setHasRegistered() {
    if (_hasSentNick && _hasSentUser) {
        // Check password condition:
        // If password is NOT required OR password IS required AND authenticated, then proceed.
        if (!_passwordRequiredForRegistration || _isAuthenticatedByPass) { // Corrected: added underscores
            if (!_registered) {
                _registered = true;
                signonTime = time(NULL);
                lastActivityTime = time(NULL);
                std::cout << "Client FD " << _fd << " (" << _nickName << ") is now REGISTERED.\n";
            }
        } else {
            std::cout << "Client FD " << _fd << " (" << _nickName << ") NOT REGISTERED: Password required but not authenticated.\n";
        }
    } else {
        std::cout << "Client FD " << _fd << " (" << _nickName << ") NOT REGISTERED: Missing NICK (" << _hasSentNick << ") or USER (" << _hasSentUser << ").\n";
    }
}


void Client::setNickname(const std::string& nickname) { _nickName = nickname; } // Simplified setNickname
void Client::setOldNick(std::string oldnick) { _oldNick = oldnick; }
void Client::setFullName(const std::string& fullname) { _fullName = fullname; }
void Client::setClientUname(std::string uname) { _username = uname; }
void Client::setHostname(const std::string& hostname) { _hostname = hostname; }
void Client::setOperator(bool status) { _isOperator = status; }
void Client::setChannelCreator(bool onoff) { _channelCreator = onoff; }
void Client::set_acknowledged() { _acknowledged = true; }

// setDefaults method implementation
void Client::setDefaults() {
    _nickName = "guest"; // Default nickname, will be overwritten by NICK command
    _username = "";
    _fullName = "";
    _hostname = "localhost"; // Default hostname
    _isOperator = false; // Not an operator by default
    _ClientPrivModes.reset(); // Clear all client modes initially
    _pendingAcknowledged = false;
    _acknowledged = false;
    _failed_response_counter = 0;
    // Do NOT set _hasSentCap, _hasSentNick, _hasSentUser, _registered, _isAuthenticatedByPass, _passwordRequiredForRegistration here
    // as these are controlled by specific IRC commands or server settings.
}

// --- CLIENT MODE MANAGEMENT ---
void Client::setMode(clientPrivModes::mode mode) { _ClientPrivModes.set(mode); }
void Client::unsetMode(clientPrivModes::mode mode) { _ClientPrivModes.reset(mode); }
bool Client::hasMode(clientPrivModes::mode mode) { return _ClientPrivModes.test(mode); }
std::string Client::getCurrentModes() const {
    std::string modes_str = "";
    if (_ClientPrivModes.test(clientPrivModes::INVISABLE)) modes_str += "i";
    if (_ClientPrivModes.test(clientPrivModes::RECEIVES_NOTICES_MODE)) modes_str += "w";
    // Add other modes as needed
    return modes_str;
}
std::string Client::getPrivateModeString() {
    return getCurrentModes(); // Assuming this is the same as getCurrentModes for now
}
bool Client::isValidClientMode(char modeChar) {
    return std::find(clientPrivModes::clientPrivModeChars.begin(), clientPrivModes::clientPrivModeChars.end(), modeChar) != clientPrivModes::clientPrivModeChars.end();
}

// --- CHANNEL MANAGEMENT FOR CLIENT ---
bool Client::addChannel(const std::string& channelName, const std::shared_ptr<Channel>& channel) {
    if (_joinedChannels.find(channelName) == _joinedChannels.end()) {
        _joinedChannels[channelName] = channel;
        return true;
    }
    return false;
}
std::string Client::getChannel(std::string channelName) {
    return channelName; // Placeholder, adjust if you need actual channel lookup
}
void Client::removeJoinedChannel(const std::string& channel_name) {
    _joinedChannels.erase(channel_name);
}
void Client::addJoinedChannel(const std::string& channel_name, std::shared_ptr<Channel> channel_ptr) {
    _joinedChannels[channel_name] = channel_ptr;
}
void Client::clearJoinedChannels() {
    _joinedChannels.clear();
}

// --- MESSAGE & COMMAND HANDLING ---
IrcMessage& Client::getMsg() { return _msg; }
void Client::sendMessage(const std::string& message) {
    _msg.queueMessage(message + "\r\n");
}
bool Client::isMsgEmpty() {
    return _msg.getQue().empty();
}
void Client::appendIncomingData(const char* buffer, size_t bytes_read) {
    _read_buff.append(buffer, bytes_read);
}
bool Client::extractAndParseNextCommand() {
    return _msg.parseMessage(_read_buff); // Assuming IrcMessage::parseMessage handles buffer consumption
}
void Client::executeCommand(const std::string& command) {
    std::cout << "Client::executeCommand called (should be handled by CommandDispatcher).\n";
}
void Client::setCommandMap(Server &server) {
    std::cout << "Client::setCommandMap called (not typically used with central dispatcher).\n";
}

// --- PING/PONG & ACTIVITY ---
void Client::sendPing() {
    _msg.queueMessage("PING :" + _hostname + "\r\n");
}
void Client::sendPong() {
    _msg.queueMessage("PONG :" + _hostname + "\r\n");
}
void Client::updateLastActivityTime() {
    lastActivityTime = time(NULL);
}

// --- NICKNAME CHANGE LOGIC ---
bool Client::change_nickname(std::string nickname) {
    _nickName = nickname;
    return true;
}

// --- DISCONNECTION ---
int Client::prepareQuit(std::deque<std::shared_ptr<Channel>>& channelsToNotify) {
    _quit = true;
    return 0;
}
void Client::disconnect(const std::string& reason) {
    _quit = true;
    std::cerr << "Client FD " << _fd << " (" << _nickName << ") marked for disconnection: " << reason << std::endl;
}
