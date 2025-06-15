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

#include <algorithm> // Required for std::transform
#include <cctype>    // Required for std::tolower (character conversion)


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
		int flag = 1;
		socklen_t optlen = sizeof(flag);
		//int buf_size = 1024;
		setsockopt(client_fd, SOL_SOCKET, SO_SNDBUF, &flag, optlen);
		setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &flag, optlen);
		/*std::cout<<"CHECKING DEBUG FLAG VALUES APPLIED?????????????????????????????????????\n";
		// Check TCP_NODELAY
		getsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &flag, &optlen);
		std::cout << "DEBUG: TCP_NODELAY applied? " << (flag ? "YES" : "NO") << std::endl;

		// Check SO_SNDBUF
		getsockopt(client_fd, SOL_SOCKET, SO_SNDBUF, &flag, &optlen);
		std::cout << "DEBUG: SO_SNDBUF value = " << flag << " bytes" << std::endl;
		std::cout<<"CHECKING DEBUG FLAG VALUES APPLIED ENDED?????????????????????????????????????\n";*/

		make_socket_unblocking(client_fd);
		//setsockopt(client_fd, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag)); 
		setup_epoll(epollfd, client_fd, EPOLLIN | EPOLLOUT | EPOLLET); //not a fan of epollet
		int timer_fd = setup_epoll_timer(epollfd, config::TIMEOUT_CLIENT);
		// errro handling if timer_fd failed
		// create an instance of new Client and add to server map
		_Clients[client_fd] = std::make_shared<Client>(client_fd, timer_fd);
		std::shared_ptr<Client> client = _Clients[client_fd];
		
		_timer_map[timer_fd] = client_fd;

		std::cout << "New Client created , fd value is  == " << client_fd << std::endl;
		
		set_current_client_in_progress(client_fd);
		client->setDefaults();
		_fd_to_nickname.insert({client_fd, client->getNickname()});
		_nickname_to_fd.insert({client->getNickname(), client_fd});
		if (!client->get_acknowledged()){
			//client->getMsg().queueMessage("NICK " +  client->getNickname() + "\r\n");
			
			client->getMsg().queueMessage(":localhost NOTICE * :initilization has begun.......\r\n");
			client->getMsg().queueMessage(":localhost CAP * LS :multi-prefix sasl\r\n");
			//no update epollout as its on for the first call
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

	/*int recvBufferSize;
    socklen_t optlen = sizeof(recvBufferSize);

    getsockopt(client_fd, SOL_SOCKET, SO_RCVBUF, &recvBufferSize, &optlen);
    std::cout << "WEEWOOOWEEWOOOOOO    DEBUGGIN:: Receive Buffer Size: " << recvBufferSize << " bytes" << std::endl;*/


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

void Server::shutDown() {
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

	std::cout<<"server shutDown completed"<<std::endl;
}

Server::~Server(){
	//shutDown();
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
	/**
	 * int pending_bytes;
socklen_t optlen = sizeof(pending_bytes);
getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &pending_bytes, &optlen);
std::cout << "DEBUG: Socket has " << pending_bytes << " bytes pending to send.\n";

	 * 
	 */

	int fd;
	fd = client->getFd();
	
	while (client->isMsgEmpty() == false && client->getMsg().getQueueMessage() != "") {
		
		std::string msg = client->getMsg().getQueueMessage();

		size_t remaining_bytes_to_send = client->getMsg().getRemainingBytesInCurrentMessage();
        std::string send_buffer_ptr = client->getMsg().getCurrentMessageCstrOffset();
		
		std::cout<<"checking the message from que before send ["<< msg <<"] and the fd = "<<fd<<"\n";
					// Right before your queueMessage call:
			/*std::cout << "DEBUG: params[0] content (raw): [" << msg << "]" << std::endl;
			// Or to show non-printable characters:
			std::cout << "DEBUG: params[0] content (hex): [";
			for (char c : msg) {
			    if (c == '\r') std::cout << "\\r";
			    else if (c == '\n') std::cout << "\\n";
			    else if (c >= ' ' && c <= '~') std::cout << c; // Printable ASCII
			    else std::cout << "\\x" << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)c << std::dec;
			}
			std::cout << "]" << std::endl;*/
		//while (msg.length() > 0) { // Keep trying to send THIS message until it's gone
        //    ssize_t bytes_to_send = msg.length();
            //ssize_t bytes_sent = send(fd, msg.c_str(), bytes_to_send, MSG_NOSIGNAL);
			
			/*socklen_t optlen = sizeof(remaining_bytes_to_send);
			getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &remaining_bytes_to_send, &optlen);
			std::cout << "DEBUG:: BEFORE SEND: Socket has " << remaining_bytes_to_send << " bytes pending to send.\n";*/
			/*struct tcp_info info;
			socklen_t info_len = sizeof(info);
			getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, &info_len);
			std::cout << "DEBUG BEFORE: Unacked Bytes (Real Pending Send Data): " << info.tcpi_unacked << std::endl;*/
			  
			ssize_t bytes_sent = send(fd, send_buffer_ptr.c_str(), remaining_bytes_to_send, MSG_NOSIGNAL);
			//send(fd, "", 0, MSG_NOSIGNAL);

			/*struct tcp_info info2;
			socklen_t info_len2 = sizeof(info2);
			getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info2, &info_len2);
			std::cout << "DEBUG AFTER: Unacked Bytes (Real Pending Send Data): " << info2.tcpi_unacked << std::endl;*/

			/*socklen_t optlen2 = sizeof(remaining_bytes_to_send);
			getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &remaining_bytes_to_send, &optlen2);
			std::cout << "DEBUG:: AFTER SEND: Socket has " << remaining_bytes_to_send << " bytes pending to send.\n";*/


			//usleep(5000); 
            if (bytes_sent == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Socket buffer is full. Cannot send more right now.
                    std::cout << "DEBUG::DEBUG send() returned EAGAIN/EWOULDBLOCK for FD " << fd << ". Pausing write.\n";
                    return; // <<< EXIT send_message. Epoll will re-trigger.
                } else {
                    // Fatal error (e.g., connection reset, broken pipe).
                    perror("send error");
                    //std::cerr << "ERROR: Fatal send error for FD " << fd << ". Disconnecting client.\n";
                    // !!!Call remove client function here !!!
                    return;
                }
            } else{
				//std::cout << "DEBUG: Before advanceCurrentMessageOffset: " << _bytesSentForCurrentMessage << std::endl;
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
	                std::cout << "ÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖDEBUG: Partial send for FD " << fd << ". Sent " << bytes_sent
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
		//std::string test = "PING :server\r\n";//":" + params[0] + " NICK " +  client->getNickname() + "\r\n";
		//	send(client->getFd(), test.c_str(), test.length(), MSG_NOSIGNAL);

		if (!client->get_acknowledged())
		{
			//ignore this for now
			//std::cout<<"SENDING WELCOME MESSAGE INCOMING 11111111111111111111\n";
			client->set_acknowledged();		
		}
		//usleep(3000);
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
	throw ServerException(ErrorType::NO_CHANNEL_INMAP, "can not get_Channel()"); // this should be no channel in map 
}

void Server::handleJoinChannel(std::shared_ptr<Client> client, const std::string& channelName, const std::string& password)
{
	// HANDLE HERE 
	// channel modes and client modes need to be seperate 

	if (!password.empty())
		std::cout<<"attempting tooooooo!--------------------------------------";

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
	
	// HANDLE HERE must check!!
	if (!currentChannel->canClientJoin(client->getNickname(), password))
	{
		std::cout<<"client is denied join rights !--------------------------------------";
		// this must bve handled 
		return ;
	}
		// set msg
		//return
	client->addChannel(channelName, get_Channel(channelName));
	currentChannel->addClient(client);
	std::string ClientList = currentChannel->getAllNicknames();
	if (ClientList.empty())
		std::cout<<"WE HAVE A WIERDS PROBLEM AND CLIENT LIST IS NULL FOR JOIN\n";
	
	if (client->isMsgEmpty() == true)
		updateEpollEvents(client->getFd(), EPOLLOUT, true);
	//std::string test = "PING :server\r\n";//":" + params[0] + " NICK " +  client->getNickname() + "\r\n";
	//send(client->getFd(), test.c_str(), test.length(), MSG_NOSIGNAL);
	//client->getMsg().queueMessage(":localhost NOTICE * :Processing update...\r\n");
	std::cout<<"channel should allow join !--------------------------------------";
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
	//client->getMsg().queueMessage(":localhost NOTICE * :Processing update...\r\n");
	//std::string test = "PING :server\r\n";//":" + params[0] + " NICK " +  client->getNickname() + "\r\n";
	//send(client->getFd(), test.c_str(), test.length(), MSG_NOSIGNAL);
	//std::string arg = "";
	//client->getMsg().queueMessage(arg);
	//client->getMsg().queueMessage(":server NOTICE * : \r\n");
	//client->getMsg().queueMessage("MODE " + client->getNickname() + " +i\r\n");
	//client->getMsg().queueMessage("WHO " + client->getNickname() + "\r\n");
				//client->getMsg().queueMessage(":server 001 " + client->getNickname() + " :Welcome!\r\n");

	//client->getMsg().queueMessage("PING :localhost\r\n");
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

void Server::handleModeCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params){
 // --- 1. Basic Parameter Count Check (Always first) ---
    ModeCommandContext context;
 	if (params.empty()) {
        client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NEED_MORE_PARAMS, {client->getNickname(), "MODE"}));
		updateEpollEvents(client->getFd(), EPOLLOUT, true);

		return;
    }
    std::string target = params[0];
    bool targetIsChannel = (target[0] == '#');

    // --- 2. Handle Logic based on Target Type ---
    if (targetIsChannel) {
        std::shared_ptr<Channel> channel;
        // Attempt to get the channel object, handling non-existent channels via exception.
        try {
            channel = get_Channel(target); // This can throw ServerException(ErrorType::NO_CHANNEL_INMAP)
        } catch (const ServerException& e) {
            // Catch the specific exception for non-existent channel
            if (e.getType() == ErrorType::NO_CHANNEL_INMAP) {
                client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NO_SUCH_CHANNEL, {target}));
				updateEpollEvents(client->getFd(), EPOLLOUT, true);

				return; // Abort: Channel does not exist
            }
		}
		std::pair<MsgType, std::vector<std::string>> validationResult = channel->initialModeValidation(client->getNickname(), params.size());
		//channel->initialModeValidation(client->getNickname(), params.size());
		if (validationResult.first != MsgType::NONE) {
            // If validation resulted in an error or a listing reply (like RPL_CHANNELMODEIS),
            // send the appropriate message and return.
            client->getMsg().queueMessage(MessageBuilder::generateMessage(validationResult.first, validationResult.second));
			updateEpollEvents(client->getFd(), EPOLLOUT, true);

			return; // Abort: Validation failed or command was for listing
        }
		validationResult = channel->modeSyntaxValidator(client->getNickname(), params);
		if (validationResult.first != MsgType::NONE) {
            client->getMsg().queueMessage(MessageBuilder::generateMessage(validationResult.first, validationResult.second));
			updateEpollEvents(client->getFd(), EPOLLOUT, true);

			return; // Abort: modeSyntaxValidator found an error and sent a message.
    	}

		std::vector<std::string> modeparams = channel->applymodes(params);
		std::vector<std::string> messageParams;
		if (!modeparams.empty())
		{
			messageParams.push_back(client->getNickname());
			messageParams.push_back(client->getClientUname());
			messageParams.push_back(channel->getName());
			if (!modeparams[0].empty())
				messageParams.push_back(modeparams[0]);
			if (!modeparams[1].empty())
				messageParams.push_back(modeparams[1]);
			else
				messageParams.push_back("");
			client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::CHANNEL_MODE_CHANGED, messageParams));
			updateEpollEvents(client->getFd(), EPOLLOUT, true);

		}
		

	}
	else { // Target is a client (private mode)
        // A client can only set/list modes on themselves for private modes.
        if (target != client->getNickname()) {
            client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::INVALID_TARGET, {client->getNickname(), target}));
            updateEpollEvents(client->getFd(), EPOLLOUT, true);
			return; // Abort: Cannot set private modes for another user
        }

        // Handle private mode listing specifically (as Channel::initialModeValidation isn't called for clients).
        if (params.size() == 1) {
            client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::RPL_UMODEIS, {client->getNickname(), client->getPrivateModeString()}));
			updateEpollEvents(client->getFd(), EPOLLOUT, true);
			return; // Abort: Command was for listing
        }
		if (params[1] == "+i")
		{
			client->setMode(clientPrivModes::INVISABLE);
			client->getMsg().queueMessage(":**:" + params[0] + " MODE " + params[0] +" :+i\r\n**");
			updateEpollEvents(client->getFd(), EPOLLOUT, true);
			return;

		}

    }

    // --- 3. If we reach here, initial validations (basic param count, target existence, and permissions) passed. ---
    // Now, proceed to modeSyntaxValidator for detailed flag/parameter parsing.
    // modeSyntaxValidator will use the 'context' (target, targetIsChannel, channel) to perform its checks.
    

    // --- 4. If all checks pass, proceed with the actual mode application logic. ---
    // ... (Your mode application logic will go here, which will use 'context') ...
// }
}

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
#include <fstream>
//this is not an emergency 

std::string generateUniqueNickname() {
    std::vector<std::string> adjectives = {
        "the_beerdrinking", "the_brave", "the_evil", "the_curious", "the_wild",
        "the_boring", "the_silent", "the_swift", "the_mystic", "the_slow", "the_lucky"
    };

    std::string newNick = "anon" + adjectives[rand() % adjectives.size()] + static_cast<char>('a' + rand() % 26);

    std::ofstream configFile("config");  // ✅ Opens the file for writing
    if (configFile.is_open()) {
        configFile << "servers = (\n";
        configFile << "  {\n";
        configFile << "    address = \"localhost\";\n";
        configFile << "    port = 6669;\n";
        configFile << "    nick = \"" << newNick << "\";\n";  // ✅ Ensures Irssi uses generated nickname
        configFile << "    user = \"user_" << newNick << "\";\n";  // ✅ Explicit user field
        configFile << "    realname = \"real_" << newNick << "\";\n";
        configFile << "  }\n";
        configFile << ");\n\n";

        // ✅ Adding explicit settings to override machine defaults
        configFile << "settings = {\n";
        configFile << "  core = {\n";
        configFile << "    real_name = \"real_" << newNick << "\";\n";
        configFile << "    user_name = \"user_" << newNick << "\";\n";
        configFile << "    nick = \"" << newNick << "\";\n";
        configFile << "  };\n";
        configFile << "};\n\n";

        // ✅ Define a placeholder channel entry to force correct config parsing
        configFile << "channels = (\n";
        configFile << "  {\n";
        configFile << "    name = \"#test_channel\";\n";
        configFile << "    chatnet = \"LocalNet\";\n";
        configFile << "    autojoin = \"No\";\n";
        configFile << "  }\n";
        configFile << ");\n\n";

        // ✅ Keep structured elements for proper config interpretation
        configFile << "IRCSource = {\n";
        configFile << "  type = \"IRC\";\n";
        configFile << "  max_kicks = \"1\";\n";
        configFile << "  max_msgs = \"4\";\n";
        configFile << "  max_whois = \"1\";\n";
        configFile << "};\n";

        configFile.close();
        std::cout << "Config updated! Generated nickname: " << newNick << std::endl;
    } else {
        std::cerr << "Error: Unable to write to config file." << std::endl;
    }

    return newNick;
}
/*std::string generateUniqueNickname() {
    std::string newNick;
    std::vector<std::string> adjectives = {"the_beerdrinking", "the_brave", "the_evil", "the_curious", "the_wild", "the_boring", "the_silent", "the_swift", "the_mystic", "the_slow", "the_lucky"};
        newNick = "anon" + adjectives[ static_cast<size_t>(rand()) % adjectives.size()] + static_cast<char>('a' + rand() % 26);
    std::ofstream configFile("config");  // ✅ Opens the file for writing
    if (configFile.is_open()) {
        configFile << "servers = (\n";
        configFile << "  {\n";
        configFile << "    address = \"localhost\";\n";
        configFile << "    port = 6669;\n";
        configFile << "    nick = \"" << newNick << "\";\n";  // ✅ Inserts generated name
        configFile << "    username = \"user_" << "user_" + newNick << "\";\n";
        configFile << "    realname = \"real_" << newNick + "\";\n";
        configFile << "  }\n";
        configFile << ");\n";

        configFile.close();
        std::cout << "Config updated! Generated nickname: " << newNick << std::endl;
    } else {
        std::cerr << "Error: Unable to write to config file." << std::endl;
    }

	return newNick;
}*/


/*void Server::handleWhoIs(std::shared_ptr<Client> client, std::string param) {
		--If target_nick is generatedname (your client's actual nickname):
		Send :<your_server_name> 311 generatedname generatedname ~user host * :realname\r\n
		Send :<your_server_name> 318 generatedname generatedname :End of WHOIS list\r\n
--
		std::cout<<"show me the param = "<<param<<" and the nick ="<<client->getNickname()<<"\n";
		if (param != client->getNickname()) // or name no exist for some reason 
		{
		    std::string whoisResponse = ":localhost 311 " + client->getNickname() + " " + client->getNickname()  + " localhost * :Real Name\r\n";
		    std::string whoisEnd = ":localhost 318 " + client->getNickname() + " :End of WHOIS list\r\n";
			//sendToClient(client.fd, whoisResponse);
			//client->getMsg().queueMessage(":" + client->getNickname() + " NICK " +  client->getNickname() + "\r\n");
			client->getMsg().queueMessage(whoisResponse);
			client->getMsg().queueMessage(whoisEnd);

			//client->getMsg().queueMessage(":localhost 401 " + client->getNickname() + " " + params[0] + " :NO suck nick\r\n");
			// if msg empty !!
			updateEpollEvents(client->getFd(), EPOLLOUT, true);
		}*/
//		Send :<your_server_name> 401 generatedname configname :No such nick/channel\r\n (using generatedname as the source of the error, as that's the client's real nick).

void Server::set_nickname_in_map(std::string nickname, int fd) {
    std::string lower_nickname = nickname;
    std::transform(lower_nickname.begin(), lower_nickname.end(), lower_nickname.begin(),
                   [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });
    _nickname_to_fd[lower_nickname] = fd; // Store lowercase as key
    _fd_to_nickname[fd] = nickname;      // Store original case here if needed (less critical for WHOIS lookup)
}


std::shared_ptr<Client> Server::findClientByNickname(const std::string& nickname) {
    // 1. Prepare the nickname for case-insensitive lookup
    //    IRC nicknames are case-insensitive for comparison.
    std::string lower_nickname = nickname;
    std::transform(lower_nickname.begin(), lower_nickname.end(), lower_nickname.begin(),
                   [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

    // 2. Look up the file descriptor using the canonical (lowercase) nickname
    auto it_nick_to_fd = _nickname_to_fd.find(lower_nickname);

    if (it_nick_to_fd != _nickname_to_fd.end()) {
        int client_fd = it_nick_to_fd->second; // Found the FD

        // 3. Use the file descriptor to get the Client shared_ptr from _Clients map
        auto it_client = _Clients.find(client_fd);
        if (it_client != _Clients.end()) {
            return it_client->second; // Found the Client shared_ptr
        }
        // This case should ideally not happen if _nickname_to_fd is kept consistent with _Clients
        // but it's a good defensive check.
        return nullptr; // Client not found in _Clients map despite FD being present
    }

    return nullptr; // Nickname not found in _nickname_to_fd map
}



void Server::handleWhoIs(std::shared_ptr<Client> requester_client, std::string target_nick) {
    // 1. Find the target client
    std::shared_ptr<Client> target_client = findClientByNickname(target_nick);

    // 2. Handle Nick Not Found (ERR_NOSUCHNICK)
    if (!target_client) {
        std::string err_msg = ":" + _server_name + " 401 " + requester_client->getNickname() + " " + target_nick + " :No such nick/channel\r\n";
        requester_client->getMsg().queueMessage(err_msg);
        updateEpollEvents(requester_client->getFd(), EPOLLOUT, true);
        return; // Important: stop here if nick not found
    }

    // 3. Construct and queue WHOIS replies for the TARGET CLIENT

    // RPL_WHOISUSER (311)
	// Ensure target_client->getHostname() exists and returns the correct string
    std::string user_info_msg = ":" + _server_name + " 311 " + requester_client->getNickname() + " "
                                + target_client->getNickname() + " "
                                + target_client->getClientUname() + " "
                                + target_client->getHostname() + " * :" // Make sure getHostname is implemented
                                + target_client->getfullName() + "\r\n";
    requester_client->getMsg().queueMessage(user_info_msg);

   // RPL_WHOISSERVER (312)
    std::string server_info_msg = ":" + _server_name + " 312 " + requester_client->getNickname() + " "
                                + target_client->getNickname() + " "
                                + _server_name + " :A & J IRC server\r\n";
    requester_client->getMsg().queueMessage(server_info_msg);

	// RPL_WHOISOPERATOR (313)
    // Check if target_client is an operator
    if (target_client->isOperator()) { // Requires isOperator() in Client
        std::string oper_msg = ":" + _server_name + " 313 " + requester_client->getNickname() + " "
                            + target_client->getNickname() + " :is an IRC operator\r\n";
        requester_client->getMsg().queueMessage(oper_msg);
    }


	// RPL_WHOISIDLE (317)
    // Requires getIdleTime() and getSignonTime() in Client
    long idle_seconds = target_client->getIdleTime(); // Implement this
    time_t signon_time = target_client->getSignonTime(); // Implement this
    std::string idle_msg = ":" + _server_name + " 317 " + requester_client->getNickname() + " "
                        + target_client->getNickname() + " " + std::to_string(idle_seconds) + " "
                        + std::to_string(signon_time) + " :seconds idle, signon time\r\n";
    requester_client->getMsg().queueMessage(idle_msg);

	// RPL_WHOISCHANNELS (319)
    std::string channels_str = "";
    const auto& joined_channels_map = target_client->getJoinedChannels(); // Get the map of joined channels
    for (const auto& pair : joined_channels_map) {
        if (auto channel_ptr = pair.second.lock()) { // Safely get shared_ptr from weak_ptr
            channels_str += channel_ptr->getClientModePrefix(target_client) + channel_ptr->getName() + " "; // Implement getClientModePrefix in Channel
        }
    }
    if (!channels_str.empty()) {
        // Remove trailing space if any
        channels_str.pop_back(); // Remove last space
        std::string channels_msg = ":" + _server_name + " 319 " + requester_client->getNickname() + " "
                                + target_client->getNickname() + " :" + channels_str + "\r\n";
        requester_client->getMsg().queueMessage(channels_msg);
    }

    // RPL_ENDOFWHOIS (318)
    std::string end_msg = ":" + _server_name + " 318 " + requester_client->getNickname() + " "
                        + target_client->getNickname() + " :End of WHOIS list\r\n";
    requester_client->getMsg().queueMessage(end_msg);

    // 4. Update Epoll Events to send queued messages
    updateEpollEvents(requester_client->getFd(), EPOLLOUT, true);
}
