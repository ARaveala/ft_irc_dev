#include "Server.hpp"
#include <sys/types.h>
#include <unistd.h>
#include <string.h> //strlen
#include <iostream> // testing with cout
#include <sys/socket.h>
#include <sys/epoll.h>
#include "epoll_utils.hpp"
#include "Client.hpp"
#include <map>
#include <memory> // shared pointers
#include "config.h"
#include "ServerError.hpp"
#include "Channel.hpp"
//#include "SendException.hpp"
#include <algorithm> // find_if
//#include <optional> // nullopt , signifies absence
class ServerException;
#include <unordered_set>
#include "MessageBuilder.hpp"
Server::Server(){
	std::cout << "#### Server instance created." << std::endl;
}

/**
 * @brief Construct a new Server:: Server object
 * 
 * @param port 
 * @param password 
 */
Server::Server(int port , std::string password) {
	_port = port;
	_password = password;
	_commandDispatcher = std::make_unique<CommandDispatcher>(this);
	
}

// ~~~SETTERS
void Server::setFd(int fd){
	_fd = fd;}

void Server::set_signal_fd(int fd){
	_signal_fd = fd;
}

// note we may want to check here for values below 0
void Server::set_event_pollfd(int epollfd){
	_epoll_fd = epollfd;
}

void Server::set_client_count(int val){
	_client_count += val;
}

void Server::set_current_client_in_progress(int fd){
	_current_client_in_progress = fd;
}

// ~~~GETTERS
int Server::getFd() const {
	 return _fd;
}

int Server::get_signal_fd() const{
	return _signal_fd;
}

int Server::get_client_count() const{
	return _client_count;
}

int Server::getPort() const{
	return _port;
}

int Server::get_event_pollfd() const{
	return _epoll_fd;
}

int Server::get_current_client_in_progress() const{
	return _current_client_in_progress;
}

/**
 * @brief Here a client is accepted , error checked , socket is adusted for non-blocking
 * the client fd is added to the epoll and then added to the Client map. a welcome message
 * is sent as an acknowlegement message back to irssi.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

void Server::create_Client(int epollfd) {
 	// Handle new incoming connection
	int client_fd = accept(getFd(), nullptr, nullptr);
 	if (client_fd < 0) {
		throw ServerException(ErrorType::ACCEPT_FAILURE, "debuggin: create Client");
	} else {
		//int flag = 1;
		//int buf_size = 1024;

		//setsockopt(client_fd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size));
		//setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, (void*)&flag, sizeof(flag));	
		make_socket_unblocking(client_fd);
		//setsockopt(client_fd, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag)); 
		setup_epoll(epollfd, client_fd, EPOLLIN | EPOLLOUT | EPOLLET); //not a fan of epollet
		int timer_fd = setup_epoll_timer(epollfd, config::TIMEOUT_CLIENT);
		// errro handling if timer_fd failed
		// create an instance of new Client and add to server map
		_Clients[client_fd] = std::make_shared<Client>(client_fd, timer_fd);
		_timer_map[timer_fd] = client_fd;
		//std::shared_ptr<Client> = _Clients[client_fd];

		std::cout << "New Client created , fd value is  == " << _Clients[client_fd]->getFd() << std::endl;
		
		set_current_client_in_progress(client_fd);
		_Clients[client_fd]->setDefaults();
		_fd_to_nickname.insert({_Clients[client_fd]->getFd(), _Clients[client_fd]->getNickname()});
		_nickname_to_fd.insert({_Clients[client_fd]->getNickname(), _Clients[client_fd]->getFd()});
		if (!_Clients[client_fd]->get_acknowledged()){
			//_Clients[client_fd]->set_pendingAcknowledged(true);
			//_Clients[client_fd]->getMsg().prepWelcomeMessage(_Clients[client_fd]->getNicknameRef());//, _Clients[client_fd]->getMsg().getQue());
			//std::string msg = MessageBuilder::generatewelcome(_Clients[client_fd]->getNickname());
			//std::string name_change = ":server NOTICE #testchannel :Nickname update detected\r\n";//":" + _Clients[client_fd]->getNickname() + " NICK " + _Clients[client_fd]->getNickname() + "\r\n";
			//updateEpollEvents(client_fd, EPOLLOUT, true);
			
			_Clients[client_fd]->getMsg().queueMessage(":localhost NOTICE * :initilization has begun.......\r\n");
			_Clients[client_fd]->getMsg().queueMessage(":localhost CAP * LS :multi-prefix sasl\r\n");

			//_Clients[client_fd]->getMsg().queueMessage(":localhost CAP * END\r\n");
			//_Clients[client_fd]->getMsg().queueMessage("NICK startingNick\r\n");
			//_Clients[client_fd]->getMsg().queueMessage("USER "  + _Clients[client_fd]->getClientUname() + " 0 * :Real Name\r\n");
			
			//_Clients[client_fd]->getMsg().queueMessage(msg);
			//_Clients[client_fd]->getMsg().queueMessage(name_change);
			set_client_count(1);		
		}
	}
}

/**
 * @brief removing a singular client
 * 
 * 
 * @param epollfd 
 * @param client_fd 
 * 
 * @note we are calling get client multple times here , we could try to work past that 
 * by just sending in the client once as a param
 */
void Server::remove_Client(int client_fd) {
	auto client_to_remove = get_Client(client_fd);
	if (!client_to_remove) {
        std::cerr << "ERROR: remove_Client called for non-existent client FD: " << client_fd << std::endl;
        return;
    }
	_nickname_to_fd.erase(client_to_remove->getNickname());
	_fd_to_nickname.erase(client_fd);	

	epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_to_remove->get_timer_fd(), 0);
	close(client_to_remove->get_timer_fd());
	
	
	epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, 0);
	close(client_fd);
	
	// channel.getClient().erase();
	_Clients.erase(client_fd);
	_epollEventMap.erase(client_fd);
	

	//_epollEventMap.erase(client_fd);
	//std::map<int, struct epoll_event> _epollEventMap;
	_client_count--;
	std::cout<<"client has been removed"<<std::endl;
}


/**
 * @brief to find the Client object in the Clients array
 *  and return a pointer to it
 *
 * @param fd the active fd
 * @return Client* , shared pointers are a refrence themselves
 */
std::shared_ptr<Client> Server::get_Client(int fd) {
	for (std::map<int, std::shared_ptr<Client>>::iterator it = _Clients.begin(); it != _Clients.end(); it++) {
		if (it->first == fd)
			return it->second;
	}
	throw ServerException(ErrorType::NO_Client_INMAP, "can not get_Client()");
}


std::map<int, std::shared_ptr<Client>>& Server::get_map() {
	return _Clients;
}

std::map<int, std::string>& Server::get_fd_to_nickname() {
	return _fd_to_nickname;
}

std::map<std::string, int>& Server::get_nickname_to_fd() {
	return _nickname_to_fd;
}

std::string Server::get_password() const {
	return _password;
}

// ERROR HANDLING INSIDE LOOP
void Server::handle_client_connection_error(ErrorType err_type) {
	switch (err_type){
		case ErrorType::ACCEPT_FAILURE:
			break;
		case ErrorType::EPOLL_FAILURE_1: {
			send(_current_client_in_progress, IRCMessage::error_msg, strlen(IRCMessage::error_msg), 0);
			close(_current_client_in_progress);
			_current_client_in_progress = 0;
			break;
		} case ErrorType::SOCKET_FAILURE: {
			send(_current_client_in_progress, IRCMessage::error_msg, strlen(IRCMessage::error_msg), 0);
			close(_current_client_in_progress);
			_current_client_in_progress = 0;
			break;
		} default:
			send(_current_client_in_progress, IRCMessage::error_msg, strlen(IRCMessage::error_msg), 0);
			close(_current_client_in_progress);
			_current_client_in_progress = 0;
			std::cerr << "server Unknown error occurred" << std::endl;
			break;
	}
}

void Server::handleReadEvent(int client_fd) {
    std::shared_ptr<Client> client = get_Client(client_fd);
    if (!client) {
        // This client might have been disconnected by another event (e.g., EPOLLHUP)
        // or during processing of previous messages in this same loop iteration.
        // Just return as there's nothing to read for it.
        return;
    }

    char buffer[config::BUFFER_SIZE];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0); // Server does the recv, ONCE per event!
	std::cout << "Debug - Raw Buffer Data: [" << std::string(buffer, bytes_read) << "]" << std::endl;
    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No more data to read right now (non-blocking socket behavior)
            // This is NORMAL, just means we've read everything that was available.
            return;
        }
        perror("recv failed in handleReadEvent"); // error
		remove_Client(client_fd);
		return;
    }
    if (bytes_read == 0) {
        std::cout << "Client FD " << client_fd << " disconnected gracefully (recv returned 0).\n";
        // consider sending a QUIT message here, if your protocol requires it.
        // e.g. _commandDispatcher->handleQuitCommand(client, "Client disconnected");
		remove_Client(client_fd);
        return;
    }
    
    // If bytes_read > 0:
    // Give the raw received data to the client's message buffer
    client->appendIncomingData(buffer, bytes_read);
	  // This loop ensures all complete messages are processed.
    while (client->extractAndParseNextCommand()) {
        // A complete IRC command has been successfully extracted and parsed by the client's IrcMessage instance.
        // It has also been REMOVED from the client's internal buffer (`_ircMessage._read_buffer`).

        // This is the point where you know the client is active and sent a valid message.
        resetClientTimer(client_fd, config::TIMEOUT_CLIENT);
        client->set_failed_response_counter(-1);

        // --- DELEGATE TO COMMANDDISPATCHER ---
		//client->getMsg().printMessage(client->getMsg());
        _commandDispatcher->dispatchCommand(client, client->getMsg().getParams());
    }
}

void Server::shutdown() {
	// close all sockets
	for (std::map<int, std::shared_ptr<Client>>::iterator it = _Clients.begin(); it != _Clients.end(); it++){
		close(it->first);
	}
	for (std::map<const std::string, std::shared_ptr<Channel>>::iterator it = _channels.begin(); it != _channels.end(); it++){
		it->second->clearAllChannel();
		//close(it->first);
	}
	for (std::map<int, int>::iterator timerit = _timer_map.begin(); timerit != _timer_map.end(); timerit++)
	{
		close(timerit->first);
	}
	// close server socket
	close(_fd);
	close(_signal_fd);
	close(_epoll_fd);
	for (std::map<int, std::shared_ptr<Client>>::iterator it = _Clients.begin(); it != _Clients.end(); it++){
		it->second.reset();
		it->second->getMsg().clearQue();
		it->second->getMsg().clearAllMsg();
	}

	_timer_map.clear();
	_Clients.clear();
	// delete channels
	_epollEventMap.clear();
	

	_server_broadcasts.clear();
	//_illegal_nicknames.clear();

	std::cout<<"server shutdown completed"<<std::endl;
}

Server::~Server(){
	//shutdown();
}

/**
 * @brief checks if a timer fd was activated in epoll
 * updates failed response counter and sends a ping to 
 * check if there is an active client, if not , the client is removed.  
 * 
 * @param fd 
 * @return true if timer fd was not an active event 
 * @return false if timer fd was an active event
 */
bool Server::checkTimers(int fd) {
	auto timerit = _timer_map.find(fd);
	if (timerit == _timer_map.end()) {
		return true;
	}
	int client_fd = timerit->second;
    auto clientit = _Clients.find(client_fd);
    if (clientit == _Clients.end()) {
		return false;
	} // did not find client on the list eek
    if (clientit->second->get_failed_response_counter() == 3) {
		std::cout<<"timer sup removing client \n";
		remove_Client(client_fd);
        _timer_map.erase(fd);
        return false;
    }
    std::cout << "should be sending ping onwards " << std::endl;
    clientit->second->sendPing();
    clientit->second->set_failed_response_counter(1);
    resetClientTimer(timerit->first, config::TIMEOUT_CLIENT);
	return false;
}

/*void Server::setGhostFd()
{
	struct epoll_event event;
	event.events = EPOLLOUT | EPOLLIN  | EPOLLET;
	event.data.fd = fd;
	epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &event);
}
void Server::removeGhostFd()
{
	close(fd);
	epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, 0);
}
*/
void Server::setEpollout(int fd)
{
	struct epoll_event event;
	event.events = EPOLLOUT | EPOLLIN | EPOLLET;
	event.data.fd = fd;
	epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &event);
}
#include <iomanip>
void Server::send_message(std::shared_ptr<Client> client)
{

	int fd;
	fd = client->getFd();
	
	while (client->isMsgEmpty() == false && client->getMsg().getQueueMessage() != "") {

		std::string msg = client->getMsg().getQueueMessage();

		size_t remaining_bytes_to_send = client->getMsg().getRemainingBytesInCurrentMessage();
        const char* send_buffer_ptr = client->getMsg().getCurrentMessageCstrOffset();
		
		std::cout<<"checking the message from que before send ["<< msg <<"] and the fd = "<<fd<<"\n";
					// Right before your queueMessage call:
			std::cout << "DEBUG: params[0] content (raw): [" << msg << "]" << std::endl;
			// Or to show non-printable characters:
			std::cout << "DEBUG: params[0] content (hex): [";
			for (char c : msg) {
			    if (c == '\r') std::cout << "\\r";
			    else if (c == '\n') std::cout << "\\n";
			    else if (c >= ' ' && c <= '~') std::cout << c; // Printable ASCII
			    else std::cout << "\\x" << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)c << std::dec;
			}
			std::cout << "]" << std::endl;
		//while (msg.length() > 0) { // Keep trying to send THIS message until it's gone
        //    ssize_t bytes_to_send = msg.length();
            //ssize_t bytes_sent = send(fd, msg.c_str(), bytes_to_send, MSG_NOSIGNAL);
            ssize_t bytes_sent = send(fd, send_buffer_ptr, remaining_bytes_to_send, MSG_NOSIGNAL);
			//usleep(5000); 
            if (bytes_sent == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Socket buffer is full. Cannot send more right now.
                    //std::cout << "DEBUG: send() returned EAGAIN/EWOULDBLOCK for FD " << fd << ". Pausing write.\n";
                    return; // <<< EXIT send_message. Epoll will re-trigger.
                } else {
                    // Fatal error (e.g., connection reset, broken pipe).
                    perror("send error");
                    //std::cerr << "ERROR: Fatal send error for FD " << fd << ". Disconnecting client.\n";
                    // !!!Call remove client function here !!!
                    return;
                }
            } else{
				client->getMsg().advanceCurrentMessageOffset(bytes_sent); 
				//client->getMsg().removeQueueMessage(); // Remove the original (full) message
			 	if (client->getMsg().getRemainingBytesInCurrentMessage() == 0) {
                // The entire current message has been sent.
                client->getMsg().removeQueueMessage(); // Remove from queue, reset offset for next message.
                std::cout << "DEBUG: Full message sent for FD " << fd << ". Moving to next message.\n";
                // The outer 'while' loop will now check for the next message in the queue.
	            } else {
	                // Partial send of the current message.
	                // Still more bytes to send for *this* message.
	                // Leave EPOLLOUT enabled. The next time send_message is called,
	                // it will automatically pick up from the new offset.
	                std::cout << "DEBUG: Partial send for FD " << fd << ". Sent " << bytes_sent
	                          << " bytes. Remaining: " << client->getMsg().getRemainingBytesInCurrentMessage() << ".\n";
	                return; // Exit send_message. Epoll will re-trigger for remaining bytes.
				}
				/*if (static_cast<ssize_t>(bytes_sent) < bytes_to_send) {
                // Partial send of the current message.
			    	client->getMsg().queueMessageFront(msg.substr(bytes_sent));
                	//std::cout << "DEBUG: Partial send for FD " << fd << ". Sent " << bytes_sent
                    //      << " bytes. Remaining: " << msg.length() << " in current message.\n";
           		 } else { // (static_cast<size_t>(bytes_sent) == bytes_to_send)
                // Entire current message was sent.
                	//std::cout << "DEBUG: Full part of current message sent for FD " << fd << ".\n";
                	break; // Exit the inner while loop to remove this message and try the next.
            	}*/
			}
        //} 
	}
	if (client->isMsgEmpty()) {
		if (!client->get_acknowledged())
		{
			//ignore this for now
			std::cout<<"SENDING WELCOME MESSAGE INCOMING 11111111111111111111\n";
			client->set_acknowledged();		
		}	
		updateEpollEvents(fd, EPOLLOUT, false);
	}

}



void Server::send_server_broadcast()
{
	while(!_server_broadcasts.empty())
	{
		std::string msg = _server_broadcasts.front();

		for (auto& pair : _Clients ) {
			ssize_t bytes_sent = send(pair.first, msg.c_str(), msg.length(), 0);
			if (bytes_sent == -1) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					std::cout<<"triggering the conts???????????????????????????????????????????\n";
					continue;  //No more space, stop writing
				}
				else perror("send error");
			}
			if (bytes_sent > 0) {
			
			}
		}
		_server_broadcasts.pop_front();  //remove sent message
	}
	_server_broadcasts.clear();
}


//channel realted 

bool Server::channelExists(const std::string& channelName) const {
    return _channels.count(channelName) > 0;
}
//
void Server::createChannel(const std::string& channelName) {
    if (_channels.count(channelName) == 0){
		_channels.emplace(channelName, std::make_shared<Channel>(channelName));
        std::cout << "Channel '" << channelName << "' created." << std::endl;
		// fill clientsend buffer 
		return ;
    }
    std::cerr << "Error: Channel '" << channelName << "' already exists" << std::endl;
}

std::shared_ptr<Channel> Server::get_Channel(std::string ChannelName) {
	for (std::map<std::string, std::shared_ptr<Channel>>::iterator it = _channels.begin(); it != _channels.end(); it++) {
		if (it->first == ChannelName)
			return it->second;
	}
	throw ServerException(ErrorType::NO_Client_INMAP, "can not get_Client()");
}

void Server::handleJoinChannel(std::shared_ptr<Client> client, const std::string& channelName, const std::string& password)
{
	std::cout<<"attempting tooooooo!--------------------------------------";
	// check there is a param 0
	if (channelName == "")
	{
		std::cout<<"no channel name provided handle error\n";
		//return ;
	}
	if (channelExists(channelName) < 1) //bool
	{
		std::cout<<"creating channel now!--------------------------------------";
		// create a channel object into a server map
		createChannel(channelName);
		// add channel to client list		
		client->setChannelCreator(true);
		// set defaults what are they 
	}
	std::shared_ptr <Channel> currentChannel = get_Channel(channelName);
	//std::string pass = "";
	//if (client->getMsg().getParams().size() == 2) // biger than 2 ?
	//pass = client->getMsg().getParam(1); 
	if (!currentChannel->canClientJoin(client->getNickname(), password))
		return ;
		// set msg
		//return
	client->addChannel(channelName, get_Channel(channelName));
	currentChannel->addClient(client);
	std::string ClientList = currentChannel->getAllNicknames();
	if (ClientList.empty())
		std::cout<<"WE HAVE A WIERDS PROBLEM AND CLIENT LIST IS NULL FOR JOIN\n";
	
	if (client->isMsgEmpty() == true)
		updateEpollEvents(client->getFd(), EPOLLOUT, true);
	client->getMsg().prep_join_channel(channelName, client->getNickname(),  client->getMsg().getQue(),ClientList);
}
void Server::handleNickCommand(std::shared_ptr<Client> client) {
	std::string msg = MessageBuilder::generateMessage(client->getMsg().getActiveMessageType(),  client->getMsg().getMsgParams());
 
	broadcastMessageToClients(client, msg, false);
	 std::cout << "DEBUG NICK Self-Message: Processing NICK for Client " << client->getNickname() 
              << " (FD: " << client->getFd() << ")\n";
    std::cout << "DEBUG NICK Self-Message: Before queuing, client->isMsgEmpty() is " 
              << (client->isMsgEmpty() ? "true" : "false") << " for FD " << client->getFd() << "\n";

	if (client->isMsgEmpty() == true)
	{
		 std::cout << "DEBUG NICK Self-Message: Queue WAS empty for Client " << client->getNickname() 
                  << ", enabling EPOLLOUT for FD " << client->getFd() << ".\n";
		updateEpollEvents(client->getFd(), EPOLLOUT, true);

	}
	else {
        std::cout << "DEBUG NICK Self-Message: Queue was NOT empty for Client " << client->getNickname() 
                  << ", EPOLLOUT NOT enabled (already enabled or other messages pending) for FD " << client->getFd() << ".\n";
    }
	client->getMsg().queueMessage(msg);
	//client->getMsg().queueMessage(msg);
	//"WHO newnick\r\n";
	//":newnick JOIN #testchannel\r\n";
	//":oldnick NICK newnick\r\n"
	//":server NOTICE #testchannel :Nickname update detected\r\n"
	//client->getMsg().queueMessage(":newnick JOIN #testchannel\r\n");

	std::cout << "DEBUG NICK Self-Message: Message QUEUED for Client " << client->getNickname() 
              << " (FD: " << client->getFd() << "). Current queue size after: " << client->getMsg().getQue().size() << "\n";
}

void Server::handleQuit(std::shared_ptr<Client> client) {
	//std::string msg =MessageBuilder::buildClientQuit( client->getNickname() + "!username@localhost", "Client disconnected");
	//broadcastMessageToClients(client, msg, false);
	if (!client) {
        std::cerr << "Server::handleQuit: Received null client pointer.\n";
        return;
    }

    std::cout << "SERVER: Handling QUIT for client " << client->getNickname() << " (FD: " << client->getFd() << ")\n";
	
    // Step 1: Prepare the list of channels the client was in.
    // We get the map of weak_ptrs from the client.
    // Create a temporary vector of channel names to iterate over safely.
    // This avoids iterator invalidation if _joinedChannels were modified during the loop.
    std::vector<std::string> channels_to_process;
    for (const auto& pair : client->getJoinedChannels()) {
        if (pair.second.lock()) { // Ensure the weak_ptr is still valid
            channels_to_process.push_back(pair.first); // pair.first is the channel name (std::string)
        }
    }

    // Step 2: Loop through each channel the client was in (SINGLE loop).
    // For each channel, remove the client and broadcast the PART message.
    for (const std::string& channel_name : channels_to_process) {
        // Use getChannel to get the shared_ptr<Channel> from the server's master list
        auto channel_ptr = get_Channel(channel_name); 

        if (channel_ptr) { // Always check if the channel still exists on the server
            std::cout << "SERVER: Removing client " << client->getNickname() << " from channel " << channel_name << "\n";

            // A) Channel removes the client from its internal list.
            channel_ptr->removeClient(client->getNickname());

            // B) Build the PART message for *this specific channel context*.
           	client->getMsg().setType(MsgType::CLIENT_QUIT, {client->getNickname(), client->getClientUname()});
			std::string part_message =  MessageBuilder::generateMessage(client->getMsg().getActiveMessageType(), client->getMsg().getMsgParams());
		   /*std::string part_message = MessageBuilder::buildClientQuit(
                //client.getPrefix(),  consist of  nickname!username@hostname // Use the quitting client's full prefix for the message source
                client->getNickname() + "!username@localhost",
				//channel_name,
				"Client disconnected"
                //quit_message_reason
            );*/

            // C) Broadcast this PART message to the *remaining* members of THIS channel.
            //    The broadcastMessageToChannel helper (which internally uses sendMessageToClient)
            //    handles iterating the channel's members and queuing the message for each.
            broadcastMessageToChannel(channel_ptr, part_message, client); // 'client' is the sender to be excluded from broadcast
        } else {
            std::cerr << "WARNING: Client " << client->getNickname() << " was in channel "
                      << channel_name << " but channel no longer exists on server.\n";
        }
    }

    // Step 3: Clean up the quitting client's own internal state.
    // This happens AFTER all channel-specific processing and broadcasts.
    client->clearJoinedChannels();

    // Step 4: Mark the client for final server-level disconnection.
    client->setQuit();

    // Step 5: Send a final server-generated message *only to the quitting client*.
    //sendMessageToClient(client, MessageBuilder::buildClosingLink(client->getNickname(), quit_message_reason));

    std::cout << "SERVER: Client " << client->getNickname() << " marked for disconnection. Cleanup complete.\n";
    // The actual removal of the client from the server's main client list and
    // closure of the socket will happen in your main epoll loop's cleanup phase.

}
//non dup
void Server::broadcastMessageToClients( std::shared_ptr<Client> client, const std::string& msg, bool quit) {
if (!client) {
	    std::cerr << "Server::handleQuit: Received null client pointer.\n";
	    return;
	}
	std::cout<<"Entering handling nick name command , client exists!\n";
	//std::string msg  = MessageBuilder::buildNickChange(client->getOldNick(), "failsafe", client->getNickname());
    std::cout<<"the message built for sending "<< msg <<"\n";
    std::set<std::shared_ptr<Client>> recipients;
	std::map<std::string, std::weak_ptr<Channel>> client_joined_channels_copy = client->getJoinedChannels();
	std::vector<std::string> channels_to_process;
    for (const auto& pair : client_joined_channels_copy) {
        std::cout<<"handling pairs and geting joined channles\n";
		if (std::shared_ptr<Channel> channel_sptr = pair.second.lock()) {
		    channels_to_process.push_back(pair.first);
		} else {
		    // This warning indicates a channel was removed from the server while still in client's list.
		    std::cerr << "WARNING: weak_ptr to channel '" << pair.first << "' expired in client's joined list.\n";
		}
		//CHANGE BACK 
		/*if (pair.second.lock()) {
			std::cout<<"a channel pushewd back \n";
            channels_to_process.push_back(pair.first);
    	}*/
	}
    for (const std::string& channel_name : channels_to_process) {
        auto channel_ptr = get_Channel(channel_name); 
        if (channel_ptr) {
			if (quit == true)
				 channel_ptr->removeClient(client->getNickname());
			 // Add all members of this channel to the recipients set.
            // `std::set` automatically handles duplicates, ensuring each client is added only once.
            /*for (const auto& pairInChannelMap : get_Clients())
			{
				std::shared_ptr<Client> memberClient = pairInChannelMap.second;
				recipients.insert(memberClient);
			}*/
			for (const auto& pairInChannelMap : channel_ptr->getAllClients()) { // Assuming Channel::getMembers() returns shared_ptrs
                 std::shared_ptr<Client> memberClient = pairInChannelMap.first.lock();
				if (memberClient->getFd() == client->getFd()) {
                       // std::cout << "DEBUG: Skipping self (" << memberClient->getNickname() << ") in channel '" << pairInChannelMap.first << "' broadcast." << std::endl;
                        continue;
				}
				else if (memberClient) {
					recipients.insert(memberClient);

				}
				//elese handle wekptr expiration
            }
        } else {
            std::cerr << "WARNING: Client " << client->getNickname() << " was in channel "
                      << channel_name << " but channel no longer exists on server.\n";
        }
    }
	for (const auto& recipientClient : recipients) {
        bool wasEmpty = recipientClient->isMsgEmpty();
        recipientClient->getMsg().queueMessage(msg);
        if (wasEmpty) {
			std::cout<<"it was empty yeah !!!!---------------\n";
            updateEpollEvents(recipientClient->getFd(), EPOLLOUT, true);
        }
        std::cout << "DEBUG: NICK message queued for FD " << recipientClient->getFd() << " (" << recipientClient->getNickname() << ")\n";
    }
	if (quit == true)
	{
		client->clearJoinedChannels();

    // Step 4: Mark the client for final server-level disconnection.
    	client->setQuit();
	}

}
/*void Server::broadcastMessageToClients(std::shared_ptr<Client> client, const std::string& msg, bool isQuitBroadcast) {
    if (!client) {
        std::cerr << "Server::broadcastMessageToClients: Received null client pointer.\n";
        return;
    }
    std::cout << "Entering broadcastMessageToClients for: " << client->getNickname() << ", message: [" << msg << "]\n";
    std::cout << "Message built for sending: " << msg << "\n";
	if (isQuitBroadcast)
		std::cout<<"this is the case that quit is true for some rfeason \n";
    // IMPORTANT: Create a COPY of the joined channels map.
    // This prevents iterator invalidation if the client's channels change concurrently.
    std::map<std::string, std::weak_ptr<Channel>> client_joined_channels_copy = client->getJoinedChannels();

    // This set will collect all UNIQUE recipients *excluding* the acting client.
    std::set<std::shared_ptr<Client>> recipients_for_broadcast;

    // Iterate over the COPY of joined channels
    for (const auto& pair : client_joined_channels_copy) {
        std::cout << "Handling channel: " << pair.first << "\n";
        // Lock the weak_ptr to get a shared_ptr, ensuring the channel still exists.
        // As you've noted, the weak_ptr is not expiring here, so this should always succeed.
        if (std::shared_ptr<Channel> channel_sptr = pair.second.lock()) {
            std::cout << "Channel locked successfully: " << channel_sptr->getName() << "\n";

            // Channel::getAllClients() returns a COPY of its map, which is good.
            // Iterate over this copy to prevent iterator invalidation if the channel's members change.
            // **Potential Bug Location: WeakPtrCompare used by Channel::getAllClients() internally**
            std::map<std::weak_ptr<Client>, std::pair<std::bitset<3UL>, int>, WeakPtrCompare> channel_clients_copy = channel_sptr->getAllClients();
            for (const auto& pairInChannelMap : channel_clients_copy) {
                // Lock the weak_ptr to get a shared_ptr for the member client.
                // As you've noted, the weak_ptr is not expiring here, so this should always succeed.
                std::shared_ptr<Client> memberClient = pairInChannelMap.first.lock();
                
                // IMPORTANT: Ensure memberClient is valid and it's NOT the acting client.
                // The acting client's message is handled by handleNickCommand.
                if (memberClient && memberClient->getFd() != client->getFd()) {
                    recipients_for_broadcast.insert(memberClient);
                    std::cout << "DEBUG: Added " << memberClient->getNickname() << " (FD " << memberClient->getFd() << ") to broadcast recipients.\n";
                } else if (memberClient && memberClient->getFd() == client->getFd()) {
                    std::cout << "DEBUG: Skipped acting client " << memberClient->getNickname() << " (FD " << memberClient->getFd() << ") for broadcast (handled separately).\n";
                } else {
                    // This 'else' block would only be hit if memberClient is nullptr,
                    // which you confirmed isn't happening. Good!
                    std::cerr << "WARNING: Encountered null memberClient during broadcast iteration (should not happen if weak_ptr is not expiring).\n";
                }
            }
        } else {
            // This 'else' block would only be hit if channel_sptr is nullptr,
            // which you confirmed isn't happening. Good!
            std::cerr << "WARNING: weak_ptr to channel '" << pair.first << "' expired during broadcast setup (should not happen).\n";
        }
    }

    // Now, queue the message for all collected unique recipients (excluding the acting client).
    for (const auto& recipientClient : recipients_for_broadcast) {
        bool wasEmpty = recipientClient->isMsgEmpty();
        recipientClient->getMsg().queueMessage(msg);
        if (wasEmpty) {
            updateEpollEvents(recipientClient->getFd(), EPOLLOUT, true);
            std::cout << "DEBUG: Message queued for FD " << recipientClient->getFd() << " (" << recipientClient->getNickname() << "). Epoll event updated.\n";
        } else {
            std::cout << "DEBUG: Message queued for FD " << recipientClient->getFd() << " (" << recipientClient->getNickname() << "). Message queue was not empty.\n";
        }
    }
    std::cout << "DEBUG: Finished broadcastMessageToClients. Total recipients: " << recipients_for_broadcast.size() << "\n";

    // Ignoring quit rules/code here as per your request.
}*/
void Server::broadcastMessageToChannel( std::shared_ptr<Channel> channel, const std::string& message_content, std::shared_ptr<Client> sender) {
    if (!channel) {
        std::cerr << "Error: Attempted to broadcast to a null channel pointer.\n";
        return;
    }
    if (message_content.empty()) {
        // No message to send, or error in message creation
        return;
    }

    // Get the map of weak_ptr<Client> from the Channel object
    const auto& client_modes_map = channel->getAllClients();

    // Iterate through all clients in this channel
    for (const auto& pair : client_modes_map) {
        // Try to obtain a shared_ptr from the weak_ptr.
        // If .lock() returns a valid shared_ptr, the client is still connected.
        if (auto current_client_ptr = pair.first.lock()) {
            // Check if the current client in the channel is the sender of the message.
            // If so, we typically don't send the broadcast back to the sender to avoid echo.
            if (sender && current_client_ptr->getFd() == sender->getFd()) {
                continue; // Skip the sender
            }
			if (current_client_ptr->isMsgEmpty() == true) {
				updateEpollEvents(current_client_ptr->getFd(), EPOLLOUT, true);
			}

			current_client_ptr->getMsg().queueMessage (message_content);
        } else {
            // This case means the weak_ptr is expired. The Client object no longer exists.
            // This is a sign that the client disconnected, and the channel's map
            // might need cleaning up (e.g., during periodic channel maintenance or on client disconnect).
            std::cerr << "WARNING: Expired weak_ptr to client found in channel "
                      << channel->getName() << " member list. Client likely disconnected.\n";

        }
    }
}
#include<chrono>

void Server::updateEpollEvents(int fd, uint32_t flag_to_toggle, bool enable) {
    auto it = _epollEventMap.find(fd);
    if (it == _epollEventMap.end()) {
        std::cerr << "ERROR: setEpollFlagStatus: FD " << fd << " not found in _epollEventMap. Cannot modify events. This might indicate a missing client cleanup.\n";
        return; // Exit immediately if FD is not tracked
    }
    uint32_t current_mask = it->second.events; // Get the currently stored mask
    uint32_t new_mask;
    // Calculate the new mask based on whether we're enabling or disabling the 'flag_to_toggle'
    // alternative : uint32_t new_mask = enable ? (current_mask | flag_to_toggle) : (current_mask & ~flag_to_toggle);

	if (enable) {
        new_mask = current_mask | flag_to_toggle; // Add the flag (turn it ON)
        //std::cout << "DEBUG: Attempting to enable flag 0x" << std::hex << flag_to_toggle << std::dec << " for FD " << fd << "\n";
    } else {
        new_mask = current_mask & ~flag_to_toggle; // Remove the flag (turn it OFF)
        //std::cout << "DEBUG: Attempting to disable flag 0x" << std::hex << flag_to_toggle << std::dec << " for FD " << fd << "\n";
    }
    // Optimization: Only call epoll_ctl if the mask has actually changed
    if (new_mask == current_mask) {
        std::cout << "DEBUG: Epoll flag 0x" << std::hex << flag_to_toggle << std::dec << " for FD " << fd << " already in desired state. No epoll_ctl call.\n";
        return; // No change needed
    }
	/*it->second.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, fd, &it->second) == -1) {
        perror("epoll_ctl EPOLL_CTL_MOD (setEpollFlagStatus) failed");
        std::cerr << "ERROR: Failed to set epoll flag status for FD " << fd
                  << ". Flag: 0x" << std::hex << flag_to_toggle << std::dec
                  << ", Enable: " << (enable ? "true" : "false")
                  << ". Error: " << strerror(errno) << ".\n";
        // If epoll_ctl fails here, it's often a severe issue (e.g., FD is bad).
        // Consider calling disconnectClient(fd) here in a production server.
	}*/
    // Update the 'events' member of the struct directly in your map
    it->second.events = new_mask;

    // Now, tell the kernel about the updated event set, passing a pointer to the stored struct.
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, fd, &it->second) == -1) {
        perror("epoll_ctl EPOLL_CTL_MOD (setEpollFlagStatus) failed");
        std::cerr << "ERROR: Failed to set epoll flag status for FD " << fd
                  << ". Flag: 0x" << std::hex << flag_to_toggle << std::dec
                  << ", Enable: " << (enable ? "true" : "false")
                  << ". Error: " << strerror(errno) << ".\n";
        // If epoll_ctl fails here, it's often a severe issue (e.g., FD is bad).
        // Consider calling disconnectClient(fd) here in a production server.
    } else {
        std::cout << "DEBUG: Epoll flag status updated for FD " << fd
                  << ". New total mask: 0x" << std::hex << new_mask << std::dec << "\n";
    }
	if (it->second.events & EPOLLOUT) {
		auto now = std::chrono::system_clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
		std::cout << "DEBUG: EPOLLOUT triggered at " << ms << " ms for FD " << fd << std::endl;
	}
}
/*Channel Server::getChannel(const std::string& channelName){
    auto it = _channels.find(channelName);
    if(it != _channels.end()){
        return it->second;
    }
    return nullptr; // Channel not found
}*/


/**
 * @brief when using weak pointers, when the original object goes, the wek pointer may 
 * be left dangling, of course its better to remove it methodically, but this function might have value
 * for valgrind testing if we have an issue
 * void cleanUpExpiredClients(std::map<std::weak_ptr<Client>, std::bitset<4>>& _ClientModes) {
    for (auto it = _ClientModes.begin(); it != _ClientModes.end(); ) {
        if (it->first.expired()) {
            it = _ClientModes.erase(it);  
        } else {
            ++it;
        }
    }
}
 * 
 */

# include <cstdlib>

//this is not an emergency 
std::string generateUniqueNickname() {
    std::string newNick;
    std::vector<std::string> adjectives = {"the_beerdrinking", "the_brave", "the_evil", "the_curious", "the_wild", "the_boring", "the_silent", "the_swift", "the_mystic", "the_slow", "the_lucky"};
        newNick = "anon" + adjectives[ static_cast<size_t>(rand()) % adjectives.size()] + static_cast<char>('a' + rand() % 26);
    return newNick;
}