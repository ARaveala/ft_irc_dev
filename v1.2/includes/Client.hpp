#pragma once

#include <string>
#include <memory>
#include <bitset>     // Added for std::bitset
#include <map>        // Added for std::map
#include <functional> // Added for std::function
#include <deque>      // Added for std::deque
#include <algorithm>  // Added for std::find
#include <ctime>      // Added for time_t and time(NULL)

#include "IrcMessage.hpp"
#include "config.h" // Assuming this contains CLIENT_PRIV_NUM_MODES and clientPrivModes

class Server;
class Channel;
class IrcMessage;

class Client {
    private:
        int _fd;
        int _timer_fd;
        int _failed_response_counter = 0;

        time_t signonTime;
        time_t lastActivityTime;

        std::bitset<config::CLIENT_PRIV_NUM_MODES> _ClientPrivModes;
        bool _channelCreator = false;
        bool _quit = false; // Flag to indicate if client wishes to quit or should be disconnected
        bool _hasSentCap = false;
        bool _hasSentNick = false;
        bool _hasSentUser = false;
        bool _registered = false; // This will become true when full registration (PASS, NICK, USER) is complete
        bool isAuthenticatedByPass = false; // Set to true after a correct PASS command
        bool passwordRequiredForRegistration = false; // Set based on server's global config

        std::string _read_buff;
        std::string _send_buff;
        std::string _oldNick;
        std::string _nickName;
        std::string _username;
        std::string _fullName;
        std::string _hostname;
        bool _isOperator;

        std::map<std::string, std::function<void()>> _commandMap;
        IrcMessage _msg;
        std::map<std::string, std::weak_ptr<Channel>> _joinedChannels;

        bool _pendingAcknowledged = false;
        bool _acknowledged = false;

    public:
        Client() = delete;
        Client(int fd, int timerfd);
        Client(int fd, int timerfd, bool serverPasswordRequired);
        ~Client();
        int getFd();
        int get_failed_response_counter();
        IrcMessage& getMsg() {return _msg;};
        void set_failed_response_counter(int count);
        void setQuit() {_quit = true;}; // Sets the quit flag, signals client should be removed
        void set_nickName(const std::string newname) {_nickName.clear(); _nickName = newname; };
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

        void setNickname(const std::string& nickname) { _nickName.clear(); _nickName = nickname;};

        void setOldNick(std::string oldnick) {_oldNick = oldnick; }

        const std::string& getOldNick() {  return _oldNick; };

        bool& getHasSentCap() {return _hasSentCap;};
        bool getHasSentNick() {return _hasSentNick;};
        bool getHasSentUser() {return _hasSentUser;};
        bool getHasRegistered() {return _registered;}; // Getter for _registered flag
        std::string getPrivateModeString();
        bool getQuit() {return _quit;}; // Getter for _quit flag
        bool get_acknowledged();
        bool get_pendingAcknowledged();
        const std::map<std::string, std::weak_ptr<Channel>>& getJoinedChannels() const;
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
        void setFullName(const std::string& fullname) { _fullName = fullname; } // Setter for _fullName
        std::string getFullName() const;

        const std::string& getUsername() const { return _username;}

        const std::string& getHostname() const;
        bool isOperator() const;

        void setDefaults();
        void setClientUname(std::string uname) {_username = uname;};

        void setHostname(const std::string& hostname);
        void setOperator(bool status);

        bool addChannel(const std::string& channelName, const std::shared_ptr<Channel>& channel);
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
        void setCommandMap(Server &server);

        void appendIncomingData(const char* buffer, size_t bytes_read);
        bool extractAndParseNextCommand();

        void updateLastActivityTime();

        void removeJoinedChannel(const std::string& channel_name);
        void addJoinedChannel(const std::string& channel_name, std::shared_ptr<Channel> channel_ptr);

        // --- ADDED GETTERS FOR PRIVATE FLAGS ---
        bool getPasswordRequiredForRegistration() const { return passwordRequiredForRegistration; }
        bool getIsAuthenticatedByPass() const { return isAuthenticatedByPass; }

        // --- ADDED SETTER FOR isAuthenticatedByPass ---
        void setIsAuthenticatedByPass(bool status) { isAuthenticatedByPass = status; }

        // --- ADDED METHOD TO HANDLE DISCONNECTION ---
        void disconnect(const std::string& reason);

        // --- ADDED MISSING DECLARATION FOR Client::sendMessage ---
        void sendMessage(const std::string& message);
};
