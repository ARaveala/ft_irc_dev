#pragma once
#include <string>
#include <map>
#include <memory>

#include "user.hpp" // can this be handled withoout including the whole hpp

// connection registration
// https://modern.ircdocs.horse/#connection-registration

class Server {
	private:
		int _port;
		// int _server_socket;
		int _fd;
		std::string _password;
		// num of clients 
		// int = fd of client client = client object
		std::map<int, std::shared_ptr<User>> _users; //unordered map?
		// num of channels 
		// string = name of channel channle = channel object
		// std::map<std::string, std::shared_ptr<Channel>> chanels
	public:
		Server();
		Server(int port, std::string password);
		~Server();

		// setters
		//void set_port(int const port);
		//void set_password(std::string const password);
		void create_user(int fd);
		// add channel
		// remove client
		// remove channel
		void setFd(int fd);
		// getters
		// find matching client to fd 
		// get client returns client
		// get channel
		// 
		int getPort() const;
		int getFd() const;
		// read to private buffer returns string to parse. 
		// message handling 
		std::string get_password() const;
		//std::string recieve_message(); // send to message handling

};

