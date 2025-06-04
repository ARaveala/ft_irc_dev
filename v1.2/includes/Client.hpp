#pragma once
#include <string>
// https://modern.ircdocs.horse/#client-to-server-protocol-structure
// Names of IRC entities (clients, servers, channels) are casemapped
#include "IrcMessage.hpp"
//#include <shared_ptr>
class Server;
//class IrcMessage;
class Channel;
class Client {
	private:
		int _fd;
		int _timer_fd;
		int _failed_response_counter = 0;
		bool _channelCreator = false;
		bool _quit = false;
		std::string _read_buff;
		std::string _send_buff;
		std::string _oldNick;
		std::string _nickName;
		std::string _username;
		std::string _fullName;
		std::map<std::string, std::function<void()>> _commandMap; // map out commands to their fucntion calls to avoid large if else
		IrcMessage _msg;
		//std::deque<std::string> _welcome;
		std::map<std::string, std::weak_ptr<Channel>> _joinedChannels; // list of joined channels

		//std::deque<std::string> _messageQue;
		//std::string _prefixes; // Client permissions 
		//int ping_sent; // std::chrono::steady_clock
		// pointer to current channel object ?
		// list of channels Client is in 
		bool _pendingAcknowledged = false;
		bool _acknowledged = false;
	public:
		Client();
		Client(int fd, int timerfd);
		~Client();
		int getFd();
		int get_failed_response_counter();
		IrcMessage& getMsg() {return _msg;};
		void set_failed_response_counter(int count);
		void setQuit() {_quit = true;};
		void setOldNick(std::string oldnick) {_oldNick = oldnick; }
		
		const std::string& getOldNick() {  return _oldNick; };
		bool getQuit() {return _quit;};
		bool get_acknowledged();
		bool get_pendingAcknowledged();
		const std::map<std::string, std::weak_ptr<Channel>>& getJoinedChannels() const;
		//void set_pendingAcknowledged(bool onoff);
		void set_acknowledged();
		void setChannelCreator(bool onoff) { _channelCreator = onoff;};
		void clearJoinedChannels() {_joinedChannels.clear();};
		int get_timer_fd();
		
		bool getChannelCreator() {return _channelCreator;};

		std::string getNickname() const;
		std::string& getNicknameRef();
		std::string getClientUname();
		std::string getfullName();
		void setDefaults();
		void setClientUname(std::string uname) {_username = uname;};
		//void addToMessageQue(std::string message) { _msg.queueMessage(message);};
		bool addChannel(std::string channelName, std::shared_ptr<Channel> channel);
		//void removeChannel(std::string channelName);
		std::string getChannel(std::string channelName);
		void sendPing();
		void sendPong();

		bool change_nickname(std::string nickname);

		bool isMsgEmpty() {
			if (_msg.getQue().empty())
			{
				return true;
			}
			return false;
		};
		int prepareQuit(std::deque<std::shared_ptr<Channel>> channelsToNotify);
		//void dispatchCommand(const std::string message, Server& server);
		// loops through to find which command fucntion to execute
		void executeCommand(const std::string& command);
		void setCommandMap(Server &server); // come back to this
		
		void appendIncomingData(const char* buffer, size_t bytes_read);
		bool extractAndParseNextCommand();
    

};


