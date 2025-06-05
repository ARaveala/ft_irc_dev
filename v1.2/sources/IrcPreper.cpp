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

/*void IrcMessage::prepWelcomeMessage(std::string& nickname)//, std::deque<std::string>& messageQue) {
{
	//setWelcomeType({nickname});
	MessageBuilder::generatewelcome(nickname);
}*/
// yes the parameters are long but this is the best way i could see how to run this function without passing entire objects 
/*void IrcMessage::readyQuit(std::deque<std::string>& channelsToNotify, FuncType::DequeRef1 prepareQuit , int fd, std::function<void(int)> removeClient) {
    //setType();
	prepareQuit(channelsToNotify);
	removeClient(fd);
}*/

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

/*void IrcMessage::getDefinedMsg(MsgType activeMsg, std::deque<std::string>& messageQue) {
	std::string theMessage = CALL_MSG_DYNAMIC(activeMsg, _params...);
	messageQue.push_back(theMessage);
}

//, std::deque<std::string>& messageQue
void IrcMessage::prep_nickname_msg(std::string& nickname, std::deque<std::string>&broadcastQueue)
{	
		//(void)messageQue;
		//std::string test = getParam(0);
		MsgType activeMsg = getActiveMsgType();	
		callDefinedMsg(activeMsg);
		std::string oldnick = nickname;
		nickname = getParam(0);
		//std::string cli = "client"; // username?
		//std::string user_message = RPL_NICK_CHANGE(oldnick, cli, nickname);
		std::string serverBroadcast_message= SERVER_MSG_NICK_CHANGE(oldnick, nickname);

		//getDefinedNickMsg();
		//messageQue.push_back(user_message);
		broadcastQueue.push_back(serverBroadcast_message);

}*/
/*void IrcMessage::prepPart() 
{

}*/

void IrcMessage::prep_join_channel(std::string channleName, std::string nickname, std::deque<std::string>& messageQue, std::string& clientList)
{

	std::string whoJoins = ":" + nickname + " JOIN " + channleName + "\r\n";
	std::string test1 = ":localhost 353 " + nickname + " = " + channleName + " :" + clientList + "\r\n";
	std::string test2 = ":localhost 366 " + nickname + " " + channleName + " :End of /NAMES list\r\n"; // ✅ End of names
	std::string test3 = ":localhost 332 " + nickname + " " + channleName + " :Welcome to " + channleName + "!\r\n"; // ✅ Channel topic
	messageQue.push_back(whoJoins);
	//messageQue.push_back(welcomeToChannel);
	messageQue.push_back(test1);
	messageQue.push_back(test2);
	messageQue.push_back(test3);
}

// the alternative to macros
