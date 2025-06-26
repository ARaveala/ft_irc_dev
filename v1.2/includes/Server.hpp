#pragma once

#include <string>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <sys/epoll.h>
#include <deque>
#include <algorithm> // Added for std::transform
#include <cctype>    // Added for std::tolower

#include "ServerError.hpp"
#include "Channel.hpp"
#include "CommandDispatcher.hpp"
#include "Client.hpp" // Ensure Client.hpp is included for Client class definition
#include "config.h" // Assuming this contains Modes::ChannelMode and clientPrivModes (changed from config.hpp)

/**
 * @brief The server class manages server related requests and
 * redirects to client/Client, message handling or channel handling when
 * required.
 *
 * @note 1. utalizing a map of shared pointers in map of clients/Clients
 * allows for quick look up of clent and gives potenatial access to other objects
 * such as class for information lookup, such as permissions.
 * shared_pointers also are more memory safe than manual memory management
 * and can simply be removed once not needed.
 *
 * @note 2. unordered map is a little faster if we choose to use that
 *
 */
class Client; // Forward declaration (already present)
class Channel; // Forward declaration (already present)

class Server {
    private:
        bool _passwordRequired;

        int _port;
        int _client_count = 0;
        int _fd;
        int _current_client_in_progress;
        int _signal_fd;
        int _epoll_fd;

        std::string _server_name;
        std::string _password;

        std::map<std::string, std::shared_ptr<Channel>> _channels;
        std::map<int, std::shared_ptr<Client>> _Clients;
        std::map<int, int> _timer_map;

        std::map<int, struct epoll_event> _epollEventMap;

        std::deque<std::string> _server_broadcasts;
        std::deque<std::shared_ptr<Channel>> _channelsToNotify;

        std::map<std::string, int> _nickname_to_fd;
        std::map<int, std::string> _fd_to_nickname;

        std::unique_ptr<CommandDispatcher> _commandDispatcher;
        // Removed unused parameters from declarations
        std::vector<std::string> splitCommaList(const std::string& input); // Keep input if it's functional
        std::pair<MsgType, std::vector<std::string>> validateChannelName(const std::string& channelName, const std::string& clientNick); // Keep both if used in real impl

    public:
        Server();
        Server(int port, const std::string& password = "");
        ~Server();
    // create things
        void create_Client(int epollfd);
        void createChannel(const std::string& channelName); // Keep if used

        void closeAndResetClient(); // Keep if used
        void remove_Client(int client_fd);
        bool isTimerFd(int fd); // Keep if used
        // SETTERS
        void setFd(int fd);
        void set_signal_fd(int fd);
        void set_client_count(int val);
        void set_event_pollfd(int epollfd);
        void set_current_client_in_progress(int fd);
        void set_nickname_in_map(std::string nick, int fd);

        // GETTERS
        int getPort() const;
        int getFd() const;
        int get_fd(const std::string& nickname) const;
        int get_signal_fd() const;
        int get_client_count() const;
        std::shared_ptr<Client> getClientByNickname(const std::string& nickname);
        int get_event_pollfd() const;
        int get_current_client_in_progress() const;

        std::shared_ptr<Client> get_Client(int fd);
        std::shared_ptr<Channel> get_Channel(const std::string& channelName);

        std::map<int, struct epoll_event> get_struct_map() {return _epollEventMap; };
        std::map<int, std::shared_ptr<Client>> get_Clients() {return _Clients;};
        std::map<int, std::string>& get_fd_to_nickname();
        std::map<std::string, int>& get_nickname_to_fd();
        std::map<int, std::shared_ptr<Client>>& get_map();

        std::map<std::string, std::shared_ptr<Channel>>& get_channels_map() { return _channels; }
        const std::map<std::string, std::shared_ptr<Channel>>& get_channels_map() const { return _channels; }

        // validators
        // Re-added parameters to match calls in main.cpp
        bool validateChannelExists(const std::shared_ptr<Client>& client, const std::string& channel_name, const std::string& sender_nickname);
        bool validateIsClientInChannel(const std::shared_ptr<Channel> channel, const std::shared_ptr<Client>& client, const std::string& channel_name, const std::string& nickname);
        bool validateTargetInChannel(const std::shared_ptr<Channel> channel, const std::shared_ptr<Client>& client, const std::string& channel_name, const std::string& target_nickname);
        bool validateTargetExists(const std::shared_ptr<Client>& client, const std::string& sender_nickname, const std::string& target_nickname);
        bool validateModes(const std::shared_ptr<Channel> Channel, const std::shared_ptr<Client>& client, Modes::ChannelMode comp);
        void handle_client_connection_error(ErrorType err_type); // Re-added err_type
        void shutDown();
        bool checkTimers(int fd); // Re-added fd
        void resetClientFullTimer(int resetVal, std::shared_ptr<Client> client); // Re-added parameters


        void removeQueueMessage() { _server_broadcasts.pop_front();};

        // epoll stuff
        int setup_epoll(int epoll_fd, int fd, uint32_t events); // Keep all if functional
        int setup_epoll_timer(int epoll_fd, int timeout_seconds); // Keep all if functional
        int create_epollfd();
        int createTimerFD(int timeout_seconds); // Re-added parameter
        void resetClientTimer(int timer_fd, int timeout_seconds); // Keep all if functional
        int make_socket_unblocking(int fd); // Keep if functional
        void send_message(const std::shared_ptr<Client>& client); // Keep if functional

        // channel related
        bool channelExists(const std::string& channelName) const; // Keep if functional
        void updateEpollEvents(int fd, uint32_t flag_to_toggle, bool enable); // Keep all if functional
        void handleJoinChannel(const std::shared_ptr<Client>& client, std::vector<std::string> params); // Keep all if functional
        void handleReadEvent(const std::shared_ptr<Client>& client, int client_fd); // Keep all if functional
        void handlePing(const std::shared_ptr<Client>& client); // Keep if functional
        void handlePong(const std::shared_ptr<Client>& client); // Keep if functional
        void handleQuit(std::shared_ptr<Client> client);
        void handleNickCommand(const std::shared_ptr<Client>& client, std::map<int, std::string>& fd_to_nick, std::map<std::string, int>& nick_to_fd, const std::string& param); // Keep all if functional
        void handleModeCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params); // Keep all if functional
        // Corrected: handleCapCommand declaration to match its definition's constness
        void handleCapCommand(const std::string& nickname, std::deque<std::string>& que, bool& capSent); // Removed const from deque
        void handlePartCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params); // Keep all if functional
        void handleKickCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params); // Keep all if functional
        void handleWhoIs(std::shared_ptr<Client> client, std::string param); // Keep all if functional
        void handleTopicCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params); // Keep all if functional
        void handleInviteCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params); // Keep all if functional
        void handlePassCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params);

        // --- NEW ADDITION TO RESOLVE RED UNDERLINE ---
        const std::string& getServerName() const { return _server_name; } // Getter for _server_name


        //whois
        // Keep dummy implementations simple, remove unused parameters
        std::set<std::shared_ptr<Client>> getChannelRecipients(std::shared_ptr<Channel> channel, std::shared_ptr<Client> sender, bool skip_sender); // Keep all if functional
        std::set<std::shared_ptr<Client>> getSharedChannelRecipients(std::shared_ptr<Client> sender, bool skip_sender); // Fixed parameter list, keep if functional
        void broadcastMessage(const std::string& message_content, std::shared_ptr<Client> sender, std::shared_ptr<Channel> target_channel,
            bool skip_sender,
            std::shared_ptr<Client> individual_recipient
        ); // Keep all if functional
};
