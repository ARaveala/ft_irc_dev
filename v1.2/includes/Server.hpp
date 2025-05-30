#pragma once
#include <string>
#include <map>
#include <memory>
#include "ServerError.hpp"
#include <set>
// ai For std::tolower, std::islower
#include <vector>    // ai Just for the initial list concept, set is better for lookup
//#include "Client.hpp" // can this be handled withoout including the whole hpp
#include <sys/epoll.h>
#include <deque> // server messages
#include "Channel.hpp"
// connection registration
// https://modern.ircdocs.horse/#connection-registration



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
		
		std::string _password;

		std::map<const std::string, std::shared_ptr<Channel>> _channels;
		std::map<int, std::shared_ptr<Client>> _Clients;
		std::map<int, int> _timer_map;

		// start of new section
		////std::map<std::string, int> nickname_to_fd;
		////std::map<int, std::string> fd_to_nickname;
		std::map<int, struct epoll_event> _epollEventMap;
		////static const std::set<std::string> _illegal_nicknames;
		// Helper function to convert a string to lowercase (defined inline in header)
		std::deque<std::string> _server_broadcasts; // for broadcasting server wide messages
		std::deque<std::string> _channel_broadcasts;
		std::deque<std::string> _channelsToNotify;
		/*static std::string to_lowercase(const std::string& s) {
			std::string lower_s = s;
			std::transform(lower_s.begin(), lower_s.end(), lower_s.begin(),
						   [](unsigned char c){ return std::tolower(c); });
			return lower_s;
		}*/
		// end of new section
		
		// loop through both to find when ping pong 
		// map client fd to sent ping time
		// map client fd to last sent message 
		
		// num of channels 
		// string = name of channel channle = channel object
		// std::map<std::string, std::shared_ptr<Channel>> chanels
		
		std::map<std::string, int> _nickname_to_fd;
		std::map<int, std::string> _fd_to_nickname;
		

		// Using std::map for nicknames; use std::unordered_map if preferred
		// #include <unordered_map>
		
	public:
		Server();
		Server(int port, std::string password);
		~Server();

		// setters
		//void set_port(int const port);
		//void set_password(std::string const password);
		void create_Client(int epollfd);
		void remove_Client(int client_fd);
		void removeClientFromChannels(int fd);
		// remove channel
	
		// SETTERS
		void setFd(int fd);
		void set_signal_fd(int fd);
		void set_client_count(int val);
		void set_event_pollfd(int epollfd);
		void set_current_client_in_progress(int fd);
		void set_private_fd(int fd) {_private_fd = fd;};
		void set_nickname_in_map(std::string, int); //todo
		////bool check_and_set_nickname(std::string nickname, int fd);  // ai
		// get channel
		
		void setEpollout(int fd);

		
		// GETTERS
		int getPort() const;
		int getFd() const;
		int get_fd(const std::string& nickname) const;  // ai
		int get_signal_fd() const;
		int get_client_count() const;
		int get_event_pollfd() const;
		int get_current_client_in_progress() const;
		
		//epoll_event& get_epoll_event_struct(int fd);
		std::map<int, struct epoll_event> get_struct_map() {return _epollEventMap; };
		std::string get_password() const;
		////std::string get_nickname(int fd) const;  // ai
		std::deque<std::string>& getBroadcastQueue() { return _server_broadcasts; }
		std::deque<std::string>& getChannelBroadcastQue() { return _channel_broadcasts; }
		std::deque<std::string>& getChannelsToNotify() { return _channelsToNotify; }
		// returns a Client shared_pointer from the map
		std::shared_ptr<Client> get_Client(int fd);
		std::shared_ptr<Channel> get_Channel(std::string channelName);
		std::map<int, std::string>& get_fd_to_nickname();
		std::map<std::string, int>& get_nickname_to_fd();
		// returns the whole map 
		std::map<int, std::shared_ptr<Client>>& get_map();
		////std::map<int, std::string>& get_fd_to_nickname();
		// message handling
		void handle_client_connection_error(ErrorType err_type);
		void acknowladgeClient();
		void shutdown();
		bool checkTimers(int fd);
		////void remove_fd(int fd); // ai // we have remove client function , this could be called in there, to remove all new maps
		void removeQueueMessage() { _server_broadcasts.pop_front();};
		// epoll stuff
		int setup_epoll(int epoll_fd, int fd, uint32_t events);
		int setup_epoll_timer(int epoll_fd, int timeout_seconds);
		int create_epollfd();
		int createTimerFD(int timeout_seconds);
		void resetClientTimer(int timer_fd, int timeout_seconds);
		void send_message(std::shared_ptr<Client> client);
		void send_server_broadcast();
		void sendChannelBroadcast();
		// channel related 
		bool channelExists(const std::string& channelName) const;
		void createChannel(const std::string& channelName);//, Client& client
		Channel* getChannel(const std::string& channelName);
		void handleJoinChannel(std::shared_ptr<Client> client, const std::string& channelName, const std::string& password);

};

std::string generateUniqueNickname();

