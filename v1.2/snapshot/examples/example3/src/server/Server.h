/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: atoof <atoof@student.hive.fi>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/15 12:13:56 by atoof             #+#    #+#             */
/*   Updated: 2024/04/15 12:13:56 by atoof            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef __SERVER_H__
#define __SERVER_H__

#include "../client/Client.h"
#include "../headers.h"
#include "../debug/debug.h"
#include "../common/reply.h"
#include "../message/Message.h"
#include "../command/Command.h"
#include "../channel/Channel.h"

#define CONFIG_FILE "config_file"
#define MAX_MSG_LENGTH 512
#define DEFAULTPORT 6667
#define CRLF "\r\n"

typedef struct s_opers
{
	std::string nick;
	std::string hostmask;
	std::string password;
} t_opers;

class Client;
class Channel;
class Message;
class Command;
class Server
{
private:
	std::string 												host_;
	int 														port_;
	const std::string 											password_;
	int															socket_;
	std::vector<struct pollfd> 									fds_; // pollfd structure for the server socket
	std::map <int, std::shared_ptr<Client>>						clients_;
	std::map <std::string, std::shared_ptr<Channel>>			channels_;
	static bool													signal_;
	std::vector<t_opers>										operators_file_;
	std::map<std::string, void (Command::*)(const Message &msg)> supported_commands_;
	static void								shutdownServer(const std::string& reason);

public:
	Server(const int &port, const std::string &password, const std::stringstream &config_file);
	virtual ~Server();
	static void 							signalHandler(int signum);
	void									createServerSocket();
	void									registerNewClient();
	void									handleClientData(int fd);
	void									initServer();
	void									deleteClient(int fd);
	void									deleteChannel(std::string const &channelname);
	void									closeDeletePollFd(int fd);
	void									closeFds();
	void									disconnectAndDeleteClient(std::shared_ptr<Client> client_ptr, std::string const &reason);
	std::shared_ptr<Client>					findClientUsingFd(int fd) const;
	std::shared_ptr<Client> 				findClientUsingNickname(std::string const &nickname) const;
	std::shared_ptr<Client> 				findClientUsingOldNickname(std::string const &nickname) const;
	void									whoGotDisconnected(int fd);
	void									extractUserIpAddress(char *ip, struct sockaddr_in6 usersocketaddress);
	void									sendResponse(int fd, const std::string &response);
	void									setServerHostname();
	void 									welcomeAndMOTD(int fd, std::string const &servername, std::string const &nickname, std::string const &client_prefix);
	std::shared_ptr<Channel>				createNewChannel(std::string const &channel_name);
	std::shared_ptr<Channel>				findChannel(std::string const &channel_name);
	void									removeChannel(std::string const &channel_name);
	std::shared_ptr<Channel> 				findOrCreateChannel(const std::string& name);
	std::string 							createRegexFromMask(const std::string &mask) const;
	std::string 							toLower(const std::string& str) const;
	std::vector<std::shared_ptr<Client>> 	findClientsByMask(const std::string &mask) const;
	bool									hasClientSentPass(std::shared_ptr <Client> const &client_ptr);
	void									initOperators(const std::stringstream &config_file);
	void									sendQuitMessages(std::shared_ptr<Client> client_ptr, std::string const &reason);

//getter

	std::vector<t_opers>	const							&getOperatorsFile() const;
	std::string const & getPassword() const;
	// getter for map of supported commands
	std::map<std::string, void (Command::*)(const Message &msg)> const &getSupportedCommands() const;
	std::string const &getServerHostname() const;
	std::map<std::string, std::shared_ptr<Channel>> const &getChannels() const;

};

#endif
