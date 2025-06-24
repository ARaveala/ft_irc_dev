
#include "Server.hpp"

Server::Server() {/*default contructor*/ }

Server::Server(int port , std::string password) {
	_port = port;
	_password = password;
	//_users
	//empty array/map of user objects
	//empty array/map of channel objects
}
//SETTERS

int Server::getFd() const {
	return _fd;
}
/*void Server::create_user(int fd) {
	//std::string name = user + itoa(fd);
	//User name(fd);
	// add name to map (fd name);
}*/

void Server::setFd(int fd) {
	_fd = fd;
}

// getters
int Server::getPort() const{
	return _port;
}


std::string Server::get_password() const{
	return _password;
}
Server::~Server() { 
	// delete array/map of users
	// delete array/map of channels	
	/*deconstructor*/
}