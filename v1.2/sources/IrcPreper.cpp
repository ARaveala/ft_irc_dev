#include "Client.hpp"
#include "IrcResources.hpp"
#include <memory>
#include "IrcMessage.hpp"

#include <sys/socket.h>
#include <string>
#include <iostream>
#include <deque>

/**
 * @brief here we prepare the message queues fore sending in what will ultimatley be the
 * one and only sending function . We pass everything by reference so we dont have to pass the objects, 
 * doing so allows us to change the value of the varaible without risking access to anything else inside the classes.
 * 
 * @param nickname reference  
 * @param messageQue refernce
 * @param broadcastQueue reference
 */
void IrcMessage::prep_nickname_msg(std::string& nickname, std::deque<std::string>& messageQue, std::deque<std::string>&broadcastQueue)
{	
		std::string test = getParam(0);

		std::string oldnick = nickname;
		nickname = getParam(0);
		std::string cli = "client"; // username?
		std::string user_message = RPL_NICK_CHANGE(oldnick, cli, nickname);
		std::string serverBroadcast_message= SERVER_MSG_NICK_CHANGE(oldnick, nickname);

		messageQue.push_back(user_message);
		broadcastQueue.push_back(serverBroadcast_message);

}
void IrcMessage::prep_nickname_inuse(std::string& nickname, std::deque<std::string>& messageQue)
{
			// error codes for handlinh error messages or they should be handled in check and set . 
			//std::string test2 = ":localhost 433 "  + getParam(0) + " " + getParam(0) + "\r\n";
			std::string test2 = NICK_INUSE(nickname);
			messageQue.push_back(test2);
			//send(Client.getFd(), test2.c_str(), test2.length(), 0); // todo what is correct format to send error code
}