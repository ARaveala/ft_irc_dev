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

Client::Client(){}

Client::Client(int fd, int timer_fd) : _fd(fd), _timer_fd(timer_fd){}

Client::~Client(){}

int Client::getFd(){
	return _fd;
}

bool Client::get_acknowledged(){
	return _acknowledged;
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

std::string Client::getNickname(){
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
		bytes_read = recv(fd, buffer, sizeof(buffer) , MSG_DONTWAIT); // last flag makes recv non blocking 
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
			setReadBuff(buffer);
			if (_read_buff.find("\r\n") != std::string::npos) {
				server.resetClientTimer(_timer_fd, config::TIMEOUT_CLIENT);
				set_failed_response_counter(-1);
				_msg.handle_message(*this, _read_buff, server);	
				_read_buff.clear();
				return ; // this might be a bad idea
			} else
				std::cout<<"something has been read and a null added"<<std::endl;
		}
	}
}


void Client::setDefaults(int num){
	// this needs an alternative to add unique identifiers 
	// also must add to all relative containers. 
	_nickName = "anon"; //+ num;
	_ClientName = "Clientanon" + num;
	_fullName = "fullanon" + num;
}

void Client::sendPing() {
	safeSend(_fd, "PING :server/r/n");
}
void Client::sendPong() {
	safeSend(_fd, "PONG :server/r/n");
}


bool Client::change_nickname(std::string nickname, int fd){
	_nickName.clear();
	_nickName = nickname;
	std::cout<<"hey look its a fd = "<< fd << std::endl;
	//this->set_nickname(nickname);

//	else (0);
	return true;
}


