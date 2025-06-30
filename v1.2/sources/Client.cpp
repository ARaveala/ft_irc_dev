#include "Client.hpp"
#include "Server.hpp"
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sys/socket.h>
//#include "config.h"
#include "ServerError.hpp"
#include "SendException.hpp"
#include "IrcMessage.hpp"
//#include <string>
#include "CommandDispatcher.hpp"
#include <ctime>

static std::string toLower(const std::string& input) {
    std::string output = input;
    std::transform(output.begin(), output.end(), output.begin(),
	[](unsigned char c) { return std::tolower(c); });
    return output;
}
// Client::Client(){} never let this exist - is that stated in the hpp?

Client::Client(int fd, int timer_fd) :
										_fd(fd),
										_timer_fd(timer_fd),
										_failed_response_counter(0),
										signonTime(0),          // Initialize to 0, set on registration
										lastActivityTime(0),    // Initialize to 0, set on registration and updated on activity
										_channelCreator(false),
										_quit(false),
										_hasSentCap(false),
										_hasSentNick(false),
										// _hasSentUSer(false),
										_registered(false),
										_isOperator(false) {      // Initialize to false
	lastActivityTime = time(NULL);
	_ClientPrivModes.reset();
}

Client::~Client(){
	// when the client is deleted , go through clienst list of channels it 
	// belongs too, then do function remove from channel with this client 
	// getChannel().removeUser(*this); //getClient().erase();
}

int Client::getFd(){return _fd;}
bool Client::get_acknowledged(){return _acknowledged;}
bool Client::get_pendingAcknowledged(){	return _pendingAcknowledged;}
int Client::get_failed_response_counter(){return _failed_response_counter;}
void Client::set_acknowledged(){ _acknowledged = true;}
time_t Client::getSignonTime() const { return signonTime;} // This value is set when client registers
long Client::getIdleTime() const { return time(NULL) - lastActivityTime;} // Calculate difference between current time and last activity time
const std::map<std::string, std::weak_ptr<Channel>>& Client::getJoinedChannels() const {return _joinedChannels;}
void Client::setOperator(bool status) { _isOperator = status;}
bool Client::isOperator() const {return _isOperator;}

// specifically adds a specific amount, not increment by 1
void Client::set_failed_response_counter(int count){
	/*std::cout<<"failed response counter is "
	<< _failed_response_counter
	<<"new value to be added "
	<< count
	<<std::endl;*/
	if ( count < 0 && _failed_response_counter == 0)
	return ;
	if ( count == 0){
		_failed_response_counter = 0;
		return;
	}	
				_failed_response_counter += count;
			}
			
			int Client::get_timer_fd(){return _timer_fd;}
			std::string Client::getNickname() const{return _nickName;}
			std::string& Client::getNicknameRef(){return _nickName;}
			std::string Client::getClientUname(){return _username;}
			std::string Client::getfullName(){return _fullName;}
			
			const std::string& Client::getHostname() const {return _hostname;}
			
			void Client::setHostname(const std::string& hostname) {_hostname = hostname;}


std::string Client::getPrivateModeString(){
	std::string activeModes = "+";
    for (size_t i = 0; i < Modes::channelModeChars.size(); ++i) {
        if (_ClientPrivModes.test(i)) {
            activeModes += Modes::clientModeChars[i];
        }
    }
    return activeModes;
}


void Client::appendIncomingData(const char* buffer, size_t bytes_read) {
	_read_buff.append(buffer, bytes_read);
	// std::cout << "Debug - buffer after apening : [" << _read_buff << "]" << std::endl;
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
		std::cout<<"token parser shows true \n";
	}
	else
		std::cout<<"token parser shows false \n";

	return true;
}


/*void Client::set_pendingAcknowledged(bool onoff){
	_pendingAcknowledged = onoff;
}*/
/**
 * @brief Reads using recv() to a char buffer as data recieved from the socket
 * comes in as raw bytes, std		void sendPing();
 * @return FAIL an empty string or throw 
 * SUCCESS the char buffer converted to std::string
 */


/*void Client::setChannelCreator(bool onoff) {
	
}*/

/*void Client::setDefaults(){ //todo check these are really called - or woudl we call from constructor?
	_isOperator = false;
	signonTime = time(NULL);
	lastActivityTime = time(NULL);
}*/

bool Client::change_nickname(std::string nickname){
	_nickName.clear();
	_nickName = nickname;
	return true;
}

std::string Client::getChannel(std::string channelName){
	auto it = _joinedChannels.find(channelName);
	if (it != _joinedChannels.end()) {
		return channelName;
	}
	std::cout<<"channel does not exist\n";
	return "";
}

std::string Client::getCurrentModes() const {
	std::string activeModes = "+";
    for (size_t i = 0; i < Modes::clientModeChars.size(); ++i) {
        if (_ClientPrivModes.test(i)) {
            activeModes += Modes::clientModeChars[i];
        }
    }
    return activeModes;
}

bool Client::addChannel(const std::string& channelName, const std::shared_ptr<Channel>& channel) {
	if (!channel)
		return false;
	auto it = _joinedChannels.find(channelName);
	if (it != _joinedChannels.end()) {
		std::cout<<"Client::addChannel -- channel already exists on client list\n";
		return false;
	}
	std::cout<<"Client::addChannel -- channel has been added to the map of joined channels for client \n";
	std::weak_ptr<Channel> weakchannel = channel;
	_joinedChannels.emplace(channelName, weakchannel);
	return true;
}

// Ensure this is called when client successfully registers
void Client::setHasRegistered() {
    _registered = true;
    signonTime = time(NULL); // Set signon time upon successful registration
    lastActivityTime = time(NULL); // Also set initial last activity time
}

void Client::removeJoinedChannel(const std::string& channel_name) {
    // Assuming 'channel_name' is already in lowercase, consistent with storage.
    size_t removed_count = _joinedChannels.erase(toLower(channel_name));
    if (removed_count > 0) {
        LOG_NOTICE("removeJoinedChannel: " + _nickName + " removed from its _joinedChannels list for channel " + channel_name);
    } else {
		LOG_ERROR("removeJoinedChannel: " + _nickName + " not found in its _joinedChannels for channel " + channel_name + " during removal attempt");
    }
}

// === Example for addJoinedChannel (you'll call this when a client successfully JOINs) ===
/*void Client::addJoinedChannel(const std::string& channel_name, std::shared_ptr<Channel> channel_ptr) {
    // Assuming 'channel_name' is already in lowercase.
    _joinedChannels[channel_name] = channel_ptr; // Store a weak_ptr to the channel
    std::cout << "CLIENT: " << _nickName << " added channel '" << channel_name << "' to its _joinedChannels list.\n";
}*/

// // === Example for getJoinedChannels (useful for QUIT command processing) ===
// const std::map<std::string, std::weak_ptr<Channel>>& Client::getJoinedChannels() const {
//     return _joinedChannels;
// }


