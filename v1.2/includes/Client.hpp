#pragma once

#include <string>
#include <memory>

#include "IrcMessage.hpp"

class Server;
class Channel;
class IrcMessage;

class Client {
	private:
		int _fd;
		int _timer_fd;
		int _failed_response_counter = 0;

		time_t signonTime;			// todo set on registration
		time_t lastActivityTime;	// todo update when read data is processed from clients socket.

		std::bitset<config::CLIENT_PRIV_NUM_MODES> _ClientPrivModes;
		bool _channelCreator = false;
		bool _quit = false;
		bool _hasSentCap = false;
		bool _hasSentNick = false;
		bool _hasSentUser = false;
		bool _registered = false;
		std::string _read_buff;
		std::string _send_buff;
		std::string _oldNick;
		std::string _nickName;
		std::string _username;
		std::string _fullName;
		std::string _hostname; // todo check this is set during connection
		bool _isOperator; // todo client can be many chanel operator

		std::map<std::string, std::function<void()>> _commandMap; // map out commands to their fucntion calls to avoid large if else
		IrcMessage _msg;
		std::map<std::string, std::weak_ptr<Channel>> _joinedChannels; // list of joined channels

		bool _pendingAcknowledged = false;
		bool _acknowledged = false;

	public:
		Client() = delete;
		Client(int fd, int timerfd);
		~Client();
		int getFd();
		int get_failed_response_counter();
		IrcMessage& getMsg() {return _msg;};
		void set_failed_response_counter(int count);
		void setQuit() {_quit = true;};
		
		void setMode(clientPrivModes::mode mode) { _ClientPrivModes.set(mode);  };
		void unsetMode(clientPrivModes::mode mode) { _ClientPrivModes.reset(mode);}
		bool hasMode(clientPrivModes::mode mode) { return _ClientPrivModes.test(mode);};
		std::string getCurrentModes() const; 
		bool isValidClientMode(char modeChar) {
		    return std::find(clientPrivModes::clientPrivModeChars.begin(), clientPrivModes::clientPrivModeChars.end(), modeChar) != clientPrivModes::clientPrivModeChars.end();
		}

		void setHasSentCap() {_hasSentCap = true;};
		void setHasSentNick() {_hasSentNick = true;};
		void setHasSentUser() {_hasSentUser = true;};
		void setHasRegistered();

		void setOldNick(std::string oldnick) {_oldNick = oldnick; }
		
		const std::string& getOldNick() {  return _oldNick; };

		bool& getHasSentCap() {return _hasSentCap;};
		bool getHasSentNick() {return _hasSentNick;};
		bool getHasSentUser() {return _hasSentUser;};
		bool getHasRegistered() {return _registered;};
		std::string getPrivateModeString(); // const and all that 
		bool getQuit() {return _quit;};
		bool get_acknowledged();
		bool get_pendingAcknowledged();
		const std::map<std::string, std::weak_ptr<Channel>>& getJoinedChannels() const;
		//void set_pendingAcknowledged(bool onoff);
		void set_acknowledged();
		void setChannelCreator(bool onoff) { _channelCreator = onoff;};
		void clearJoinedChannels() {_joinedChannels.clear();};
		int get_timer_fd();
		long getIdleTime() const;
		time_t getSignonTime() const;
		
		bool getChannelCreator() {return _channelCreator;};

		std::string getNickname() const;
		std::string& getNicknameRef();
		std::string getClientUname();
		std::string getfullName();
		const std::string& getHostname() const;
		bool isOperator() const;

		void setDefaults();
		void setClientUname(std::string uname) {_username = uname;};

		void setHostname(const std::string& hostname);
		void setOperator(bool status);

		bool addChannel(std::string channelName, std::shared_ptr<Channel> channel);
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
		int prepareQuit(std::deque<std::shared_ptr<Channel>>& channelsToNotify);
		void executeCommand(const std::string& command);
		void setCommandMap(Server &server); // come back to this
		
		void appendIncomingData(const char* buffer, size_t bytes_read);
		bool extractAndParseNextCommand();

		void updateLastActivityTime();
};


