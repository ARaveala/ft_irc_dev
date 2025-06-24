#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <iostream>
#include <string>
#include <regex>
#include <vector>
#include <algorithm>
#include "../server/Server.h"
#include "../message/Message.h"
#include "../channel/Channel.h"
#include "../common/MagicNumbers.h"
#include "../common/reply.h"


class Server;
class Channel;
class Message;

class Command
{
private:
	Server *server_ptr_;

public:

	Command(Server *server_ptr);
	~Command();

	void handleAway(const Message &msg);
	void handleInvite(const Message &msg);
	void handleWho(const Message &msg);
	void handleUser(const Message &msg);
	void handlePrivmsg(const Message &msg);
	void handleQuit(const Message &msg);
	void handlePass(const Message &msg);
	void handleCap(const Message &msg);
	void handlePing(const Message &msg);
	void handleWhois(const Message &msg);
	void handlePart(const Message &msg);
	void handleKick(const Message &msg);
	void handleTopic(const Message &msg);
	void handleOper(const Message &msg);
	void handleKill(const Message &msg);
	
	void handleMode(const Message &msg);
	void handleUserMode(std::shared_ptr<Client> client_ptr, const std::string &target, const std::string &mode_string);
	void applyUserMode(std::shared_ptr<Client> client_ptr, const std::string &mode_string);
	void sendCurrentUserModes(std::shared_ptr<Client> client_ptr);
	void handleChannelMode(std::shared_ptr<Client> client_ptr, const std::string &target, 
							const std::string &mode_string, const std::vector<std::string> &mode_arguments);
	void applyChannelModes(std::shared_ptr<Client> client_ptr, std::shared_ptr<Channel> channel_ptr,
							const std::string& mode_string, const std::vector<std::string>& mode_arguments);
	void handleModeL(std::shared_ptr<Channel> channel_ptr, bool mode_setting, const std::vector<std::string> &mode_arguments,
					size_t &arg_index, std::string &changed_modes, std::string &used_parameters, char &last_sign);
	void handleModeK(std::shared_ptr<Channel> channel_ptr, bool mode_setting, const std::vector<std::string> &mode_arguments,
					size_t &arg_index, std::string &changed_modes, std::string &used_parameters, char &last_sign);
	void handleModeO(std::shared_ptr<Client> client_ptr, std::shared_ptr<Channel> channel_ptr, const std::vector<std::string> &mode_arguments,
							bool mode_setting, size_t &arg_index, std::string &changed_modes, std::string &used_parameters, char &last_sign);
	bool applyModeO(std::shared_ptr<Client> client_ptr, std::shared_ptr<Channel> channel_ptr, const std::string &target_nickname, bool mode_setting);
	void handleModeChange(std::shared_ptr<Client> client_ptr, std::shared_ptr<Channel> channel_ptr, char mode_char, bool mode_setting,
							const std::vector<std::string> &mode_arguments, size_t &arg_index, std::string &changed_modes,
							std::string &used_parameters, char &last_sign);
	void appendToChangedModeString(bool mode_setting, std::string &changed_modes, char &last_sign, char mode_char);
	void parseModeString(const std::string &mode_string, std::vector<std::pair<char, bool>> &mode_changes, bool &mode_setting);
	bool modeRequiresParameter(char mode_char);
	bool mandatoryModeParameter(char mode_char);

	void handleNick(const Message &msg);
	bool isValidNickname(std::string& nickname);
	bool isNicknameInUse(std::string const &nickname);
	
	void handleJoin(const Message &msg);
	bool isValidChannelName(const std::string& channel_name) const;
	bool channelExists(std::string const &channel_name);
	void sendNamReplyAfterJoin(std::shared_ptr<Channel> channel_ptr, std::string nickname, int client_fd);
	void sendNamelist(std::shared_ptr<Channel> channel_ptr, std::string nickname, int client_fd);
	
	std::vector<std::string> split(const std::string &s, char delim);
	
};

#endif
