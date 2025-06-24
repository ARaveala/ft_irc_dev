#pragma once

#include <string.h>		//strerror()
#include <map>			//std::map()
#include <poll.h>		// pollfd
#include <csignal>		// signal(), SIGINT
#include <fcntl.h>		// fcntl()

#include "Channel.hpp"
#include "Msg.hpp"
#include "log.hpp" // for server logging

#define MAX_LEN_HOSTNAME 64

extern bool servRunning;

class Server {
	private:
		std::string _password;
		sockaddr_in _serverAddr;

		int			_port;
		int			_serverSocket;
		std::string _startTimeStr;

		std::vector<Client>		_clients;
		std::vector<Channel>	_channel_names;
		std::vector<pollfd>		_pollfds;

	public:
		Server(std::string password, int port);
		~Server();

		//Server Init
		int		serverInit();

		//Server Getter
		std::string getStartTimeStr() { return _startTimeStr; }

		//ServerLoop
		void	serverLoop();
		void	setUpServerPollfd();
		void	checkServerSocket();
		void	acceptClient();
		void	disconnectClient(size_t& i, std::map<int, std::string> &clientBuffers);

		//Client Loop
		void	checkClientSockets(std::map<int, std::string> &clientBuffers);
		void	processClientBuffer(size_t i, std::map<int, std::string> &clientBuffers);

		//Remove Client
		void	removeClient(int bytesRead, size_t i, std::map<int, std::string> &clientBuffers);
		void	printErrorBytesRead(int	bytesRead, int i);
		void	removeUserFromAllChannels(int i);

		//Make Message and Run Command
		void	makeMessages(std::vector<Msg> &msgs, std::string buffer);
		int		makeSelectAndRunCommand(std::string buffer, Client &client);
		int		commandSelector(Msg msg, Client &client);

		//COMMANDS
		int		passwordCommand(Msg msg, Client &client);
		int		userCommand(Msg msg, Client &client);
		void	kickCommand(Msg msg, Client &client);
		void	inviteCommand(Msg msg, Client &client);
		void	partCommand(Msg msg, Client &client);
		void	topicCommand(Msg msg, Client &client);
		void	whoCommand(Msg msg, Client &client);

		//Nick Command
		int		nicknameCommand(Msg msg, Client &client);
		int		nickClash(const std::string& nickname, int socket);

		//Privmsg Command
		void	privmsgCommand(Msg msg, Client &client);
		void	channelMessage(Msg msg, Client &client);		
		void	userMessageToChannel(Channel channel, int sender_socket, std::string message);
		void	directMessage(Msg msg, Client &client);

		//Mode Command
		void	modeCommand(Msg msg, Client &client);
		int		channelChecks(Msg msg, Client &client);
		void	topicMode(Msg msg, Client &client, Channel* tarChannel);
		void	keyMode(Msg msg, Client &client, Channel* tarChannel);
		void	inviteMode(Msg msg, Client &client, Channel* tarChannel);
		void	operatorMode(Msg msg, Client &client, Channel* tarChannel);
		void	userLimitMode(Msg msg, Client &client, Channel *tarChannel);

		//Join command
		void	joinCommand(Msg msg, Client &client);
		void    createChannel(Msg msg, Client &client);
		void	addChannelUser(Channel &new_channel, Client &client, bool operator_permissions);
		int		channelJoinChecks(Channel channel, Msg msg, Client &client);
		void	joinChannelMessage(std::string channelName, Client &client);

		//Command Utils
		Channel* getChannel(std::string channelName);
		int		channelExists(std::string channel);
		int		userExists(std::string user, std::string channel);
		int		getClientSocket(std::string nickname);;
		void	broadcastToChannel(Channel &channel, std::string message, Client &client, int check);

		int		clientStatus(Msg msg, Client &client);
		void	topicPrint(std::string channelName, Client &client);		
		int		removeUser(std::string user, std::string channel, std::string message, int partOrKick);

		//Debuggers
		void	printChannels();
		void	printChannelUsers(Channel channel);

		//Clean up
		void	closeFDs();

};

	int			getChannelIndex(std::string channel_name, std::vector<Channel> channel_names);
	void		initializeMsg(Msg &msg, std::vector<std::string> array);

	//User input checks
	int			checkArgumentCount(int argc);
	u_int16_t	getAndCheckPortValue(std::string port_string);
	int			passwordCheck(std::string password);

	//Time utils
	std::string getCurrentTime();
	std::string getCurrentEpochTime();
	time_t		stringToUnixTimeStamp(std::string time);

	//debuggers
	void	printMsg(Msg msg);
	void	printArray(std::vector<std::string>message_array);