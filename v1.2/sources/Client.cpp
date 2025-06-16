#include "Client.hpp"
#include "Server.hpp"
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sys/socket.h>
//#include "config.h"
#include "ServerError.hpp"
#include "SendException.hpp"
#include "epoll_utils.hpp" // reset client timer
#include "IrcMessage.hpp"
//#include <string>
#include "CommandDispatcher.hpp"
#include <ctime>


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
	_isOperator(false)      // Initialize to false
  	
{
		lastActivityTime = time(NULL);
		_ClientPrivModes.reset();
}

Client::~Client(){
	// when the client is deleted , go through clienst list of channels it 
	// belongs too, then do function remove from channel with this client 
	// getChannel().removeUser(*this); //getClient().erase();
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

// specifically adds a specific amount, not increment by 1
void Client::set_failed_response_counter(int count){
	std::cout<<"failed response counter is "
				<< _failed_response_counter
				<<"new value to be added "
				<< count
				<<std::endl;
	if ( count < 0 && _failed_response_counter == 0)
		return ;	
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

std::string Client::getfullName(){
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

void Client::set_acknowledged(){
	_acknowledged = true;
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

void Client::setDefaults(){ //todo check these are really called - or woudl we call from constructor?
	// this needs an alternative to add unique identifiers 
	// also must add to all relative containers. 
	_nickName = generateUniqueNickname();
	_username = "user_" + _nickName;
	_fullName = "real_" + _nickName;
	_isOperator = false;
	signonTime = time(NULL);
	lastActivityTime = time(NULL);
}

void Client::sendPing() {
	safeSend(_fd, "PING :localhost/r/n"); // todo should these be \r\n?
}
void Client::sendPong() {
	safeSend(_fd, "PONG :localhost/r/n"); // todo should these be \r\n?
}


bool Client::change_nickname(std::string nickname){
	_nickName.clear();
	_nickName = nickname;
	//std::cout<<"hey look its a fd = "<< fd << std::endl;
	//this->set_nickname(nickname);

//	else (0);
	return true;
}


std::string Client::getChannel(std::string channelName)
{
	auto it = _joinedChannels.find(channelName);
	//if (find(_joinedChannels.begin(), _joinedChannels.end(), channelName) != _joinedChannels.end())
	if (it != _joinedChannels.end())
	{
		std::cout<<"channel already exists on client list\n";
		return channelName;
	}
	else
		std::cout<<"channel does not exist\n";
	return "";
		//_joinedChannels.push_back(channelName);
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

/*void Client::removeSelfFromChannel()
{}*/
int Client::prepareQuit(std::deque<std::shared_ptr<Channel>>& channelsToNotify) { // Gemini corrected this &

	std::cout<<"preparing quit \n";
	int indicator = 0;
    for (auto it = _joinedChannels.begin(); it != _joinedChannels.end(); ) {
		std::cout<<"We are loooooping now  \n";
        if (auto channelPtr = it->second.lock()) {
			if (indicator == 0)
			{
				indicator = 1; // we could count how many channles are counted here ?? 
					
			}
			channelsToNotify.push_back(channelPtr);
			//_channelsToNotify.push_back(it->second);
		
            channelPtr->removeClient(_nickName);

			++it;
        } else {
            it = _joinedChannels.erase(it);  //Remove expired weak_ptrs
        }
    }
	std::cout<<"what is indicator here "<<indicator<<"\n";
	return indicator;
}

bool Client::addChannel(std::string channelName, std::shared_ptr<Channel> channel) {

	if (!channel)
		return false; // no poopoo pointers could be throw
	auto it = _joinedChannels.find(channelName);
	//if (std::find(_joinedChannels.begin(), _joinedChannels.end(), channelName) != _joinedChannels.end()) {
	if (it != _joinedChannels.end()) {
		std::cout<<"channel already exists on client list\n";
		return false;
	} else {
		std::cout<<"------------------channel ahas been added to the map of joined channles for client----------------------------------------- \n";
		std::weak_ptr<Channel> weakchannel = channel;
		_joinedChannels.emplace(channelName, weakchannel);
	}
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
    // Calculate difference between current time and last activity time
    return time(NULL) - lastActivityTime;
}

// Ensure this is called when client successfully registers
void Client::setHasRegistered() {
    _registered = true;
    signonTime = time(NULL); // Set signon time upon successful registration
    lastActivityTime = time(NULL); // Also set initial last activity time
}

// You need to ensure this is called whenever you read data from the client's socket
// For example, in your Server::handleReadEvent:
// client_ptr->updateLastActivityTime(); // Or directly set client_ptr->lastActivityTime = time(NULL);
// Add this method to Client class:
void Client::updateLastActivityTime() {
   lastActivityTime = time(NULL);
   // This might reside in server's main event loop when handling EPOLLIN events for clients
}