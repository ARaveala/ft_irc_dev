#include "Client.hpp"
#include "Server.hpp" // Make sure Server.hpp defines the generateUniqueNickname() function, or you move it to general_utilities.hpp
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sys/socket.h>
#include "ServerError.hpp"
#include "SendException.hpp"
#include "IrcMessage.hpp"
#include "CommandDispatcher.hpp"
#include <ctime> // For time(NULL)
#include "IrcResources.hpp" // For Modes::clientModeChars, Modes::channelModeChars (assuming this is where they are defined)


// Client::Client(){} never let this exist - is that stated in the hpp?
// Yes, a delete default constructor in .hpp (Client() = delete;) prevents this.

// Consolidated Constructor
// Ensure that the Client.hpp file also has this constructor declaration:
// Client(int fd, int timerfd, bool serverPasswordRequired);
Client::Client(int fd, int timer_fd, bool serverPasswordRequired) :
    _fd(fd),
    _timer_fd(timer_fd),
    _failed_response_counter(0),
    signonTime(0),          // Initialize to 0, set on full registration
    lastActivityTime(time(NULL)), // Initial activity time is set upon connection
    _ClientPrivModes(),     // std::bitset constructor initializes all bits to 0
    _channelCreator(false),
    _quit(false),
    _hasSentCap(false),
    _hasSentNick(false),
    _hasSentUser(false),    // Corrected typo here
    _registered(false),
    isAuthenticatedByPass(false), // Initially false, set true upon successful PASS
    passwordRequiredForRegistration(serverPasswordRequired), // Set based on server's global config
    _isOperator(false)      // Initialize to false
{
    // No need for lastActivityTime = time(NULL); here, as it's in the initializer list
    // No need for _ClientPrivModes.reset(); here, as default constructor handles it
    setDefaults(); // Call your defaults to set initial nickname, username, fullname
}

Client::~Client(){
    // When the client is deleted, iterate through the client's list of channels it
    // belongs to, and call a function to remove this client from those channels.
    // This process should be handled by the Server during client disconnection/QUIT.
    // The `prepareQuit` function already does this by adding channels to `channelsToNotify`.
}

int Client::getFd(){
    return _fd;
}

bool Client::get_acknowledged(){
    return _acknowledged;
}

bool Client::get_pendingAcknowledged(){
    return _pendingAcknowledged;
}

int Client::get_failed_response_counter(){
    return _failed_response_counter;
}

void Client::set_failed_response_counter(int count){
    if ( count < 0 && _failed_response_counter == 0)
        return ;
    if ( count == 0){
        _failed_response_counter = 0;
        return;
    }
    _failed_response_counter += count;
}

int Client::get_timer_fd(){
    return _timer_fd;
}

std::string Client::getNickname() const{
    return _nickName;
}

std::string& Client::getNicknameRef(){
    return _nickName;
}

std::string Client::getClientUname(){
    return _username;
}

std::string Client::getFullName() const {
    return _fullName;
}

const std::string& Client::getHostname() const {
    return _hostname;
}

void Client::setHostname(const std::string& hostname) {
    _hostname = hostname;
}

const std::map<std::string, std::weak_ptr<Channel>>& Client::getJoinedChannels() const {
    return _joinedChannels;
}

std::string Client::getPrivateModeString(){
    std::string activeModes = "+";
    // Assuming Modes::clientModeChars is correctly defined and accessible
    // Note: You had Modes::channelModeChars.size() here, should be clientModeChars.size()
    for (size_t i = 0; i < Modes::clientModeChars.size(); ++i) {
        if (_ClientPrivModes.test(i)) {
            activeModes += Modes::clientModeChars[i];
        }
    }
    return activeModes;
}

void Client::appendIncomingData(const char* buffer, size_t bytes_read) {
    _read_buff.append(buffer, bytes_read);
    // std::cout << "Debug - buffer after appending : [" << _read_buff << "]" << std::endl;
}

bool Client::extractAndParseNextCommand() {
    size_t crlf_pos = _read_buff.find("\r\n");
    if (crlf_pos == std::string::npos) {
        return false; // No complete message yet
    }
    std::string full_message = _read_buff.substr(0, crlf_pos);
    _read_buff.erase(0, crlf_pos + 2); // Consume the message from the buffer
    if (_msg.parse(full_message) == true)
    {
        std::cout << "token parser shows true \n";
    }
    else
        std::cout << "token parser shows false \n";

    return true;
}

void Client::set_acknowledged(){
    _acknowledged = true;
}

void Client::setDefaults(){
    // This function sets initial, default values for a new client.
    // This is where a unique identifier for the nickname should be generated.
    _nickName = generateUniqueNickname(); // Ensure this function is globally accessible or passed in
    _username = "user_" + _nickName;
    _fullName = "real_" + _nickName;
    _isOperator = false; // Default to not an operator
    // signonTime and lastActivityTime are set upon full registration or activity.
    // If you set them here, they'll be overwritten in setHasRegistered() anyway.
}

bool Client::change_nickname(std::string nickname){
    // No need for _nickName.clear() before assignment
    _nickName = nickname;
    return true;
}

std::string Client::getChannel(std::string channelName)
{
    auto it = _joinedChannels.find(channelName);
    if (it != _joinedChannels.end()) {
        return channelName;
    }
    std::cout << "channel does not exist\n"; // Use cerr for errors
    return "";
}

std::string Client::getCurrentModes() const {
    std::string activeModes = "+";
    // Assuming Modes::clientModeChars is correctly defined and accessible
    for (size_t i = 0; i < Modes::clientModeChars.size(); ++i) {
        if (_ClientPrivModes.test(i)) {
            activeModes += Modes::clientModeChars[i];
        }
    }
    return activeModes;
}

int Client::prepareQuit(std::deque<std::shared_ptr<Channel>>& channelsToNotify) {
    std::cout << "preparing quit \n";
    int indicator = 0;
    // Iterate through a copy of keys or use a reverse iterator if order matters
    // or simply iterate and erase carefully.
    for (auto it = _joinedChannels.begin(); it != _joinedChannels.end(); ) {
        std::cout << "We are loooooping now  \n";
        if (auto channelPtr = it->second.lock()) { // Check if the weak_ptr is still valid
            if (indicator == 0) {
                indicator = 1;
            }
            channelsToNotify.push_back(channelPtr); // Add channel to a list for server to notify
            channelPtr->removeClient(_nickName); // Remove client from channel's list
            ++it; // Move to the next element
        } else {
            it = _joinedChannels.erase(it); // Remove expired weak_ptrs
        }
    }
    std::cout << "what is indicator here " << indicator << "\n";
    return indicator;
}

bool Client::addChannel(const std::string& channelName, const std::shared_ptr<Channel>& channel) {
    if (!channel)
        return false;
    auto it = _joinedChannels.find(channelName);
    if (it != _joinedChannels.end()) {
        std::cout << "channel already exists on client list\n"; // Use cerr for errors
        return false;
    }
    std::cout << "------------------channel has been added to the map of joined channels for client----------------------------------------- \n";
    std::weak_ptr<Channel> weakchannel = channel;
    _joinedChannels.emplace(channelName, weakchannel); // Use emplace for efficiency
    return true;
}

bool Client::isOperator() const {
    return _isOperator;
}

void Client::setOperator(bool status) {
    _isOperator = status;
}

time_t Client::getSignonTime() const {
    return signonTime; // This value is set when client registers
}

long Client::getIdleTime() const {
    return time(NULL) - lastActivityTime;
}

// This should be called by the Server when the client successfully completes NICK, USER, and optionally PASS
void Client::setHasRegistered() {
    // A client is registered if:
    // 1. They have sent a NICK command (_hasSentNick is true).
    // 2. They have sent a USER command (_hasSentUser is true).
    // 3. If the server requires a password (passwordRequiredForRegistration is true),
    //    they must also have successfully sent the PASS command (isAuthenticatedByPass is true).
    if (_hasSentNick && _hasSentUser) {
        // If a password is required, check if it has been authenticated.
        // If password is NOT required OR password IS required AND authenticated, then proceed.
        if (!passwordRequiredForRegistration || isAuthenticatedByPass) {

            if (!_registered) { // Only perform registration steps once to avoid re-registration issues
                _registered = true;
                signonTime = time(NULL); // Set signon time upon full registration
                lastActivityTime = time(NULL); // Also set initial last activity time upon registration

                // The Server's CommandDispatcher will handle sending welcome messages (001-004, MOTD)
                // after checking client->getHasRegistered() in its main command processing loop.
                // No need to send messages from here directly, as it's a client-side state update.
            }
        }
        // If _hasSentNick && _hasSentUser are true, but the password isn't authenticated (and required),
        // then _registered remains false. The CommandDispatcher will handle sending ERR_PASSWDMISMATCH.
    }
}

// This should be called by the Server whenever data is read from the client's socket
void Client::updateLastActivityTime() {
   lastActivityTime = time(NULL);
}

void Client::removeJoinedChannel(const std::string& channel_name) {
    size_t removed_count = _joinedChannels.erase(channel_name);
    if (removed_count > 0) {
        std::cout << "CLIENT: " << _nickName << " removed from its _joinedChannels list for channel '" << channel_name << "'.\n";
    } else {
        std::cerr << "CLIENT ERROR: " << _nickName << " not found in its _joinedChannels for channel '" << channel_name << "' during removal attempt.\n";
    }
}

// jw
void Client::sendMessage(const std::string& message) {
    _msg.queueMessage(message + "\r\n");
}

// jw
void Client::disconnect(const std::string& reason) {
    _quit = true;
    std::cerr << "Client FD " << _fd << " disconnecting: " << reason << std::endl;
    // The Server's main loop or handleQuit will actually close the socket and remove the client.
}