#include <CommandDispatcher.hpp>
#include <iostream>
#include "Server.hpp"
#include "Client.hpp"
#include "IrcMessage.hpp"
#include <cctype>
#include <regex>
#include "IrcResources.hpp"
#include "MessageBuilder.hpp"
#include "config.h"


// these where static but apparanetly that is not a good approach 
CommandDispatcher::CommandDispatcher(Server* server_ptr) :  _server(server_ptr){
	if (!_server) {
        throw std::runtime_error("CommandDispatcher initialized with nullptr Server!");
    }
	// what if __server == null?
}
CommandDispatcher::~CommandDispatcher() {}
/*void CommandDispatcher::handleChannelModes(std::shared_ptr<Channel> currentChannel, IrcMessage currentMsg, int fd)
{

}*/

/*void CommandDispatcher::manageQuit(const std::string& quitMessage, std::deque<std::shared_ptr<Channel>> channelsToNotify)
{



}*/
/*bool CommandDispatcher::initialModeValidation(std::shared_ptr<Client> client, const std::vector<std::string>& params, 	ModeCommandContext& context){
	if (params.empty()) {
    	client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NEED_MORE_PARAMS, {client->getNickname(), "MODE"}));
        return false;
    }
	context.target = params[0];
    context.targetIsChannel = (context.target[0] == '#');

// updatepoll, can do if returned false and check the mess
    std::shared_ptr<Channel> channel;
	std::string target = params[0];
    bool targetIsChannel = (target[0] == '#'); 
	if (params.size() == 1) {
       	if (targetIsChannel) {
	        client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::RPL_CHANNELMODEIS, {channel->getName(), channel->getCurrentModes()}));
	    } else {
	        client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::RPL_UMODEIS, {client->getNickname(), client->getCurrentModes()}));
	    }
	    return false;
	} if (targetIsChannel) {
        if (!_server->channelExists(params[0])) {
            client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NO_SUCH_CHANNEL, {params[0]}));
            return false;
        }
		context.channel = _server->get_Channel(context.target);
        //channel = _server->get_Channel(params[0]);
        if (!channel->isClientInChannel(client->getNickname())) {
            client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NOT_IN_CHANNEL, {client->getNickname(), params[0]}));
            return false;
		} if (!channel->getClientModes(client->getNickname()).test(Modes::OPERATOR)) {
			client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NOT_OPERATOR, {client->getNickname(), channel->getName()}));
			return false;
		}
	} else {
		if (target != client->getNickname()) {
			//client cant set private modes on other clients 
			client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::INVALID_TARGET, {target}));
	    	return false;
		}
	}
	return true;
}*/

/*bool CommandDispatcher::modeSyntaxValidator(std::shared_ptr<Client> client, const std::vector<std::string>& params, const ModeCommandContext& context) {
	size_t currentIndex = 1; // skip channel

	char currentSign = ' ';

	while (currentIndex < params.size()) {
		const std::string& currentToken = params[currentIndex];
		bool isModeGroupToken = (currentToken.length() > 0 && (currentToken[0] == '+' || currentToken[0] == '-'));
		if (currentIndex == 1 && isModeGroupToken == false) {
			// abort first set must be mode flags or such alike
			return false;
		}
		if (isModeGroupToken) {
            // If it's a mode group token, set the sign and iterate through its characters
            currentSign = currentToken[0];
			 for (size_t i = 1; i < currentToken.length(); ++i) {
                char modeChar = currentToken[i];
				bool modeCharValid = false;
                if (context.targetIsChannel) {
                    modeCharValid = context.channel->isValidChannelMode(modeChar) || context.channel->isValidClientMode(modeChar);
                } else { // Client private mode
                    modeCharValid = client->isValidPrivClientMode(modeChar);
				}
			    if (!modeCharValid) {
                    std::cout << "DEBUG: Syntax Error: Unknown mode char '" << modeChar << "'." << std::endl;
                    client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::UNKNOWN_MODE, {std::string(1, modeChar), client->getNickname()}));
                    return false; // Abort on first unknown mode
                }
				bool paramExpected = context.channel->channelModeRequiresParameter(modeChar);
				if (paramExpected) {
                	// Check for missing parameter

                	if (currentIndex + 1 >= params.size()) {
                	    std::cout << "DEBUG: Syntax Error: Mode '" << modeChar << "' requires a parameter but none found." << std::endl;
                	    client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NEED_MORE_PARAMS, {client->getNickname(), "MODE", std::string(1, modeChar)}));
                	    return false; // Abort on missing parameter
                	}
					currentIndex++;

				}
				const std::string& modeParam = params[currentIndex];

                // --- 1c. Validate parameter content ---
                if (context.targetIsChannel) {
                    if (modeChar == 'o' || modeChar == 'v') { // Client modes within a channel (+o, +v)
                        // Username must start with lowercase AND exist on server
                        if (modeParam.empty() || !std::islower(modeParam[0]) || !_server->clientExists(modeParam)) { // this should be is on channel, we dont care if it exists
                            std::cout << "DEBUG: Syntax Error: Invalid or non-existent user '" << modeParam << "' for mode '" << modeChar << "'." << std::endl;
                            client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NO_SUCH_NICK, {client->getNickname(), modeParam}));
                            return false; // Abort on invalid/non-existent user
                        }
                    } else if (modeChar == 'l' && currentSign == '+') { // Set user limit (+l)
                        try {
                            if (modeParam.empty()) throw std::invalid_argument("empty");
                            std::stoul(modeParam); // Throws if not a valid unsigned long we will add a lmit max check here 
                        } catch (const std::exception& e) {
                            std::cout << "DEBUG: Syntax Error: Invalid numeric parameter '" << modeParam << "' for mode '" << modeChar << "'." << std::endl;
                            // message here shouyld be limit too high or negative not allowed 
							client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NEED_MORE_PARAMS, {client->getNickname(), "MODE", std::string(1, modeChar), "Invalid limit"}));
                            return false; // Abort on invalid numeric
                        }
                    } else if (modeChar == 'k' && currentSign == '+') { // Set channel key/password (+k)
                        if (modeParam.empty()) {
                            std::cout << "DEBUG: Syntax Error: Empty password parameter for mode '" << modeChar << "'." << std::endl;
                            // check password restrictions
							client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NEED_MORE_PARAMS, {client->getNickname(), "MODE", std::string(1, modeChar), "Empty password"}));
                            return false; // Abort on empty password
                        }
                    }
                }
                // If this `modeChar` required and consumed a parameter, we must advance the main `currentParamIndex` by 1
                // more than the mode group token itself when we exit this `if (isModeGroupToken)` block.
                // This will be handled by incrementing currentPa
			}

		}
		else { // Current token is NOT a mode group (doesn't start with '+' or '-')
                // This token must be an unexpected parameter or malformed command.
                // This means the previous mode group didn't consume it, and it's not a new mode group.
                std::cout << "DEBUG: Syntax Error: Unexpected token '" << currentToken << "'. Expected mode group or end of command." << std::endl;
                client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NEED_MORE_PARAMS, {client->getNickname(), "MODE", currentToken}));
                return false; // Abort the entire command
            }
        } // End while loop over params

        return true; // A
}*/

#include <sys/socket.h>
#include <iomanip>
void CommandDispatcher::dispatchCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params)
{
	int client_fd = client->getFd();
    if (!client) {
        std::cerr << "Error: Client pointer is null in dispatchCommand()" << std::endl;
        return;
    }
	client->getMsg().printMessage(client->getMsg());
	std::string command = client->getMsg().getCommand();
	std::cout<<"handling cap with param = "<<params[1]<<"\n";
	// this may not be needed but could be the soruce of inconsitent behaviour, it has not fixed it ;(
	if (command == "CAP")
	{
		client->getMsg().queueMessage(":localhost CAP " + client->getNickname() + " LS :multi-prefix sasl\r\n");
        client->getMsg().queueMessage(":localhost CAP " + client->getNickname() + " ACK :multi-prefix\r\n");
        client->getMsg().queueMessage(":localhost CAP " + client->getNickname() + " ACK :END\r\n");
		client->setHasSentCap();
	}
	if (command == "QUIT")
	{
		std::cout<<"QUIT CAUGHT IN command list handlking here --------------\n";
		_server->handleQuit(client);
		return ;
	}

	if (command == "USER")
	{
		client->getMsg().queueMessage("USER " + client->getClientUname() + " 0 * :" + client->getfullName() +"\r\n");
		client->setHasSentUser();
		//return ;
	}
	if (command == "NICK") {
		if (client->getHasSentNick() == false)
		{
			std::cout<<"@@@@@@@@@@{{{{{{{{7777777777777777777CLIENT NICKNAME IS BEING SENT 1ST TIME--------------------ooooooooommmmmmmmmmmmmmm--------------\n";
			//std::cout<<"CHECKING PARAM WE ARE CHANGING"<<client->getMsg().getParam(0)<<"\n";

			client->getMsg().queueMessage(":" + params[0]  + "!user@localhost"+ " NICK " +  client->getNickname() + "\r\n");

			_server->updateEpollEvents(client_fd, EPOLLOUT, true);
			client->setHasSentNick();
			return;
			//std::cout<<"CHECKING PARAM AAAFTER WE HAVE CHANGING"<<client->getMsg().getParam(0)<<"\n";

		}

		client->setOldNick(client->getNickname()); // we might not need this anymore 
		client->getMsg().prep_nickname(client->getClientUname(), client->getNicknameRef(), client_fd, _server->get_fd_to_nickname(), _server->get_nickname_to_fd()); // 
		_server->handleNickCommand(client);
		return ; 
	}
	if (!client->getHasRegistered() && client->getHasSentNick() && client->getHasSentUser()) {
		client->setHasRegistered();
		std::string msg = MessageBuilder::generatewelcome(client->getNickname());
		client->getMsg().queueMessage(msg);
		// stating end of registartion 
		client->getMsg().queueMessage(":localhost 375 " + client->getNickname() + " :You are now active.\r\n");
		client->getMsg().queueMessage(":localhost 376 " + client->getNickname() + " :End of MOTD\r\n");

		_server->updateEpollEvents(client_fd, EPOLLOUT, true);
		

	}
	if (command == "PING"){
		std::cout<<"sending pong back "<<std::endl;
		client->getMsg().queueMessage("PONG :localhost/r/n");
		client->getMsg().queueMessage(":localhost NICK " + client->getNickname() + "\r\n");
		_server->updateEpollEvents(client->getFd(), EPOLLOUT, true);
		return;
		//Client->set_failed_response_counter(-1);
		//resetClientTimer(Client->get_timer_fd(), config::TIMEOUT_CLIENT);
	}
	if (command == "PONG"){
		std::cout<<"sending ping back "<<std::endl;
		
		client->getMsg().queueMessage("PING :localhost/r/n");
		
		
		_server->updateEpollEvents(client->getFd(), EPOLLOUT, true);
		return;

	}

    if (command == "JOIN"){

		std::cout<<"JOIN CAUGHT LETS HANDLE IT \n";
		if (!params[0].empty())
		{
			// HANDLE HERE 
			_server->handleJoinChannel(client, params[0], params[1]);
			//client->getMsg().queueMessage("WHO " + client->getNickname() + "\r\n");
			//client->getMsg().queueMessage(":localhost NOTICE * :Processing update...\r\n");
			
			// handle join
			// ischannel
			// if (!ischannel) , createChannel(), setChannelDefaults() updateChannalconts()?, confirmOperator()
			// else if (ischannel), isinvite(), hasinvite(), ChannelhasPaswd(), clientHasPasswd()/passwrdMatch(),
			// hasBan(), joinChannel() updateChannalconts()
			//		
			/**
			 * @brief checks
			 * look through list of channel names to see if channel exists // std::map<std::string, Channel*> channels
			 * 	if doesnt exist - create it with default settings // what are default settings?
			 * set max size? // is the is default channel std::int _maxSize also flag -n 
			 * set current number of clients in side the channel // channel std::int _nClients
			 * add channel to vector of channels client has joined //  <Client> _joinedchannels
			 * add current client to channel operator // channel std::string _operator OR
			 * adjust bitset map
			 * 
			 * if does exist - loop through and find if channel is invite only // channel std::bool _inviteOnly channel std::set _currentUsers, _invitedUsers
			 * if it is invite only, does client have invite isnide channel.
			 * 
			 * is it password protected.
			 * if it is password protected, did user provide password. if not then user can not enter
			 * if yes, does password match
			 * 
			 *  is client banned from channel.
			 * 
			 * assuming checks passed, client can now join channel
			 * add client to list of clients on channel // channel > list of clients
			 *  if this clients is first on the channel, set the flag to -o // channel > who is -o? can be only one.

			 * 
			 */
	    /*if (getCommand() == "KICK") {
			
	    }

	    PART
	    LEAVE
	    TOPIC
	    NAMES
	    LIST
	    INVITE
	    PARAMETER NICKNAME
		
*/
		}
		else// if(client->getHasSentCap() == true) 
		{
			
			if (client->getHasSentCap() == true)
			{
				std::cout<<"hhhhhhhhhhhhhh    ENTERING join on first HANDLING \n";
				client->getMsg().queueMessage(":localhost 322 " + client->getNickname() + " #dummychannel 0 :\r\n");
				client->getMsg().queueMessage(":localhost 323 " + client->getNickname() + " :End of channel list\r\n");
				_server->updateEpollEvents(client->getFd(), EPOLLOUT, true);

			}
		}
	}

	/**
	 * @brief 
	 * 
	 * sudo tcpdump -A -i any port <port number>, this will show what irssi sends before being reecived on
	 * _server, irc withe libera chat handles modes like +io +o user1 user2 but , i cant find a way to do that because
	 * irssi is sending the flags and users combined together into 1 string, where does flags end and user start? 
	 * 
	 * so no option but to not allow that format !
	 * 
	 */
	if (command == "MODE")
	{


		_server->handleModeCommand(client, params);

		// · i: Set/remove Invite-only channel
        //· t: Set/remove the restrictions of the TOPIC command to channel operators
        //· k: Set/remove the channel key (password)
		//· o: Give/take channel operator privilege
		//· l: Set/remove the user limit to channel

	}
	if (client->getMsg().getCommand() == "PRIVMSG") 
	{
		if (!params[0].empty())
		{
			if (params[0][0] == '#')
			{
				std::cout<<"stepping into priv message handling \n";
				std::string buildMessage;
				// right now we loop through all params, do we need to check for symbols ? like# or modes
				// we need to adjust the space handling in the message class as righ tnow we loose all spaces 
				for (size_t i = 1; i < params.size(); i++)
				{
					buildMessage += params[i];
				}
				// eed to check does channel exist
				if (_server->channelExists(params[0]) == true) {
					// MessageBuilder 
					_server->broadcastMessageToChannel(_server->get_Channel(params[0]),":" + client->getNickname()  + " PRIVMSG " + params[0] + " " + buildMessage + "\r\n", client);
				}
				// check is it a channel name , starts with #, collect to _serverchannelbroadcast
				// is it just a nickname , collect to messageQue, send to only that fd of
	
			}
			else
			{
				int fd = _server->get_nickname_to_fd().find(params[0])->second;
				// check against end()
				if (fd < 0)
				{
					// no user found by name
					// no username provided
					std::cout<<"no user here by that name \n"; 
					return ;
				}
				_server->get_Client(fd)->getMsg().queueMessage( ":" + client->getNickname() + " PRIVMSG "  + params[0]  + " " + params[1] + "\r\n");
				if (!_server->get_Client(fd)->isMsgEmpty()) {
					_server->updateEpollEvents(fd, EPOLLOUT, true);
				}
				
			}
		}
		if (!params[0].empty())
		{
			// handle error or does irssi handle
		}
		
	}
	if (command == "WHOIS")
	{
		_server->handleWhoIs(client, params[0]);
		//client->getMsg().queueMessage("RECONNECT\r\n");

	}
}