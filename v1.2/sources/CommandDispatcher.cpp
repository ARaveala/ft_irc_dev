#include <CommandDispatcher.hpp>
#include <iostream>
#include "Server.hpp"
#include "Client.hpp"

#include <cctype>
#include <regex>
#include "IrcResources.hpp"
void CommandDispatcher::handle_message(const std::string message, Server& server, int fd)
{
	std::shared_ptr<Client> client = server.get_Client(fd);
	if (message.empty()) {
        std::cerr << "Error: Received an empty message in handle_message()" << std::endl;
        return;
    }

    if (!client) {
        std::cerr << "Error: Client pointer is null in handle_message()" << std::endl;
        return;
    }
	client->getMsg().parse(message);
	//std::string param = getParam(0);
	if (client->getMsg().getCommand() == "QUIT")
	{
		// quite will set up messages clean up from channel (should be changed?) and quit bool set to true.
		// quit is managed in broadcast channel message for now, due to needing the client fd for epollout
		// allternatives are make a ghost fd, or find next available client fd. 
		std::cout<<"QUIT CAUGHT IN command list handlking here --------------\n";
		client->getMsg().setType(MsgType::CLIENT_QUIT, {client->getNickname()});
		client->getMsg().callDefinedMsg();
		// this is one fucntion call and can be mapped later 
		if (client->prepareQuit(server.getChannelsToNotify()) == 1)
		{
			std::cout<<"preparing the quit message for broadcast\n";
			client->getMsg().callDefinedBroadcastMsg(server.getChannelBroadcastQue());
		}
		client->setQuit();
		
		//client->getMsg().removeQueueMessage();
		std::cout<<"client removed at quit command \n";

		return ;
	}
	if (client->getMsg().getCommand() == "NICK") {
		client->getMsg().prep_nickname(client->getNicknameRef(), client->getFd(), server.get_fd_to_nickname(), server.get_nickname_to_fd()); // 
		client->getMsg().callDefinedMsg();//(client->getMsg().getActiveMsgType());
		return ; 
	}
	if (client->getMsg().getCommand() == "PING"){
		client->sendPong();
		std::cout<<"sending pong back "<<std::endl;
		//Client->set_failed_response_counter(-1);
		//resetClientTimer(Client->get_timer_fd(), config::TIMEOUT_CLIENT);
	}
	if (client->getMsg().getCommand() == "PONG"){
		std::cout<<"------------------- we recived pong inside message handling haloooooooooo"<<std::endl;
	}

    if (client->getMsg().getCommand() == "JOIN" && !client->getMsg().getParam(0).empty()){
		server.handleJoinChannel(client, client->getMsg().getParam(0), client->getMsg().getParam(1));
		
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
	/**
	 * @brief 
	 * 
	 * sudo tcpdump -A -i any port <port number>, this will show what irssi sends before being reecived on
	 * server, irc withe libera chat handles modes like +io +o user1 user2 but , i cant find a way to do that because
	 * irssi is sending the flags and users combined together into 1 string, where does flags end and user start? 
	 * 
	 * so no option but to not allow that format !
	 * 
	 */
	if (client->getMsg().getCommand() == "MODE")
	{
		std::shared_ptr <Channel> currentChannel = server.get_Channel(client->getMsg().getParam(0));
		std::string mode = client->getMsg().getParam(1);
		//std::string test = "+abc";
		std::string name;
		size_t ClientModeCount = 0;
		std::string validClientPattern = R"([+-][o]+)"; // once this is done it should be easy to add more
		
		size_t paramSize = client->getMsg().getParams().size();
		std::cout<<"TELL ME THE SIZE OF PARAMSLIST BEFORE DOING ANYTHING TO IT = "<<paramSize<<"\n";

		if ((ClientModeCount = client->getMsg().countOccurrences(mode, "o")) != paramSize - 2) // bbq and the flags
		{
			std::cout<<"TELL ME THE SIZE OF PARAMSLIST = "<<paramSize - 2<<"\n";
			std::cout<<"TELL ME THE CLIENTMODECOUNT = "<<ClientModeCount<<"\n";
			std::cout<<" WE do not have an equal number of client modes to params \n";
			//return;
		}
		std::string validPattern = R"([+-][ioklt]+)";
		if ((mode[0] == '-' || mode[0] == '+') && std::regex_match(mode, std::regex(validPattern)))//isalpha(mode[1])) // regex for specifci letters only ? 
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
					name = client->getMsg().getParam(total);
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
						client->getMsg().queueMessage(":localhost 482 " + client->getNickname() + " " + client->getMsg().getParam(0) + " :"+ client->getNickname() +", You're not channel operator\r\n");
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
						pass = client->getMsg().getParam(total);
						if (pass == "")
							std::cout<<"getParam went out of bounds here !!!!!!! fix it although in this instance we handle it inside setchannelmode \n";
						//onst std::string pass, std::map<std::string, int> listOfClients,  FuncType::setMsgRef setMsgType
						ret2 = currentChannel->setChannelMode(newmode, name, client->getNickname(), client->getMsg().getParam(0), pass, server.get_nickname_to_fd(), [&](MsgType msg, std::vector<std::string>& params) {
																															client->getMsg().setType(msg, params);
																															});
						//std::cout<<"ret is now = "<<ret <<"\n";
						if (ret2 == 2)
						{
							//setMsgtype
							client->getMsg().queueMessage(":localhost 482 " + client->getNickname() + " " + client->getMsg().getParam(0) + " :"+ client->getNickname() +", You're not channel operator\r\n");
							break;// :Permission denied—operator privileges required
						}
						//total += ret;
						//std::cout<<"total is now = "<<total <<"\n";

						//if (ret == 1) // client mode done
						//{
						//	names += name + " ";
						//	ClientModeCount =- 1;
							//std::cout<<"ret was 1 name being added to list = "<<names<<"\n";
						//}
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
				server.getChannelsToNotify().push_back(client->getMsg().getParam(0));
				server.getChannelBroadcastQue().push_back(":" + client->getNickname() +"!ident@localhost" +" MODE " + client->getMsg().getParam(0) + " " + client->getMsg().getParam(1) + " " + names + "\r\n");
			}
				
			//server.getChannelBroadcastQue()->
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
		if (!client->getMsg().getParam(0).empty())
		{
			if (client->getMsg().getParam(0)[0] == '#')
			{
				std::cout<<"stepping into priv message handling \n";
				std::string buildMessage;
				// right now we loop through all params, do we need to check for symbols ? like# or modes
				// we need to adjust the space handling in the message class as righ tnow we loose all spaces 
				for (size_t i = 1; i < client->getMsg().getParams().size(); i++)
				{
					buildMessage += client->getMsg().getParam(i) ;
				}
				// eed to check does channel exist
				if (server.channelExists(client->getMsg().getParam(0)) == true)
				{
					server.getChannelsToNotify().push_back(client->getMsg().getParam(0));
					server.getChannelBroadcastQue().push_back(":" + client->getNickname() + " PRIVMSG " + client->getMsg().getParam(0) + " " + buildMessage + "\r\n");

				}
				// check is it a channel name , starts with #, collect to serverchannelbroadcast
				// is it just a nickname , collect to messageQue, send to only that fd of
	
			}
			else
			{
				int fd = server.get_nickname_to_fd().find(client->getMsg().getParam(0))->second;
				if (fd < 0)
				{
					// no user found by name
					// no username provided
					std::cout<<"no user here by that name \n"; 
					return ;
				}
				server.set_private_fd(fd);
				client->getMsg().queueMessage( ":" + client->getNickname() + " PRIVMSG "  + client->getMsg().getParam(0)  + " " + client->getMsg().getParam(1) + "\r\n");

			}
		}
		if (!client->getMsg().getParam(0).empty())
		{
			// handle error or does irssi handle
		}
		
	}	

	client->getMsg().printMessage(client->getMsg());
}