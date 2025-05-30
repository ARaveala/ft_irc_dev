#pragma once
//#include <string> contianed in resources
#include <vector>
#include <memory>
#include <map>
#include <deque> 
#include <set>
#include <algorithm> // ai For std::transform
#include <cctype>    
#include "IrcResources.hpp"
#include <bitset>
#include <functional>
#include "config.h"
class Client; // Forward declaration
class Server;

class IrcMessage {
	private:
		// int current_fd;
		std::bitset<config::MSG_TYPE_NUM> _msgState;  //tracks active error
	    std::vector<std::string> _params;
		//MsgType _activeMsg = MsgType::NONE;

    	std::string _prefix;
    	std::string _command;
    	std::vector<std::string> _paramsList;
		std::deque<std::string> _messageQue;

		std::map<std::string, int> _nickname_to_fd;
		std::map<int, std::string> _fd_to_nickname;
		std::map<std::string, int> nickname_to_fd;
		std::map<int, std::string> fd_to_nickname;
		static const std::set<std::string> _illegal_nicknames;
	
		static std::string to_lowercase(const std::string& s) {
			std::string lower_s = s;
			std::transform(lower_s.begin(), lower_s.end(), lower_s.begin(),
						   [](unsigned char c){ return std::tolower(c); });
			return lower_s;
		}
	public:
    	IrcMessage();
    	~IrcMessage();
    	// parse incoming
    	bool parse(const std::string& rawMessage);
    	// parse outgoing
    	std::string toRawString() const;
    	void setPrefix(const std::string& prefix);
    	void setCommand(const std::string& command); 
    	const std::string& getPrefix() const;
    	const std::string& getCommand() const;
    	const std::vector<std::string>& getParams() const;
		const std::string getParam(unsigned long index) const ;
		void printMessage(const IrcMessage& msg);

		
		
		// araveala has added this to help give you full control
		int countOccurrences(const std::string& text, const std::string& pattern);
		// naming is changable
		// these are now required as subject requires all activity including read and write
		// must go through epoll
		void queueMessage(const std::string& msg) { _messageQue.push_back(msg);};
		void removeQueueMessage() { _messageQue.pop_front();};
		std::deque<std::string>& getQue() { return _messageQue; };
		std::string getQueueMessage() { return _messageQue.front();};
		void prep_nickname(std::string& nickname, int client_fd, std::map<int, std::string>& fd_to_nick, std::map<std::string, int>& nick_to_fd);
		//void prep_nickname_inuse(std::string& nickname, std::deque<std::string>& messageQue);
		void prep_join_channel(std::string channleName, std::string nickname, std::deque<std::string>& messageQue, std::string& clientList);
		void prepWelcomeMessage(std::string& nickname);//, std::deque<std::string>& messageQue);
		// apprenbtly this is normal, can look at mariadb databse code, huge signitures but small code bodies. no need to pass entire object 
		void readyQuit(std::deque<std::string>& channelsToNotify, std::function<void(std::deque<std::string>&)>, int fd, std::function<void(int)> removeClient);
		//void handle_message(Client& Client, const std::string message, Server& server);
		void clearQue() {_messageQue.clear();};


		MsgType getActiveMsgType() const;


		// moving from server to here 
		bool check_and_set_nickname(std::string nickname, int fd, std::map<int, std::string>& fd_to_nick, std::map<std::string, int>& nick_to_fd);//, std::string& nickref);  // ai
		std::map<int, std::string>& get_fd_to_nickname();
		void remove_fd(int fd, std::map<int, std::string>& fd_to_nick); // ai // we have remove client function , this could be called in there, to remove all new maps
		std::string get_nickname(int fd, std::map<int, std::string>& fd_to_nick) const;  // ai
		int get_fd(const std::string& nickname) const;
		//void dispatch_nickname(int client_fd, const std::string& oldnick, std::string newnickname, std::map<int, std::shared_ptr <Client>>& clientsMap);

		// message types and params
		void setType(MsgType msg, std::vector<std::string> sendParams); // using bitsets to switch on enum message definer
		
		void setWelcomeType(std::vector<std::string> sendParams);
	
	    void callDefinedMsg();//(MsgType msgType);
		void callDefinedBroadcastMsg(std::deque<std::string>& channelbroadcast);
		//void getDefinedMsg(MsgType activeMsg, std::deque<std::string>& messageQue);
		// cleanup functions
		void clearAllMsg() {
			_nickname_to_fd.clear();
			_fd_to_nickname.clear();
			nickname_to_fd.clear();
			fd_to_nickname.clear();
			//_illegal_nicknames.clear()
		};





};