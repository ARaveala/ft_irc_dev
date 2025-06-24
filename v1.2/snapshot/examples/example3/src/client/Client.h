#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "../headers.h"
#include "../server/Server.h"
#include "../command/Command.h"
#include "../message/Message.h"
#include "../channel/Channel.h"

class Server;
class Commands;
class Message;
class Channel;

class Client: public std::enable_shared_from_this<Client>
{
	private:
	int															fd_;
	bool														registered_;
	bool														has_correct_password_;
	bool														rejected_;
	std::string													nickname_;
	std::string													old_nickname_;
	std::string													username_;
	std::string													hostname_;
	std::string													realname_;
	std::string													ip_address_;
	std::string													client_prefix_;
	std::string													buffer;
	bool														mode_i_;
	bool														mode_local_op_;
	bool														away_status_;
    std::string													away_message_;
	std::vector<std::weak_ptr<Channel>>							channels_;
	// TODO monitoring ping pong status
	// clients that have not replied in a long time should be dropped from the server

	public:
	Client() = default;
	Client(const int &fd, const std::string &nickname, const std::string &username, const std::string &ipaddress);
	~Client();

	// getters
	int			const	&getFd() const;
	std::string const	&getNickname() const;
	std::string const	&getOldNickname() const;
	std::string const	&getUsername() const;
	std::string const	&getHostname() const;
	std::string const	&getRealname() const;
	std::string const	&getIpAddress() const;
	bool	 	const	&getModeI() const;
	bool		const   &getModeLocalOp() const;
	bool		const	&getRegisterStatus() const;
	std::string const	&getClientPrefix() const;
	std::string const	&getAwayMessage() const;
	bool		const	&getRejectedStatus() const;
	bool		const	&getHasCorrectPassword() const;
	bool 		const	&isAway() const;
	std::vector<std::weak_ptr<Channel>> const &getChannels() const;
	

	// setters

	void		setFd(int const &fd);
	void		setNickname(std::string const &nickname);
	void		setOldNickname(std::string const &nickname);
	void		setUsername(std::string const &username);
	void		setHostname(std::string const &hostname);
	void		setRealname(std::string const &realname);
	void		setUserMode(char const &usermode);
	void		setIpAddress(std::string const &ip_address);
	void		setHasCorrectPassword(bool const &status);
	void        setClientPrefix();
	void		setAway(bool status, const std::string& message = "");
	void		setModeI(bool status);
	void		setModeLocalOp(bool status);
	void		setRejectedStatus(bool const &status);
	
	// member functions
	void		registerClient();
	void		unregisterClient();
	void 		processBuffer(Server *server_ptr);
	void 		appendToBuffer(const std::string& data);
	void		processCommand(Message &message, Server *server_ptr);

	bool		joinChannel(const std::shared_ptr<Channel>& channel_ptr);
	void		leaveChannel(const std::weak_ptr<Channel>& channel_ptr);
	
};
#endif
