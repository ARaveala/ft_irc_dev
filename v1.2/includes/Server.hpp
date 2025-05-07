#pragma once
#include <string>
#include <map>
#include <memory>
#include "ServerError.hpp"
#include <set>
#include <algorithm> // ai For std::transform
#include <cctype>    // ai For std::tolower, std::islower
#include <vector>    // ai Just for the initial list concept, set is better for lookup
//#include "Client.hpp" // can this be handled withoout including the whole hpp

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
class Server {
	private:
	int _port;
		int _client_count = 0;
		int _fd;
		int _current_client_in_progress;
		int _signal_fd;
		int _epoll_fd;
		std::string _password;
		//std::map<int, std::pair<std::shared_ptr<Client>, int >>_Clients; //unordered map?
		//new
		std::map<int, std::shared_ptr<Client>> _Clients;
		std::map<int, int> _timer_map;
		// start of new section
		std::map<std::string, int> nickname_to_fd;
		std::map<int, std::string> fd_to_nickname;
		static const std::set<std::string> _illegal_nicknames;
		// Helper function to convert a string to lowercase (defined inline in header)
		static std::string to_lowercase(const std::string& s) {
			std::string lower_s = s;
			std::transform(lower_s.begin(), lower_s.end(), lower_s.begin(),
						   [](unsigned char c){ return std::tolower(c); });
			return lower_s;
		}
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
		void remove_Client(int epollfd, int client_fd);
		// remove channel
	
		// SETTERS
		void setFd(int fd);
		void set_signal_fd(int fd);
		void set_client_count(int val);
		void set_event_pollfd(int epollfd);
		void set_current_client_in_progress(int fd);
		void set_nickname_in_map(std::string, int); //todo
		bool check_and_set_nickname(std::string nickname, int fd);  // ai
		// get channel
		
		// GETTERS
		int getPort() const;
		int getFd() const;
		int get_fd(const std::string& nickname) const;  // ai
		int get_signal_fd() const;
		int get_client_count() const;
		int get_event_pollfd() const;
		int get_current_client_in_progress() const;
		std::string get_password() const;
		std::string get_nickname(int fd) const;  // ai
		
		// returns a Client shared_pointer from the map
		std::shared_ptr<Client> get_Client(int fd);
		// returns the whole map 
		std::map<int, std::shared_ptr<Client>>& get_map();
		std::map<int, std::string>& get_fd_to_nickname();
		// message handling
		void handle_client_connection_error(ErrorType err_type);
		void acknowladgeClient();
		void shutdown();
		bool checkTimers(int fd);
		void remove_fd(int fd);  // ai
	};
	
	/**
	 * @example template <bool ReadOnly>
	 typename std::conditional<ReadOnly, const std::map<int, std::pair<std::shared_ptr<Client>, int>>&, std::map<int, std::pair<std::shared_ptr<Client>, int>>&>::type 
	 Server::get_map() {
		return _Clients;
		}
		const std::map<int, std::pair<std::shared_ptr<Client>, int>>& readonly_Clients = server.get_map<true>(); // Read only access
		std::map<int, std::pair<std::shared_ptr<Client>, int>>& modifiable_Clients = server.get_map<false>(); 
		
		this is if we want to create a const return type so that accidental changes can not be made, it would be good practice to 
learn to do so 
 */

