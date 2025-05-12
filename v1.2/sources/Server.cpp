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
	_Clients.erase(client_fd);
	_epollEventMap.erase(client_fd);
	_fd_to_nickname.erase(client_fd);
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

std::map<int, std::string>& Server::get_fd_to_nickname() {
	return _fd_to_nickname;
}

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
	_nickname_to_fd.clear();
	_fd_to_nickname.clear();
	nickname_to_fd.clear();
	fd_to_nickname.clear();
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

// definition of illegal nick_names ai
std::set<std::string> const Server::_illegal_nicknames = {
    "ping", "pong", "server", "root", "nick", "services", "god"
};

// we should enum values or alike or we can just send the correct error message straight from here ?
// check_and_set_nickname definition
bool Server::check_and_set_nickname(std::string nickname, int fd) {

    // 1. Check for invalid characters
	// check nickname exists
    if (nickname.empty()) {
         std::cout << "#### Nickname '" << nickname << "' rejected for fd " << fd << ": Empty." << std::endl;
        return false;
    }
	// check nickname is all lowercase
    for (char c : nickname) {
         if (!std::islower(static_cast<unsigned char>(c))) {
             std::cout << "#### Nickname '" << nickname << "' rejected for fd " << fd << ": Contains non-lowercase chars." << std::endl;
             return false;
         }
    }

    std::string processed_nickname = nickname; // TODO do we need this allocation? no i dont think so 

    // 2. check legality
    if (_illegal_nicknames.count(processed_nickname) > 0) {
        std::cout << "#### Nickname '" << nickname << "' rejected for fd " << fd << ": Illegal name." << std::endl;
        return false;
    }

    // check if nickname exists for anyone
    auto nick_it = _nickname_to_fd.find(processed_nickname);
	if (nick_it != _nickname_to_fd.end()) {
        // Nickname exists. Is it the same Client trying to set their current nick?
        if (nick_it->second == fd) {
            // FD already head requested nickname.
            std::cout << "#### Nickname '" << nickname << "' for fd " << fd << ": Already set. No change needed." << std::endl;
            return true;
        } else {
            // Nickname is taken by some cunt else
            std::cout << "#### Nickname '" << nickname << "' rejected for fd " << fd << ": Already taken by fd " << nick_it->second << "." << std::endl;
            return false;
        }
    }

    // Check if the FD has an old nickname with an iterator
    auto fd_it = _fd_to_nickname.find(fd);

    if (fd_it != _fd_to_nickname.end()){
        // This FD already has a nickname. We need to remove the old one from both maps.
		// find out nickname
        std::string old_nickname = fd_it->second;
        std::cout << "#### FD " << fd << " had old nickname '" << old_nickname << "', removing entries." << std::endl;

        // Remove the old nickname -> fd entry using the old nickname as key
        // Use erase(key) which is safe even if the key somehow wasn't found
        _nickname_to_fd.erase(old_nickname);

        // Remove the old fd -> nickname entry using the iterator we already have
        _fd_to_nickname.erase(fd_it);

        std::cout << "#### Removed old nickname '" << old_nickname << "' for fd " << fd << "." << std::endl;

    } else {
        // FD does not currently have a nickname.
         std::cout << "#### FD " << fd << " does not have an existing nickname." << std::endl;
    }

    // udpate both maps
    std::cout << "#### Setting nickname '" << nickname << "' for fd " << fd << "." << std::endl;
    _nickname_to_fd.insert({processed_nickname, fd});
    _fd_to_nickname.insert({fd, processed_nickname});

    std::cout << "#### Nickname '" << nickname << "' set successfully for fd " << fd << "." << std::endl;
    return true;
}

std::string Server::get_nickname(int fd) const {
     auto it = _fd_to_nickname.find(fd);
     if (it != _fd_to_nickname.end()) {
         return it->second; // Return the nickname
     }
     return "";
}

int Server::get_fd(const std::string& nickname) const {
     std::string processed_nickname = to_lowercase(nickname);

     auto it = _nickname_to_fd.find(processed_nickname);
     if (it != _nickname_to_fd.end()) {
         return it->second; // Return the fd
     }
     return -1; // nickname not found
}

void Server::remove_fd(int fd) {
    // Call this when a client disconnects to clean up their nickname entry
    auto fd_it = _fd_to_nickname.find(fd);
    if (fd_it != _fd_to_nickname.end()) {

        // find the nickname from the fd
		std::string old_nickname = fd_it->second;
        
		std::cout << "#### Removing fd " << fd << " and nickname '" << old_nickname << "' due to disconnect." << std::endl;

        // Remove from nickname_to_fd map using the nickname
        _nickname_to_fd.erase(old_nickname);
        // Remove from fd_to_nickname map using the iterator
        _fd_to_nickname.erase(fd_it);

        std::cout << "#### Cleaned up entries for fd " << fd << "." << std::endl;
    } else {
         std::cout << "#### No nickname found for fd " << fd << " upon disconnect." << std::endl;
    }
}

void Server::send_message(std::shared_ptr<Client> client)
{
	int fd = client->getFd();
	while (!client->isMsgEmpty()) {
		std::string msg = client->getMsg().getQueueMessage();
		std::cout<<"checking the message from que before send ["<< msg <<"]\n";
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