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
Client::Client(){}

Client::Client(int fd, int timer_fd) : _fd(fd), _timer_fd(timer_fd){}

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

std::string Client::getClientName(){
	return _ClientName;
}

std::string Client::getfullName(){
	return _fullName;
}

std::string Client::getReadBuff() {
	return _read_buff;

}

void Client::setReadBuff(const std::string& buffer) {
	_read_buff += buffer;
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


void Client::receive_message(int fd, Server& server) {
	char buffer[config::BUFFER_SIZE];
	ssize_t bytes_read = 0;
	while (true)
	{
		memset(buffer, 0, sizeof(buffer));
		bytes_read = recv(fd, buffer, config::BUFFER_SIZE , MSG_DONTWAIT); // last flag makes recv non blocking 	
		std::cout << "Debug - Raw Buffer Data: [" << std::string(buffer, bytes_read) << "]" << std::endl;
		if (bytes_read < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return ;
			}
			perror("recv gailed");
			return ;
		}
		if (bytes_read == 0) {
			throw ServerException(ErrorType::CLIENT_DISCONNECTED, "debug :: recive_message");
		}
		else if (bytes_read > 0) {
			
			//std::cout << "Debug - Current bytes read: [" << bytes_read << "]" << std::endl;

			//std::cout << "Debug - Current Read Buffer: [" << _read_buff << "]" << std::endl;
			//_read_buff += std::string(buffer, bytes_read);

			setReadBuff(buffer);
			//std::cout << "Debug - Read Buffer: [" << _read_buff << "]" << std::endl;
			// add a timer of some kind here to give server time to catch up if big message
			if (_read_buff.find("\r\n") != std::string::npos) {
				server.resetClientTimer(_timer_fd, config::TIMEOUT_CLIENT);
				set_failed_response_counter(-1);
				//server.handle_message(_read_buff, server);	
				CommandDispatcher::handle_message(_read_buff, server, _fd);	
				_read_buff.clear();
				return ; // this might be a bad idea
			} else
				std::cout<<"something has been read and a null added"<<std::endl;
		}
	}
}

/*void Client::setChannelCreator(bool onoff) {
	
}*/

void Client::setDefaults(){
	// this needs an alternative to add unique identifiers 
	// also must add to all relative containers. 
	_nickName = generateUniqueNickname();
	_ClientName = "Clientanon";
	_fullName = "fullanon";
}

void Client::sendPing() {
	safeSend(_fd, "PING :server/r/n");
}
void Client::sendPong() {
	safeSend(_fd, "PONG :server/r/n");
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
int Client::prepareQuit(std::deque<std::string>& channelsToNotify) {
    //std::string quitMessage = CLIENT_QUIT(_nickName);
	// std::vector<int> fds;
	std::cout<<"preparing quit \n";
	int indicator = 0;
    for (auto it = _joinedChannels.begin(); it != _joinedChannels.end(); ) {
		std::cout<<"We are loooooping now  \n";
        if (auto channelPtr = it->second.lock()) {
			if (indicator == 0)
			{
				indicator = 1; // we could count how many channles are counted here ?? 
					
			}
			channelsToNotify.push_back(it->first);
			
			// const std::vector<int>& newFds = channelPtr->getAllfds();
			// fds.insert(fds.end(), newFds.begin(), newFds.end());
			
			//fds.push_back() channelPtr->getAllfds();

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
/*void Client::removeChannel(std::string channelName) {
	if (std::find(_joinedChannels.begin(), _joinedChannels.end(), channelName) != _joinedChannels.end())
	{
		_joinedChannels.pop_front();
		std::cout<<"channel already exists on client list\n";
	}
	else
		std::cout<<"channel does not exist\n";
		//_joinedChannels.push_back(channelName);

}*/