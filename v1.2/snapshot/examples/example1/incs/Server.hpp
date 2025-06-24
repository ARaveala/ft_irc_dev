#pragma once

#include "Client.hpp"
#include "Channel.hpp"
#include "Socket.hpp"
#include "Poll.hpp"
#include <csignal>
#include <iostream>
#include <cstring>
#include <string>
#include <algorithm>
#include <sstream>
#include <memory>
#include <set>
#include <cstdlib>
#include <netdb.h>
#define MAX_CONNECTIONS 10

class Server
{
	public:
		Server(const std::string &, const std::string &);
		~Server();
		
		// Server.cpp
		void startServer();
		void run();
		void createNewClient();
		void handleNewData(int fd, int index);
		void processCommand(std::string command, int fd, int index);
		
		// SignalHandler.cpp
		static void handle_signal(int sig);
		
		// ServerUtils.cpp
		void clearClient(int clinetFd, int index);
		bool validateClientRegistration(int fd, int index);

		// Helpers.cpp
		void sendResponse(std::string msg, int fd);
		void sendError(std::string msg, int fd);
		std::string getNickname(int fd);
		std::shared_ptr<Client> getClient(const std::string& nick);
		int searchByUsername(std::string user);
		int searchByNickname(std::string nick);
		bool isInChannel(const std::shared_ptr<Client>& client, Channel *channel);
		int getChannelIndex(std::string name);

		// ./commands/..
		void capLs(int fd, int index);
		Channel *createChannel(const std::string &name, const std::shared_ptr<Client> creator, int fd);
		Channel *findChannel(const std::string &name);
		void invite(std::string buf, int fd, int index);
		void join(std::string buf, int fd, int index);
		void kick(std::string buf, int fd, int index);
		void list(std::string buf, int fd, int index);
		void mode(std::string buf, int fd, int index);
		void nick(std::string buf, int fd, int index);
		void pass(std::string buf, int fd, int index);
		void ping(std::string buf, int fd, int index);
		void privmsg(std::string buf, int fd, int index);
		void topic(std::string buf, int fd, int index);
		void user(std::string buf, int fd, int index);
		void quit(int fd, int index);

		bool isRunning() const { return _isRunning; }
		void sendChannelCreationResponse(const std::string &nick, const std::string &user, const std::string &channel, int fd);

	private:
		static bool signal;
		bool _isRunning;

		const std::string _port;
		const std::string _password;

		std::vector<std::shared_ptr<Client>> _clients;
		std::vector<Channel *> _channels;
		std::string _test;

		Socket _socket;
		Poll _poll;
};
