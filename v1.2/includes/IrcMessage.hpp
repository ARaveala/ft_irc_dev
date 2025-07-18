#pragma once

#include <vector>
#include <memory>
#include <map>
#include <deque> 
#include <set>
#include <algorithm>
#include <cctype>    
#include <bitset>
#include <functional>

#include "config.h"
#include "IrcResources.hpp"

class Client;
class Server;

class IrcMessage {
	private:
		size_t _bytesSentForCurrentMessage = 0;

		std::deque<std::string> _messageQue;
		
		std::string _prefix;
		std::string _command;

		static std::string to_lowercase(const std::string& s) {
			std::string lower_s = s;
			std::transform(lower_s.begin(), lower_s.end(), lower_s.begin(),
			[](unsigned char c){ return std::tolower(c); });
			return lower_s;
		}
		
		std::map<std::string, int> _nickname_to_fd;
		std::map<int, std::string> _fd_to_nickname;
		std::map<std::string, int> nickname_to_fd;
		std::map<int, std::string> fd_to_nickname;

		static const std::set<std::string> _illegal_nicknames;

		std::vector<std::string> _paramsList;
		std::vector<std::string> _params;
		std::bitset<config::MSG_TYPE_NUM> _msgState;

	public:
    	IrcMessage();
    	~IrcMessage();
    	bool parse(const std::string& rawMessage);
    	std::string toRawString() const;
    	void setPrefix(const std::string& prefix);
    	void setCommand(const std::string& command); 
    	const std::string& getPrefix() const;
    	const std::string& getCommand() const;
    	const std::vector<std::string>& getParams() const;
		const std::string getParam(unsigned long index) const ;
		void printMessage(const IrcMessage& msg);

		void queueMessage(const std::string& msg) { _messageQue.push_back(msg);};
		void queueMessageFront(const std::string& msg) { _messageQue.push_front(msg);};
		void removeQueueMessage() { _messageQue.pop_front(); _bytesSentForCurrentMessage = 0;};
		std::deque<std::string>& getQue() { return _messageQue; };
		std::string& getQueueMessage() { return _messageQue.front();};
		
		void clearQue() {_messageQue.clear();};

		const std::string getMsgParam(int index) const{ return _params[index]; };
		void changeTokenParam(int index, const std::string& newValue) {_paramsList[index] = newValue;};
		const std::vector<std::string>& getMsgParams() { return _params; };
		
		bool isValidNickname(const std::string& nick);
		MsgType check_nickname(std::string nickname, int fd, const std::map<std::string, int>& nick_to_fd);//, std::string& nickref); 
		std::map<int, std::string>& get_fd_to_nickname();
		void remove_fd(int fd, std::map<int, std::string>& fd_to_nick);


		void advanceCurrentMessageOffset(ssize_t bytes_sent);
		size_t getRemainingBytesInCurrentMessage() const;
		const char* getCurrentMessageCstrOffset() const;

};