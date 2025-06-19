#pragma once

#include <string>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <sys/epoll.h>
#include <deque> // server messages

#include "ServerError.hpp"
#include "Channel.hpp"
#include "CommandDispatcher.hpp"

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
class Client;
class Channel;

class Server {
	private:
		int _port;
		int _client_count = 0;
		int _fd;
		int _current_client_in_progress;
		int _signal_fd;
		int _epoll_fd;
		int _private_fd = 0;

		std::string _server_name;
		std::string _password;

		std::map<std::string, std::shared_ptr<Channel>> _channels;
		std::map<int, std::shared_ptr<Client>> _Clients; //shared pointer to identify client, also fds, nicknames? can there be unique indentifier?
		std::map<int, int> _timer_map;

		std::map<int, struct epoll_event> _epollEventMap;

		std::deque<std::string> _server_broadcasts; // for broadcasting server wide messages
		std::deque<std::shared_ptr<Channel>> _channelsToNotify;
		
		std::map<std::string, int> _nickname_to_fd;
		std::map<int, std::string> _fd_to_nickname;
		
		std::unique_ptr<CommandDispatcher> _commandDispatcher;
		
	public:
		Server();
		Server(int port, const std::string& password);
		~Server();
	// create things
		void create_Client(int epollfd);
		void createChannel(const std::string& channelName);

		void remove_Client(int client_fd);
		// todo we need a remove channel, when last client leaves channel?
		//void removeClientFromChannels(int fd);
	
		// SETTERS
		void setFd(int fd);
		void set_signal_fd(int fd);
		void set_client_count(int val);
		void set_event_pollfd(int epollfd);
		void set_current_client_in_progress(int fd);
		void set_private_fd(int fd) {_private_fd = fd;};
		void set_nickname_in_map(std::string, int); 

		// GETTERS
		int getPort() const;
		int getFd() const;
		int get_fd(const std::string& nickname) const;
		int get_signal_fd() const;
		int get_client_count() const;
        std::shared_ptr<Client> getClientByNickname(const std::string& nickname);
		int get_event_pollfd() const;
		int get_current_client_in_progress() const;

		std::string get_password() const;
		
		std::shared_ptr<Client> get_Client(int fd);
		std::shared_ptr<Channel> get_Channel(const std::string& channelName);
		
		std::map<int, struct epoll_event> get_struct_map() {return _epollEventMap; };
		std::map<int, std::shared_ptr<Client>> get_Clients() {return _Clients;};		
		std::map<int, std::string>& get_fd_to_nickname();
		std::map<std::string, int>& get_nickname_to_fd();
		std::map<int, std::shared_ptr<Client>>& get_map();

		std::map<std::string, std::shared_ptr<Channel>>& get_channels_map() { return _channels; }
		const std::map<std::string, std::shared_ptr<Channel>>& get_channels_map() const { return _channels; }


		std::deque<std::string>& getBroadcastQueue() { return _server_broadcasts; }
		std::deque<std::shared_ptr<Channel>> getChannelsToNotify() { return _channelsToNotify; }

		void handle_client_connection_error(ErrorType err_type);
		void shutDown();
		bool checkTimers(int fd);
	
		
		void removeQueueMessage() { _server_broadcasts.pop_front();};
		
		// epoll stuff
		int setup_epoll(int epoll_fd, int fd, uint32_t events);
		int setup_epoll_timer(int epoll_fd, int timeout_seconds);
		int create_epollfd();
		int createTimerFD(int timeout_seconds);
		void resetClientTimer(int timer_fd, int timeout_seconds);
		int make_socket_unblocking(int fd);
		void send_message(const std::shared_ptr<Client>& client);
		//void send_server_broadcast();
		//void sendChannelBroadcast();
		
		// channel related 
		bool channelExists(const std::string& channelName) const;
		void broadcastMessageToChannel(std::shared_ptr<Channel> channel, const std::string& message_content, std::shared_ptr<Client> sender, bool skip);
		void broadcastMessageToClients( std::shared_ptr<Client> client, const std::string& msg, bool quit);
		void updateEpollEvents(int fd, uint32_t flag_to_toggle, bool enable);
		void handleJoinChannel(std::shared_ptr<Client> client, const std::string& channelName, const std::string& password);
		void handleReadEvent(int client_fd);
		void handleQuit(std::shared_ptr<Client> client);
		void handleNickCommand(std::shared_ptr<Client> client, std::map<int, std::string>& fd_to_nick, std::map<std::string, int>& nick_to_fd, const std::string& param);
		void handleModeCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params);
		void handleCapCommand(const std::string& nickname, std::deque<std::string>& que, bool& capSent);
		void handlePartCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params);
		void handleKickCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params);
		void handleWhoIs(std::shared_ptr<Client> client, std::string param);
		void handleTopicCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params);
	    void handleInviteCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params);



		//whois
		std::shared_ptr<Client> findClientByNickname(const std::string& nickname);
	// if these work we will swap for broadcasttochannel, breoadcasttoclients
		std::set<std::shared_ptr<Client>> getChannelRecipients(std::shared_ptr<Channel> channel, std::shared_ptr<Client> sender, bool skip_sender);
		std::set<std::shared_ptr<Client>> getSharedChannelRecipients(std::shared_ptr<Client> sender, bool skip_sender);
		void broadcastMessage(const std::string& message_content, std::shared_ptr<Client> sender, std::shared_ptr<Channel> target_channel, bool skip_sender, std::shared_ptr<Client> individual_recipient);
};

std::string generateUniqueNickname();

