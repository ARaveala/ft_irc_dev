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
		std::string _read_buff;
		std::string _send_buff;
		std::string _nickName;
		std::string _ClientName;
		std::string _fullName;
		IrcMessage _msg;
		//std::deque<std::string> _welcome;
		std::map<std::string, std::weak_ptr<Channel>> _joinedChannels; // list of joined channels

		//std::deque<std::string> _message_que;
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
		void receive_message(int fd, Server& server);
		int getFd();
		int get_failed_response_counter();
		IrcMessage& getMsg() {return _msg;};
		void set_failed_response_counter(int count);
		bool get_acknowledged();
		bool get_pendingAcknowledged();
		void set_pendingAcknowledged(bool onoff);
		void set_acknowledged();
		int get_timer_fd();
		std::string getNickname();
		std::string& getNicknameRef();
		std::string getClientName();
		std::string getfullName();
		void setDefaults();

		bool addChannel(std::string channelName, std::shared_ptr<Channel> channel);
		void removeChannel(std::string channelName);
		std::string getChannel(std::string channelName);
		void sendPing();
		void sendPong();

		bool change_nickname(std::string nickname, int fd);

		std::string getReadBuff();
		void setReadBuff(const std::string& buffer);

		bool isMsgEmpty() {
			if (_msg.getQue().empty())
			{
				return true;
			}
			return false;
		};
		void prepareQuit(std::deque<std::string>& channelsToNotify);
		void handle_message(const std::string message, Server& server);
};


