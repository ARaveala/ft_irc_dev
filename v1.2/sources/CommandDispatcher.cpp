#include <CommandDispatcher.hpp>
#include <iostream>
#include "Server.hpp"
#include "Client.hpp"
#include "IrcMessage.hpp"
#include <cctype>
#include <regex>
#include "IrcResources.hpp"
#include "MessageBuilder.hpp"


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
	// this may not be needed but could be the soruce of inconsitent behaviour, it has not fixed it ;(
	if (command == "CAP")
	{
		/*if (params[0] == "LS")
		{
			client->getMsg().queueMessage(":localhost CAP * LS :multi-prefix sasl\r\n");

			_server->updateEpollEvents(client->getFd(), EPOLLOUT, true);
		}
			return;*/
		if (params[0] == "END")
			return;
		std::string msg = MessageBuilder::buildCapResponse(client->getNickname(), params[1]);
		_server->updateEpollEvents(client->getFd(), EPOLLOUT, true);
		client->getMsg().queueMessage(msg);
		client->setHasSentCap();
	}
	if (command == "QUIT")
	{
		std::cout<<"QUIT CAUGHT IN command list handlking here --------------\n";
		_server->handleQuit(client);
		return ;
	}

	/*if (command == "WHOIS")
	{
		if (!params[0].empty() && params[0] != client->getClientUname())
		{
			client->setClientUname(params[0]);
		}
		//return;
	}*/
	if (command == "USER")
	{
		client->setHasSentUser();
		//return ;
	}
	if (command == "NICK") {
		if (client->getHasSentNick() == false)
		{
			std::cout<<"@@@@@@@@@@{{{{{{{{7777777777777777777CLIENT NICKNAME IS BEING SENT 1ST TIME--------------------ooooooooommmmmmmmmmmmmmm--------------\n";
			std::cout<<"CHECKING PARAM WE ARE CHANGING"<<client->getMsg().getParam(0)<<"\n";

			//client->getMsg().changeTokenParam(0, client->getNickname());
			client->getMsg().queueMessage(":localhost 433 * "+ params[0] + " :Nickname is already in use.\r\n");
			//client->getMsg().queueMessage(":" + oldnick + "!" + client->getNickname() + "@localhost NICK " +  client->getNickname() + "\r\n";);
			_server->updateEpollEvents(client_fd, EPOLLOUT, true);
			client->setHasSentNick();
			return;
			//std::cout<<"CHECKING PARAM AAAFTER WE HAVE CHANGING"<<client->getMsg().getParam(0)<<"\n";

		}
		client->setOldNick(client->getNickname()); // we might not need this anymore 
		client->getMsg().prep_nickname(client->getClientUname(), client->getNicknameRef(), client_fd, _server->get_fd_to_nickname(), _server->get_nickname_to_fd()); // 
		_server->handleNickCommand(client);
		//sendToClient(client.fd, forcePing);
		//std::string message = MessageBuilder::generateMessage(client->getMsg().getActiveMessageType(), client->getMsg().getMsgParams());;
		//send(client->getFd(), message.c_str(), message.size(), 0);
		//client->safeSend(client->getFd(), "PONG :server/r/n");
		//shutdown(client->getFd(), SHUT_WR); 
		return ; 
	}
	if (!client->getHasRegistered() && client->getHasSentNick() && client->getHasSentUser()) {
		client->setHasRegistered();
		//usleep(5000);
		//client->getMsg().queueMessage(":server 001 " + client->getNickname() + " :Welcome to IRC!\r\n");
		std::string msg = MessageBuilder::generatewelcome(client->getNickname());
		client->getMsg().queueMessage(msg);
		

	}
	if (command == "PING"){
		client->sendPong();
		std::cout<<"sending pong back "<<std::endl;
		//Client->set_failed_response_counter(-1);
		//resetClientTimer(Client->get_timer_fd(), config::TIMEOUT_CLIENT);
	}
	if (command == "PONG"){
		std::cout<<"------------------- we recived pong inside message handling haloooooooooo"<<std::endl;
	}

    if (command == "JOIN"){

		std::cout<<"JOIN CAUGHT LETS HANDLE IT \n";
		if (!params[0].empty())
		{
			_server->handleJoinChannel(client, params[0], params[1]);
			
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
			
			client->getMsg().queueMessage(":localhost 461 " + client->getNickname() + " JOIN :Not enough parameters\r\n");
			_server->updateEpollEvents(client->getFd(), EPOLLOUT, true);
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
		std::cout<<"&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&ENTERING MODE HANDLING \n";
		std::cout<<"TELL ME THE SIZE OF PARAMSLIST = "<<params.size()<<"\n";
		if (params.size() == 2)
		{
			// is name given clients name 
			if (params[0] != client->getNickname())
			{
				std::cout<<"client can only change own modes, unless in channel\n";
			}
			// temporary for registartion debugging
			// should do sign var, and mode var
			if (params[1] == "+i")
			{
				client->setInvis(true);
				client->getMsg().queueMessage(":**:" + params[0] + " MODE " + params[0] +" :+i\r\n**");
				_server->updateEpollEvents(client->getFd(), EPOLLOUT, true);

			}
		}
//:**:anonthe_evill MODE anonthe_evill :+i\r\n**
		/*
		// re write starts
		// get raw message params 
		std::vector<std::string> params = client->getMsg().getParams();
		if (params.empty())
		{
			// build RPL_NEEDMOREPARAMS (461) //set msgtype //client, "", "461", "Not enough parameters"
			return ; // nothing to process
		}
		// check that channel exists 
		// will this check be enough , do we need to check the # at the begining we may need 
		// to remove it from sent string--------!!!
		if (_server.channelExists(params[0]) == false) // && params[0][0] == #
		{
			// channel does to exists error message 403 RPL_NOSUCHCHANNEL.
			return;
		}
		std::shared_ptr <Channel> currentChannel = _server.get_Channel(client->getMsg().getParam(0));
		// check client is in channel 
		if (currentChannel->isClientInChannel(client->getNickname()))
		{
			// client is not in channel where request is made. permission denied
			return ;
		}
		// initial checks have been made , we handle channel modes.
		// fucntion signiture would look like this ....





		// re write ends 
*/
		std::shared_ptr <Channel> currentChannel = _server->get_Channel(params[0]);
		std::string mode = params[1];
		//std::string test = "+abc";
		std::string name;
		size_t ClientModeCount = 0;
		std::string validClientPattern = R"([+-][o]+)"; // once this is done it should be easy to add more
		
		//size_t paramSize = params.size();
		//std::cout<<"TELL ME THE SIZE OF PARAMSLIST BEFORE DOING ANYTHING TO IT = "<<paramSize<<"\n";

		 if (params.size() == 1) {
        	std::string channel_name = params[0];
        	std::shared_ptr<Channel> channel = _server->get_Channel(channel_name); // Assuming _server->get_Channel is valid

 	       if (!channel) {
    	        // Channel does not exist
        	    //client->getMsg().queueMessage(MessageBuilder::buildNumericReply(_serverName, "403", client->getNickname(), channel_name, "No such channel"));
            	return;
		   }
		//	if (channel->getTopic().empty()) { // Assuming Channel has getTopic()
	    //        client->getMsg().queueMessage(MessageBuilder::buildNumericReply(_serverName, "331", client->getNickname(), channel_name, "No topic is set"));
	    //    } else {
	    //        client->getMsg().queueMessage(MessageBuilder::buildNumericReply(_serverName, "332", client->getNickname(), channel_name, ":" + channel->getTopic()));
	    //    }
	    //    return; // Important: Exit after handling the query!
			std::string modes =  ":localhost 324 " + client->getNickname() + " #bbq +nt\r\n";
			std::string checktopic = channel->getTopic();
			std::cout<<"checking the topic is set to what "<<checktopic<<" \n";
		   	std::string topic = ":localhost 332 " + client->getNickname() + " #bbq :Welcome to #bbq!\r\n";
			
		   	client->getMsg().queueMessage(modes);
		   	client->getMsg().queueMessage(topic);
			_server->updateEpollEvents(client->getFd(), EPOLLOUT,true);
		}

		/*if ((ClientModeCount = client->getMsg().countOccurrences(mode, "o")) != paramSize - 2) // bbq and the flags
		{
			std::cout<<"TELL ME THE SIZE OF PARAMSLIST = "<<paramSize - 2<<"\n";
			std::cout<<"TELL ME THE CLIENTMODECOUNT = "<<ClientModeCount<<"\n";
			std::cout<<" WE do not have an equal number of client modes to params \n";
			return;
		}*/
		std::string validPattern = R"([+-][ioklt]+)";
		if ((mode[0] == '-' || mode[0] == '+') && std::regex_match(mode, std::regex(validPattern))) // regex for specifci letters only ? 
		{
			// parsing to check that flags are valid +o +oop 
			size_t i = 1;
			size_t ret = 0;
			size_t ret2 = 0;
			//size_t clientsHandled = 0;
			size_t total = 2;
			std::string names;
			std::string pass = "";
			while (i < mode.length())
			{	
				//asuming regex confimed everything 
				std::string newmode = std::string(1, mode[0]) + mode[i]; //mode[0] + mode[i + 1 ];
				//std::cout<<"getting mode "<<newmode<<"\n";
				//std::cout<<"getting total now "<<total<<"\n";
				//std::cout<<"getting Clientmodecount "<<ClientModeCount<<"\n";
//wrong total too big
				//if (ClientModeCount == 0)
				//{
					//std::cout<<"empty name being added \n";
				//	name = "";
				//}
				// check to see if we are dealing with a client mode or not 
				if (ClientModeCount > 0 && std::regex_match(newmode, std::regex(validClientPattern)))
				{
					name = params[total];
					if (name == "")
						std::cout<<"getParam went out of bounds here !!!!!!! fix it \n";
					try
					{
						ret = currentChannel->setClientMode(newmode, name, client->getNickname());
						
					}
					catch(const std::exception& e)
					{
						std::cerr << e.what() << '\n';
					}					
					if (ret == 2)
					{
						//setMsgtype
						client->getMsg().queueMessage(":localhost 482 " + client->getNickname() + " " + params[0] + " :"+ client->getNickname() +", You're not channel operator\r\n");
						break;// :Permission denied—operator privileges required
					}
					total += ret;
					if (ret == 1) // client mode done
					{
						names += name + " ";
						ClientModeCount =- 1;
						//std::cout<<"ret was 1 name being added to list = "<<names<<"\n";
					}
					//std::cout<<"this name now being added = "<<name<<"\n";

				}
				//if (client->getMsg().getParams().size())
				else
				{
					// if mode == o take a parameter , otherwise its a channel mode
					try
					{
						// this could be even further out perhaps? 
						//ret = currentChannel->setClientMode(newmode, name, _nickName);
						pass = params[total];
						if (pass == "")
							std::cout<<"getParam went out of bounds here !!!!!!! fix it although in this instance we handle it inside setchannelmode \n";
						//onst std::string pass, std::map<std::string, int> listOfClients,  FuncType::setMsgRef setMsgType
						ret2 = currentChannel->setChannelMode(newmode, name, client->getNickname(), params[0], pass, _server->get_nickname_to_fd(), [&](MsgType msg, std::vector<std::string>& params) {
																															client->getMsg().setType(msg, params);
																															});
						//std::cout<<"ret is now = "<<ret <<"\n";
						if (ret2 == 2)
						{
							//setMsgtype
							client->getMsg().queueMessage(":localhost 482 " + client->getNickname() + " " + params[0] + " :"+ client->getNickname() +", You're not channel operator\r\n");
							break;// :Permission denied—operator privileges required
						}
					}
					catch(const std::exception& e)
					{
						//std::cout<<"exception caught brekaing \n";

						std::cerr << e.what() << '\n';
						break;
					}
				}
				i++;
			
			}
			std::cout<<"list of names = "<<names<<"\n";
			
			if (ret != 2 || ret2 != 2)
			{
				_server->getChannelsToNotify().push_back(_server->get_Channel(params[0]));
				 std::deque<std::shared_ptr<Channel>> channels_to_process = _server->getChannelsToNotify();
			    // 2. Iterate directly over the shared_ptr<Channel> objects in the deque
			    for (const auto& channel_ptr : channels_to_process) {
			        // Always check if the shared_ptr is valid (not null)
			        if (channel_ptr) {
			            std::cout << "Processing channel: " << channel_ptr->getName() << std::endl;
			            // Now you can perform operations on 'channel_ptr'
			            // For example, if this were handleQuit context:
			            // channel_ptr->removeClient(quitting_client_nickname);
			             _server->broadcastMessageToChannel(channel_ptr, ":" + client->getNickname() +"!ident@localhost" +" MODE " + params[0] + " " + params[1] + " " + names + "\r\n", client);
			        } else {
			            std::cout << "Skipping null channel pointer in deque." << std::endl;
			    	}
				}
			//	_server->getChannelBroadcastQue().push_back(":" + client->getNickname() +"!ident@localhost" +" MODE " + params[0] + " " + params[1] + " " + names + "\r\n");
			}
				
			//_server.getChannelBroadcastQue()->
			//setmsg channel broadcast this client + modes + string of names 
			// setdefinedmsg
			// do messages need to be built here 

		}
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
				if (_server->get_Client(fd)->isMsgEmpty()) {
					_server->updateEpollEvents(fd, EPOLLOUT, true);
				}
				_server->get_Client(fd)->getMsg().queueMessage( ":" + client->getNickname() + " PRIVMSG "  + params[0]  + " " + params[1] + "\r\n");
			}
		}
		if (!params[0].empty())
		{
			// handle error or does irssi handle
		}
		
	}
	if (command == "WHOIS")
	{
		/*If target_nick is generatedname (your client's actual nickname):
		Send :<your_server_name> 311 generatedname generatedname ~user host * :realname\r\n
		Send :<your_server_name> 318 generatedname generatedname :End of WHOIS list\r\n*/
		std::cout<<"show me the param = "<<params[0]<<" and the nick ="<<client->getNickname()<<"\n";
		if (params[0] != client->getNickname()) // or name no exist for some reason 
		{
			client->getMsg().queueMessage(":localhost 401 " + client->getNickname() + " " + params[0] + " :NO suck nick\r\n");
			_server->updateEpollEvents(client->getFd(), EPOLLOUT, true);
		}
//		Send :<your_server_name> 401 generatedname configname :No such nick/channel\r\n (using generatedname as the source of the error, as that's the client's real nick).
	}

	//client->getMsg().printMessage(client->getMsg());
}