#pragma once

#include <string>
#include <memory>
#include <bitset>     // For std::bitset
#include <map>        // For std::map
#include <functional> // For std::function
#include <deque>      // For std::deque
#include <algorithm>  // For std::find
#include <ctime>      // For time_t and time(NULL)

#include "IrcMessage.hpp"
#include "config.h" // Assuming this contains CLIENT_PRIV_NUM_MODES and clientPrivModes

class Server;       // Forward declaration
class Channel;      // Forward declaration
class IrcMessage;   // Forward declaration

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
        bool _registered = false; // Becomes true when full registration (PASS, NICK, USER) is complete
        bool _isAuthenticatedByPass = false; // Set to true after a correct PASS command
        bool _passwordRequiredForRegistration = false; // Set based on server's global config

        std::string _read_buff;
        std::string _send_buff;
        std::string _oldNick;
        std::string _nickName;
        std::string _username;
        std::string _fullName;
        std::string _hostname;
        bool _isOperator;

        std::map<std::string, std::function<void()>> _commandMap;
        IrcMessage _msg; // IrcMessage object for handling incoming/outgoing messages
        std::map<std::string, std::weak_ptr<Channel>> _joinedChannels;

        bool _pendingAcknowledged = false;
        bool _acknowledged = false;

    public:
        Client() = delete; // Delete default constructor to force fd/timerfd initialization
        Client(int fd, int timerfd); // Constructor if serverPasswordRequired is not passed (e.g., test setup)
        Client(int fd, int timerfd, bool serverPasswordRequired); // Main constructor used by Server
        ~Client();

        // Getters for core client properties
        int getFd() const;
        int get_timer_fd() const;
        const std::string& getNickname() const;
        std::string& getNicknameRef(); // For mutable access (e.g., by Server for nickname changes)
        std::string getClientUname() const; // Renamed to const
        std::string getFullName() const; // Corrected case, added const
        const std::string& getUsername() const;
        const std::string& getHostname() const;
        bool isOperator() const;
        bool getChannelCreator() const;
        long getIdleTime() const;
        time_t getSignonTime() const;
        bool getQuit() const; // Getter for _quit flag
        bool getHasSentCap() const;
        bool getHasSentNick() const;
        bool getHasSentUser() const;
        bool getHasRegistered() const; // Getter for _registered flag
        bool get_acknowledged() const;
        bool get_pendingAcknowledged() const;
        int get_failed_response_counter() const;
        const std::map<std::string, std::weak_ptr<Channel>>& getJoinedChannels() const;

        // Getters for password-related flags
        bool getIsAuthenticatedByPass() const;
        bool getPasswordRequiredForRegistration() const;

        // Setters for client properties
        void set_failed_response_counter(int count);
        void setQuit(); // Sets the quit flag, signals client should be removed
        void set_nickName(const std::string newname);
        void setHasSentCap();
        void setHasSentNick();
        void setHasSentUser();
        void setHasRegistered(); // Attempts to set _registered based on all conditions
        void setNickname(const std::string& nickname);
        void setOldNick(std::string oldnick);
        void setFullName(const std::string& fullname); // Matches `getFullName()`
        void setClientUname(std::string uname);
        void setHostname(const std::string& hostname);
        void setOperator(bool status);
        void setChannelCreator(bool onoff);
        void set_acknowledged();
        void setDefaults(); // <--- THIS IS THE MISSING METHOD DECLARATION

        // Setter for authentication status
        void setIsAuthenticatedByPass(bool status);

        // Client Mode management
        void setMode(clientPrivModes::mode mode);
        void unsetMode(clientPrivModes::mode mode);
        bool hasMode(clientPrivModes::mode mode);
        std::string getCurrentModes() const;
        std::string getPrivateModeString();
        bool isValidClientMode(char modeChar);

        // Channel management for client
        bool addChannel(const std::string& channelName, const std::shared_ptr<Channel>& channel);
        std::string getChannel(std::string channelName); // Retrieves channel name (consider shared_ptr return for full channel object)
        void removeJoinedChannel(const std::string& channel_name);
        void addJoinedChannel(const std::string& channel_name, std::shared_ptr<Channel> channel_ptr);
        void clearJoinedChannels();

        // Message & Command handling
        IrcMessage& getMsg(); // Provides access to client's IrcMessage object
        void sendMessage(const std::string& message); // Queues message for sending to client's socket
        bool isMsgEmpty();
        void appendIncomingData(const char* buffer, size_t bytes_read);
        bool extractAndParseNextCommand();
        void executeCommand(const std::string& command); // (If CommandDispatcher is not used)
        void setCommandMap(Server &server); // (If client handles own commands)

        // Ping/Pong & Activity
        void sendPing();
        void sendPong();
        void updateLastActivityTime();

        // Nickname change logic
        bool change_nickname(std::string nickname);

        // Disconnection
        int prepareQuit(std::deque<std::shared_ptr<Channel>>& channelsToNotify); // For QUIT command processing
        void disconnect(const std::string& reason); // Marks client for disconnection
};
