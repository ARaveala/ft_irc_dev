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
				handle_message(_read_buff, server);	
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


#include "IrcResources.hpp"
/**
 * @brief this functions task is to find what command we have been sent and to deligate the
 * handling of that command to respective functions
 * 
 * @param Client 
 * @param message 
 * @param server 
 */
// could we do a map of commands to fucntion pointers to handle this withouts ifs ?
void Client::handle_message(const std::string message, Server& server)
{
	_msg.parse(message);
	
	/*if (getCommand() == "QUIT")
	{
		std::cout<<"QUIT called removing client \n";
		server.remove_Client(server.get_event_pollfd(), client_fd);
		return ;
	}*/
	if (_msg.getCommand() == "NICK"){
		if(_msg.check_and_set_nickname(_msg.getParam(0), getFd())) {
			_msg.prep_nickname_msg(getNicknameRef(), _msg.getQue(), server.getBroadcastQueue());
		}
		else {
			_msg.prep_nickname_inuse(_nickName,  _msg.getQue());
		}

        // todo check nick against list
        // todo map of Clientnames
        // Client creation - add name to list in server
        // Client deletion - remove name from list in server

	}
	if (_msg.getCommand() == "PING"){
		sendPong();
		std::cout<<"sending pong back "<<std::endl;
		//Client->set_failed_response_counter(-1);
		//resetClientTimer(Client->get_timer_fd(), config::TIMEOUT_CLIENT);
	}
	if (_msg.getCommand() == "PONG"){
		std::cout<<"------------------- we recived pong inside message handling haloooooooooo"<<std::endl;
	}

    if (_msg.getCommand() == "JOIN"){
		if (server.channelExists(_msg.getParam(0)) < 1) // will param 0 be correct 
		{
			server.createChannel(_msg.getParam(0));
			server.get_Channel(_msg.getParam(0))->addClient(server.get_Client(_fd));

			// this is join channel, it sends the confrim message to join
			_msg.prep_join_channel(_msg.getParam(0), _nickName,  _msg.getQue());
			// set defaults what are they 
		}

		// handle join
		// ischannel
		// if (!ischannel) , createChannel(), setChannelDefaults() updateChannalconts()?, confirmOperator()
		// else if (ischannel), isinvite(), hasinvite(), ChannelhasPaswd(), clientHasPasswd()/passwrdMatch(),
		// hasBan(), joinChannel() updateChannalconts()
		//		
		/**
		 * @brief checks
		 * look through list of channel names to see if channel exists // std::map<std::string, Channel*> channels
		 * 	if doesnt exist - create it with default settings // what are default settings?
		 * set max size? // is the is default channel std::int _maxSize also flag -n 
		 * set current number of clients in side the channel // channel std::int _nClients
		 * add channel to vector of channels client has joined //  <Client> _joinedchannels
		 * add current client to channel operator // channel std::string _operator OR
		 * adjust bitset map
		 * 
		 * if does exist - loop through and find if channel is invite only // channel std::bool _inviteOnly channel std::set _currentUsers, _invitedUsers
		 * if it is invite only, does client have invite isnide channel.
		 * 
		 * is it password protected.
		 * if it is password protected, did user provide password. if not then user can not enter
		 * if yes, does password match
		 * 
		 *  is client banned from channel.
		 * 
		 * assuming checks passed, client can now join channel
		 * add client to list of clients on channel // channel > list of clients
		 *  if this clients is first on the channel, set the flag to -o // channel > who is -o? can be only one.

		 * 
		 */
    /*if (getCommand() == "KICK") {
        
    }

    JOIN
    PART
    LEAVE
    TOPIC
    NAMES
    LIST
    INVITE
    PARAMETER NICKNAME

*/
	}

	getMsg().printMessage(getMsg());
}
