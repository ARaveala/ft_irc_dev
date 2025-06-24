#pragma once
#include <string>

// https://modern.ircdocs.horse/#client-to-server-protocol-structure
// Names of IRC entities (clients, servers, channels) are casemapped


class User {
	private:
		int _fd;
		char buffer[1024];// to recv() too
		//std::string _ip;
		std::string _Nickname;
		std::string _Fullname;
		// pointer to current channel object ?
		// list of channels user is in 
	public:
		User();
		User(int fd);
		~User();
		std::string recieve_message(int fd);
		
};
