#pragma once

#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

class Socket
{
	public:
		Socket();
		~Socket();

		void create();
		void setOptions();
		void setNonBlocking();
		void setIP(const std::string &hostname);
		void bindSocket(const std::string& port);
		void startListening(int maxConnections);
		void closeSocket();
		
		int getFd() const { return _socketFd; }
		std::string getIP() { return _ip; }
	private:
		int _socketFd;
		std::string _ip;
};
