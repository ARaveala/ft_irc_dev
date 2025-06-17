#include "Client.hpp"
//#include "IrcResources.hpp"
#include <memory>
#include "IrcMessage.hpp"
#include "config.h"
#include <sys/socket.h>
#include <string>
#include <iostream>
#include <deque>
#include "MessageBuilder.hpp"
/**
 * @brief here we prepare the message queues fore sending in what will ultimatley be the
 * one and only sending function . We pass everything by reference so we dont have to pass the objects, 
 * doing so allows us to change the value of the varaible without risking access to anything else inside the classes.
 * 
 * @param nickname reference  
 * @param messageQue refernce
 * @param broadcastQueue reference
 */



void IrcMessage::prep_nickname(const std::string& username, std::string& nickname, int client_fd, std::map<int, std::string>& fd_to_nick, std::map<std::string, int>& nick_to_fd)
{	if (username.empty())
		std::cout<<"asdasdasd\n";
	if (check_and_set_nickname(getParam(0), client_fd, fd_to_nick, nick_to_fd))
	{
		std::string oldnick = nickname;
		nickname.clear();
		nickname = getParam(0);
		setType(MsgType::RPL_NICK_CHANGE, {oldnick, username, nickname});
		
	} 
}
void IrcMessage::prep_join_channel(std::string channleName, std::string nickname, std::deque<std::string>& messageQue, std::string& clientList)
{
	
	std::string whoJoins = ":" + nickname + " JOIN " + channleName + "\r\n";
	std::string test1 = ":localhost 353 " + nickname + " = " + channleName + " :" + clientList + "\r\n";
	std::string test2 = ":localhost 366 " + nickname + " " + channleName + " :End of /NAMES list\r\n"; // ✅ End of names
	std::string test3 = ":localhost 332 " + nickname + " " + channleName + " :Welcome to " + channleName + "!\r\n"; // ✅ Channel topic
	messageQue.push_back(whoJoins);
	//messageQue.push_back(welcomeToChannel);
	messageQue.push_back(test3);
	messageQue.push_back(test1);
	messageQue.push_back(test2);
}

// the alternative to macros
