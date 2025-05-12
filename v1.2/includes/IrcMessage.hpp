#pragma once
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <deque> 
//#define  RPL_NICK(oclient, uclient, client) (":" + oclient + "!" + uclient + "@localhost NICK " +  client + "\r\n")
// Forward declaration (optional but can sometimes help compile times)
// class std::string; // No, string is included above.

// RFC 2812

class Client; // Forward declaration
class Server;

class IrcMessage {
	private:
		// int current_fd;
    	std::string _prefix;
    	std::string _command;
    	std::vector<std::string> _paramsList;
		std::deque<std::string> _messageQue;
	
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
		// naming is changable
		// these are now required as subject requires all activity including read and write
		// must go through epoll
		void queueMessage(const std::string& msg) { _messageQue.push_back(msg);};
		void removeQueueMessage() { _messageQue.pop_front();};
		std::deque<std::string>& getQue() { return _messageQue; };
		std::string getQueueMessage() { return _messageQue.front();};
		void prep_nickname_msg(std::string& nickname,  std::deque<std::string>& messageQue, std::deque<std::string>& broadcastQueue);
		
		void handle_message(Client& Client, const std::string message, Server& server);
		void clearQue() {_messageQue.clear();};
		//void dispatch_nickname(int client_fd, const std::string& oldnick, std::string newnickname, std::map<int, std::shared_ptr <Client>>& clientsMap);
	};