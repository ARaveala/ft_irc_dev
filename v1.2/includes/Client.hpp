#pragma once

#include <string>
#include <memory>
#include <chrono>
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
		bool _passwordValid = false;
		bool _clientReadyForInput = false; // used to check if client is ready for input, if not we send a message to wait
		std::string _read_buff;
		std::string _send_buff;
		std::string _oldNick;
		std::string _nickName; // stupid Name, should be name
		std::string _username;
		std::string _fullName;
		std::string _hostname; // todo check this is set during connection
		bool _isOperator; // todo client can be many chanel operator
		std::chrono::time_point<std::chrono::steady_clock> _pleaseWait;
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
		void set_nickName(const std::string& newname) {_nickName.clear(); _nickName = newname; };
		void setRealname(const std::string& newname) { _fullName = newname;};
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
		void setClientReadyForInput() {_clientReadyForInput = true;};

		void setPasswordValid() {_passwordValid = true;};

		void setHasRegistered();

		void setNickname(const std::string& nickname) { _nickName.clear();_nickName = nickname;};

		void setOldNick(const std::string& oldnick) {_oldNick = oldnick; }
		
		const std::string& getOldNick() {  return _oldNick; };

		bool& getHasSentCap() {return _hasSentCap;};
		bool getHasSentNick() const {return _hasSentNick;};
		bool getHasSentUser() const {return _hasSentUser;};
		bool getHasRegistered() const {return _registered;};
		bool getClientReadyForInput() const {return _clientReadyForInput;};
		bool getPasswordValid() const {return _passwordValid;};

		std::string getPrivateModeString(); 
		bool getQuit() const {return _quit;};
		bool get_acknowledged();
		bool get_pendingAcknowledged();
		const std::map<std::string, std::weak_ptr<Channel>>& getJoinedChannels() const;
	
		void set_acknowledged();
		void setChannelCreator(bool onoff) { _channelCreator = onoff;};
		void clearJoinedChannels() {_joinedChannels.clear();};
		int get_timer_fd();
		long getIdleTime() const;
		time_t getSignonTime() const;
		
		bool getChannelCreator() const {return _channelCreator;};

		std::string getNickname() const;
		std::string& getNicknameRef();
		std::string getClientUname();
		std::string getfullName();

		const std::string& getUsername() const { return _username;}

		const std::string& getHostname() const;
		bool isOperator() const;

		void setClientUname(const std::string& uname) {_username = uname;};

		void setHostname(const std::string& hostname);
		void setOperator(bool status);

		bool addChannel(const std::string& channelName, const std::shared_ptr<Channel>& channel);
		std::string getChannel(std::string channelName);

		bool isMsgEmpty();
		void appendIncomingData(const char* buffer, size_t bytes_read);
		bool extractAndParseNextCommand();

		void removeJoinedChannel(const std::string& channel_name);

    // You'll also need a way to add channels to this map (e.g., in Server::handleJoinCommand)
    // Assumes channel_name is already lowercase.
    //void addJoinedChannel(const std::string& channel_name, std::shared_ptr<Channel> channel_ptr);

    // For QUITTING, you might need a getter for all joined channels
    // const std::map<std::string, std::weak_ptr<Channel>>& getJoinedChannels() const;

	//adedded extar feature to prevent user error on early nick or join 
		void setRegisteredAt(std::chrono::time_point<std::chrono::steady_clock> start) {_pleaseWait = start;}
		std::chrono::time_point<std::chrono::steady_clock> getRegisteredAt() {return _pleaseWait;}
};
