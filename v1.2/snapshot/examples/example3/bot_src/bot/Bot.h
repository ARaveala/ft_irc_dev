#ifndef __BOT_H__
#define __BOT_H__

#include "../headers/headers.h"
#include "../headers/colour.h"
#include "../botmessage/BotMessage.h"
#include "../botcommand/BotCommand.hpp"
#include <unordered_map>
#include <algorithm>
#define CONFIG_FILE "bot_src/config_file"

class BotMessage;
class BotCommand;
class Bot
{
private:
	struct addrinfo addr_info_;
	struct addrinfo *serv_addr_info_;
	std::string 	server_addr_;
	int 			server_port_;
	std::string 	server_password_;
	std::string 	nickname_;
	std::string 	username_;
	std::string 	buffer_;
	struct pollfd 	poll_fd_;
	int 			server_fd_;
	bool 			register_status_;
	static bool 	signal_;
	std::unordered_map<std::string, std::string> operators_;
	std::vector<std::string> forbidden_words_;
	std::vector<std::string> violated_users_;
	std::map<std::string, void (BotCommand::*)(const BotMessage &msg)> supported_commands_;

public:
	Bot(std::string &server_address, int &port, std::string &password, char **av);
	~Bot();
	void init_bot();
	std::string const	&getServerAddr() const;
	std::string const	&getServerPassword() const;
	int const 			&getServerPort() const;
	std::string const	&getNickname() const;
	std::string const	&getUsername() const;
	int const 			&getServerfd() const;
	bool const 			&getRegisterStatus() const;
	std::unordered_map<std::string, std::string> const &getOperators() const;
	std::vector<std::string> const &getForbiddenWords() const;
	std::vector<std::string> const &getViolatedUsers() const;
	

	void 		setUsername(std::string const &username);
	void 		setNickname(std::string const &nickname);
	void 		setRegisterStatus(bool const &status);
	void		insertInViolatedUsers(std::string const &username);

	static void signalhandler(int signum);
	void createBotSocket();
	void readBuffer();
	void reConnection();
	void appendToBuffer(const std::string &data);
	void processBuffer();
	bool isValidNickname(std::string nickname);
	void send_response(int fd, const std::string &response);
	void testConnection();
	void processCommand(BotMessage &message);
	std::map<std::string, void (BotCommand::*)(const BotMessage &msg)> const &getSupportedCommands() const;
	void	readConfigFile();
};

#endif