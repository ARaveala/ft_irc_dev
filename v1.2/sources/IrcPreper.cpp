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

void IrcMessage::prepWelcomeMessage(std::string& nickname, std::deque<std::string>& messageQue) {
	std::string welcome = ":server 001 " + nickname + " :Welcome to the IRC server\r\n";
	std::string extra1 = ":server 002 " + nickname + " :Your host is ft_irc, running version 1.0\r\n";
	std::string extra2 = ":server 003 " + nickname + " :This server was created today\r\n";
	std::string extra3 = ":server 004 " + nickname + " ft_irc 1.0 o o\r\n";
	messageQue.push_back(welcome);
	messageQue.push_back(extra1);
	messageQue.push_back(extra2);
	messageQue.push_back(extra3);
	//IRCMessage::welcome_msg, strlen(IRCMessage::welcome_msg)
}

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
void IrcMessage::prep_join_channel(std::string channleName, std::string& nickname, std::deque<std::string>& messageQue)
{
	//std::string whoJoins = ":" + nickname + " JOIN :#" + channleName + "\r\n";
	std::string whoJoins = ":" + nickname + " JOIN " + channleName + "\r\n";

	// this message needs adjustment
	//std::string welcomeToChannel = ":ft_irc PRIVMSG #" + channleName + " :Welcome to our channel!\r\n";
	std::string test1 = ":ft_irc 353 " + nickname + " = " + channleName + " :user1!user1@localhost user2!!user2@localhost user3!!user3@localhost\r\n"; // ✅ Name list
	std::string test2 = ":ft_irc 366 " + nickname + " " + channleName + " :End of /NAMES list\r\n"; // ✅ End of names

	std::string test3 = ":ft_irc 332 " + nickname + " " + channleName + " :Welcome to " + channleName + "!\r\n"; // ✅ Channel topic

	
//	std::string welcomeToChannel = ":ft_irc 332 " + nickname + " #" + channleName + " :Welcome to our channel!\r\n";
	messageQue.push_back(whoJoins);
	//messageQue.push_back(welcomeToChannel);
	messageQue.push_back(test1);
	
	messageQue.push_back(test2);
	messageQue.push_back(test3);
	
	// list all memebers by nick
	//messageQue.push_back(welcomeToChannel);
	/**
	 * @brief :server 353 Nickname = #channelName :Alice Bob Charlie
				:server 366 Nickname #channelName :End of /NAMES list

	 * 
	 */
}