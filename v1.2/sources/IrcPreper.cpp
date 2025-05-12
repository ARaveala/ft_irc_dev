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

<<<<<<< HEAD
}
=======
}
>>>>>>> 966456906fe022c3a12f34a2b0a67c4b7c344e8b
