#include "Server.hpp"
#include <sys/types.h>
#include <unistd.h>
#include <string.h> //strlen
#include <iostream> // testing with cout
#include <sys/socket.h>
#include <sys/epoll.h>
#include "epoll_utils.hpp"
#include "Client.hpp"
#include <map>
#include <memory> // shared pointers
#include "config.h"
#include "ServerError.hpp"
#include "Channel.hpp"
//#include "SendException.hpp"
#include <algorithm> // find_if
//#include <optional> // nullopt , signifies absence
class ServerException;

Server::Server(){
	std::cout << "#### Server instance created." << std::endl;
}

Server::Server(int port , std::string password) {
	_port = port;
	_password = password;
	
}

// ~~~SETTERS
void Server::setFd(int fd){
	_fd = fd;}

void Server::set_signal_fd(int fd){
	_signal_fd = fd;
}

// note we may want to check here for values below 0
void Server::set_event_pollfd(int epollfd){
	_epoll_fd = epollfd;
}

void Server::set_client_count(int val){
	_client_count += val;
}

void Server::set_current_client_in_progress(int fd){
	_current_client_in_progress = fd;
}

// ~~~GETTERS
int Server::getFd() const {
	 return _fd;
}

int Server::get_signal_fd() const{
	return _signal_fd;
}

int Server::get_client_count() const{
	return _client_count;
}

int Server::getPort() const{
	return _port;
}

int Server::get_event_pollfd() const{
	return _epoll_fd;
}

int Server::get_current_client_in_progress() const{
	return _current_client_in_progress;
}

/**
 * @brief Here a client is accepted , error checked , socket is adusted for non-blocking
 * the client fd is added to the epoll and then added to the Client map. a welcome message
 * is sent as an acknowlegement message back to irssi.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
void Server::create_Client(int epollfd) {
 	// Handle new incoming connection
	int client_fd = accept(getFd(), nullptr, nullptr);
 	if (client_fd < 0) {
		throw ServerException(ErrorType::ACCEPT_FAILURE, "debuggin: create Client");
	} else {
		int flag = 1;
		int buf_size = 1024;

		setsockopt(client_fd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size));
		setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, (void*)&flag, sizeof(flag));	
		make_socket_unblocking(client_fd);
		//setsockopt(client_fd, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag)); 
		setup_epoll(epollfd, client_fd, EPOLLIN | EPOLLOUT | EPOLLET); // not a fan of epollet
		int timer_fd = setup_epoll_timer(epollfd, config::TIMEOUT_CLIENT);
		// errro handling if timer_fd failed
		// create an instance of new Client and add to server map
		_Clients[client_fd] = std::make_shared<Client>(client_fd, timer_fd);
		_timer_map[timer_fd] = client_fd;
		std::cout << "New Client created , fd value is  == " << _Clients[client_fd]->getFd() << std::endl;

// WELCOME MESSAGE
		set_current_client_in_progress(client_fd);

		if (!_Clients[client_fd]->get_acknowledged()){
			// send message back so server dosnt think we are dead this has to be handled through epoll too damn it 
			send(client_fd, IRCMessage::welcome_msg, strlen(IRCMessage::welcome_msg), 0);
			_Clients[client_fd]->set_acknowledged();
		}

		set_client_count(1);
		
		_Clients[client_fd]->setDefaults(get_client_count());
	}
}

/**
 * @brief removing a singular client
 * 
 * 
 * @param epollfd 
 * @param client_fd 
 * 
 * @note we are calling get client multple times here , we could try to work past that 
 * by just sending in the client once as a param
 */
void Server::remove_Client(int epollfd, int client_fd) {
	close(client_fd);
	epoll_ctl(epollfd, EPOLL_CTL_DEL, client_fd, 0);
	close(get_Client(client_fd)->get_timer_fd());
	epoll_ctl(epollfd, EPOLL_CTL_DEL, get_Client(client_fd)->get_timer_fd(), 0);
	// channel.getClient().erase();
	_Clients.erase(client_fd);
	_epollEventMap.erase(client_fd);
	
	
	//////_fd_to_nickname.erase(client_fd);
	
	
	//_nickname_to_fd.erase(client_fd);

	//_epollEventMap.erase(client_fd);
	//std::map<int, struct epoll_event> _epollEventMap;
	//std::map<std::string, int> _nickname_to_fd;
	//std::map<int, std::string> _fd_to_nickname;
	_client_count--;
	std::cout<<"client has been removed"<<std::endl;
}


/**
 * @brief to find the Client object in the Clients array
 *  and return a pointer to it
 *
 * @param fd the active fd
 * @return Client* , shared pointers are a refrence themselves
 */
std::shared_ptr<Client> Server::get_Client(int fd) {
	for (std::map<int, std::shared_ptr<Client>>::iterator it = _Clients.begin(); it != _Clients.end(); it++) {
		if (it->first == fd)
			return it->second;
	}
	throw ServerException(ErrorType::NO_Client_INMAP, "can not get_Client()");
}


std::map<int, std::shared_ptr<Client>>& Server::get_map() {
	return _Clients;
}

/*std::map<int, std::string>& Server::get_fd_to_nickname() {
	return _fd_to_nickname;
}*/

std::string Server::get_password() const {
	return _password;
}

// ERROR HANDLING INSIDE LOOP
void Server::handle_client_connection_error(ErrorType err_type) {
	switch (err_type){
		case ErrorType::ACCEPT_FAILURE:
			break;
		case ErrorType::EPOLL_FAILURE_1: {
			send(_current_client_in_progress, IRCMessage::error_msg, strlen(IRCMessage::error_msg), 0);
			close(_current_client_in_progress);
			_current_client_in_progress = 0;
			break;
		} case ErrorType::SOCKET_FAILURE: {
			send(_current_client_in_progress, IRCMessage::error_msg, strlen(IRCMessage::error_msg), 0);
			close(_current_client_in_progress);
			_current_client_in_progress = 0;
			break;
		} default:
			send(_current_client_in_progress, IRCMessage::error_msg, strlen(IRCMessage::error_msg), 0);
			close(_current_client_in_progress);
			_current_client_in_progress = 0;
			std::cerr << "server Unknown error occurred" << std::endl;
			break;
	}
}

void Server::shutdown() {
	// close all sockets
	for (std::map<int, std::shared_ptr<Client>>::iterator it = _Clients.begin(); it != _Clients.end(); it++){
		close(it->first);
	}
	for (std::map<int, int>::iterator timerit = _timer_map.begin(); timerit != _timer_map.end(); timerit++)
	{
		close(timerit->first);
	}
	// close server socket
	close(_fd);
	close(_signal_fd);
	close(_epoll_fd);
	for (std::map<int, std::shared_ptr<Client>>::iterator it = _Clients.begin(); it != _Clients.end(); it++){
		it->second.reset();
		it->second->getMsg().clearQue();
	}
	_timer_map.clear();
	_Clients.clear();
	// delete channels
	_epollEventMap.clear();
	
	// these need to be called on all instnaces or in teh client destrcutors
	/*_nickname_to_fd.clear();
	_fd_to_nickname.clear();
	nickname_to_fd.clear();
	fd_to_nickname.clear();*/
	//_illegal_nicknames.clear();

	_server_broadcasts.clear();
	//_illegal_nicknames.clear();

	std::cout<<"server shutdown completed"<<std::endl;
}

Server::~Server(){
	shutdown();
}

/**
 * @brief checks if a timer fd was activated in epoll
 * updates failed response counter and sends a ping to 
 * check if there is an active client, if not , the client is removed.  
 * 
 * @param fd 
 * @return true if timer fd was not an active event 
 * @return false if timer fd was an active event
 */
bool Server::checkTimers(int fd) {
	auto timerit = _timer_map.find(fd);
	if (timerit == _timer_map.end()) {
		return true;
	}
	int client_fd = timerit->second;
    auto clientit = _Clients.find(client_fd);
    if (clientit == _Clients.end()) {
		return false;
	} // did not find client on the list eek
    if (clientit->second->get_failed_response_counter() == 3) {
		std::cout<<"timer sup removing client \n";
		remove_Client(_epoll_fd, client_fd);
        _timer_map.erase(fd);
        return false;
    }
    std::cout << "should be sending ping onwards " << std::endl;
    clientit->second->sendPing();
    clientit->second->set_failed_response_counter(1);
    resetClientTimer(timerit->first, config::TIMEOUT_CLIENT);
	return false;
}


void Server::send_message(std::shared_ptr<Client> client)
{
	int fd = client->getFd();
	while (!client->isMsgEmpty()) {
		std::string msg = client->getMsg().getQueueMessage();
		std::cout<<"checking the message from que before send ["<< msg <<"] and the fd = "<<fd<<"\n";
		ssize_t bytes_sent = send(fd, msg.c_str(), msg.length(), 0); //safesend
		if (bytes_sent == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				std::cout<<"triggering the in the actual message conts???????????????????????????????????????????\n";
				continue;  //no more space, stop writing
			}
			
			else perror("send error");
		}
		if (bytes_sent > 0) {
			usleep(5000); //wait incase we are going too fast and so sends dont complete
			client->getMsg().removeQueueMessage(); 

		}
		/*if (bytes_sent == 0)
		{
			

		}*/
		if (!client->isMsgEmpty()) {
			struct epoll_event event;
			event.events = EPOLLIN | EPOLLOUT | EPOLLET;
			event.data.fd = fd;
			epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &event);
		}
	}

}

/*void send_channel_broadcast(ChannelManager& manager)
{
	manager.broadcast();
// fucntion to loop through joined channels and channels in channel manager
// find all channles clients has joined and send similar to server message to all clienst in the
// channel
}*/

void Server::send_server_broadcast()
{
	while(!_server_broadcasts.empty())
	{
		std::string msg = _server_broadcasts.front();

		for (auto& pair : _Clients ) {
			ssize_t bytes_sent = send(pair.first, msg.c_str(), msg.length(), 0);
			if (bytes_sent == -1) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					std::cout<<"triggering the conts???????????????????????????????????????????\n";
					continue;  //No more space, stop writing
				}
				else perror("send error");
			}
			/*if (bytes_sent > 0) {
			
			}*/
		}
		_server_broadcasts.pop_front();  //remove sent message
	}
}

//channel realted 

bool Server::channelExists(const std::string& channelName) const {
    return _channels.count(channelName) > 0;
}
//
void Server::createChannel(const std::string& channelName) {
    if (_channels.count(channelName) == 0){
		_channels.emplace(channelName, std::make_shared<Channel>(channelName));
        std::cout << "Channel '" << channelName << "' created." << std::endl;
        // fill clientsend buffer 
		return ;
    }
    std::cerr << "Error: Channel '" << channelName << "' already exists" << std::endl;
}

std::shared_ptr<Channel> Server::get_Channel(std::string ChannelName) {
	for (std::map<std::string, std::shared_ptr<Channel>>::iterator it = _channels.begin(); it != _channels.end(); it++) {
		if (it->first == ChannelName)
			return it->second;
	}
	throw ServerException(ErrorType::NO_Client_INMAP, "can not get_Client()");
}
/*Channel Server::getChannel(const std::string& channelName){
    auto it = _channels.find(channelName);
    if(it != _channels.end()){
        return it->second;
    }
    return nullptr; // Channel not found
}*/


/**
 * @brief when using weak pointers, when the original object goes, the wek pointer may 
 * be left dangling, of course its better to remove it methodically, but this function might have value
 * for valgrind testing if we have an issue
 * void cleanUpExpiredClients(std::map<std::weak_ptr<Client>, std::bitset<4>>& _ClientModes) {
    for (auto it = _ClientModes.begin(); it != _ClientModes.end(); ) {
        if (it->first.expired()) {
            it = _ClientModes.erase(it);  
        } else {
            ++it;
        }
    }
}
 * 
 */