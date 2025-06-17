#include "Server.hpp"
#include <sys/types.h>
#include <unistd.h>
#include <string.h> //strlen
#include <iostream> // testing with cout
#include <sys/socket.h>
#include <sys/epoll.h>
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

//#include <algorithm> // Required for std::transform
#include <cctype>    // Required for std::tolower (character conversion)


Server::Server(){
	std::cout << "#### Server instance created.\n";
}

/**
 * @brief Construct a new Server:: Server object
 * 
 * @param port 
 * @param password 
 */
Server::Server(int port , const std::string& password) : _port(port), _password(password) {
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
//#include <sys/types.h>
//#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

void Server::create_Client(int epollfd) {
 	// Handle new incoming connection
	int client_fd = accept(getFd(), nullptr, nullptr);
 	if (client_fd < 0) {
		throw ServerException(ErrorType::ACCEPT_FAILURE, "debuggin: create Client");
	}
	int flag = 1;
	socklen_t optlen = sizeof(flag);
	setsockopt(client_fd, SOL_SOCKET, SO_SNDBUF, &flag, optlen);
	setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &flag, optlen);
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
		client->getMsg().queueMessage(MessageBuilder::generateInitMsg());
		//no update epollout as its on for the first call
		set_client_count(1);		
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
// void Server::remove_Client(int client_fd) {
// 	// get clients shared pointer
// 	auto client_to_remove = get_Client(client_fd);
// 	if (!client_to_remove) {
//         std::cerr << "ERROR: remove_Client called for non-existent client FD: " << client_fd << std::endl;
//         return;
//     }

// 	// iterate through channels that client_to_remove was part of
// 	// client should know what channels they were in std::map<std::string, std::weak_ptr<Channel>>
// 	// call Channel::removeClient() for each channel
// 	// channel_ptr->removeClient(client_to_remove->getNickname())
// 	// return a bool if the channel is empty after client departed
// 	// Boadcast QUIT for each channel.
// 	// :<nickname>!<username>@<hostname> QUIT :<reason>
// 	// if the boolean return says channel is empty, then remove that channel from server map.



	
// 	// epoll something
// 	epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_to_remove->get_timer_fd(), 0);
// 	close(client_to_remove->get_timer_fd());
	
// 	// epoll something
// 	epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
// 	close(client_fd);
	
// 	// channel.getClient().erase();
// 	_Clients.erase(client_fd);  // why capital C? We have a map of clients
// 	_epollEventMap.erase(client_fd);
	
// 	//std::map<int, struct epoll_event> _epollEventMap;
// 	_client_count--;

// 	// erase both from the fd and from the nickname - now we don't have a record of the user in the server
// 	_nickname_to_fd.erase(client_to_remove->getNickname());
// 	_fd_to_nickname.erase(client_fd);	

// 	std::cout<<"client has been removed"<<std::endl;
// }


void Server::remove_Client(int client_fd) {
    // 1. Get the shared pointer to the client. This client object holds all its state,
    //    including which channels it joined.
    std::shared_ptr<Client> client_to_remove = get_Client(client_fd);
    if (!client_to_remove) {
        std::cerr << "ERROR: remove_Client called for non-existent client FD: " << client_fd << std::endl;
        return;
    }

    // --- Step 2: Handle channel disassociation and notification ---
    // IRC protocol requires informing channels when a client leaves, even due to disconnect.
    // Create a copy of channel names to iterate safely, as modifying a map while iterating can invalidate iterators.
    std::vector<std::string> joined_channel_names;
    for (const auto& pair : client_to_remove->getJoinedChannels()) {
        if (pair.second.lock()) { // Ensure the weak_ptr is still valid and the channel object exists
            joined_channel_names.push_back(pair.first);
        }
    }

    // Loop through each channel the client was a member of
    for (const std::string& channel_name : joined_channel_names) {
        std::shared_ptr<Channel> channel_ptr;
        try {
            channel_ptr = get_Channel(channel_name);
        } catch (const ServerException& e) {
            if (e.getType() == ErrorType::NO_CHANNEL_INMAP) {
                // This means the channel was already cleaned up by another client's action (e.g., last PART/QUIT).
                std::cerr << "WARNING: Client " << client_to_remove->getNickname() << " was in channel "
                          << channel_name << ", but it no longer exists on server. Skipping channel cleanup for this client.\n";
                continue;
            }
            throw; // Re-throw other unexpected exceptions
        }

        if (channel_ptr) { // Ensure the channel pointer is valid
            std::cout << "SERVER: Notifying and removing client " << client_to_remove->getNickname()
                      << " from channel " << channel_name << " due to disconnect.\n";

            // A) Build the QUIT message to broadcast to remaining channel members.
            // This is equivalent to an implicit QUIT command from the client.
            std::string client_prefix = ":" + client_to_remove->getNickname() + "!" +
                                       client_to_remove->getUsername() + "@" + // Make sure Client has getUsername() and getHostname()
                                       client_to_remove->getHostname();
            std::string quit_message = client_prefix + " QUIT :Client disconnected\r\n"; // Default reason

            // B) Broadcast the QUIT message to other clients in this specific channel.
            // Your `broadcastMessageToChannel` skips the sender, which is correct here.
            broadcastMessageToChannel(channel_ptr, quit_message, client_to_remove);

            // C) Remove the client from the channel's internal member list.
            // Channel::removeClient should also update client_to_remove's _joinedChannels list.
            bool channel_became_empty = channel_ptr->removeClient(client_to_remove->getNickname());

            // D) If the channel became empty after this client left, remove it from the server's map.
            if (channel_became_empty) {
                std::cout << "SERVER: Channel '" << channel_name << "' is now empty. Deleting from server's master list.\n";
                _channels.erase(channel_name); // Assuming _channels is the correct map name.
            }
        }
    }
    // After iterating through all channels, clear the client's internal list of joined channels.
    client_to_remove->clearJoinedChannels();


    // --- Step 3: Perform server-wide cleanup ---
    // Remove client's nickname to FD and FD to nickname mappings.
    _nickname_to_fd.erase(client_to_remove->getNickname());
    _fd_to_nickname.erase(client_fd);

    // Remove timer FD from epoll and close it.
    epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_to_remove->get_timer_fd(), 0);
    close(client_to_remove->get_timer_fd());

    // Remove client FD from epoll and close it.
    epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
    close(client_fd);
    
    // Remove the client's shared_ptr from the server's main client map.
    // Assuming `_Clients` (capital C) is the correct member variable name for your map of shared_ptrs.
    _Clients.erase(client_fd);
    // Remove the epoll event structure associated with this client FD.
    _epollEventMap.erase(client_fd);
    
    // Decrement the active client count.
    _client_count--;
    std::cout << "Client has been completely removed. Total clients: " << _client_count << std::endl;
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
		if (it->first == fd) {
			return it->second;
		}
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
    client->appendIncomingData(buffer, bytes_read);
	  // This loop ensures all complete messages are processed.
    while (client->extractAndParseNextCommand()) {
        // This is the point where you know the client is active and sent a valid message.
        resetClientTimer(client_fd, config::TIMEOUT_CLIENT);
        client->set_failed_response_counter(-1);
        // --- DELEGATE TO COMMANDDISPATCHER ---
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

#include <iomanip>
void Server::send_message(const std::shared_ptr<Client>& client)
{
	int fd;
	fd = client->getFd();
	
	while (!client->isMsgEmpty()) { // && client->getMsg().getQueueMessage() != ""	
		std::string msg = client->getMsg().getQueueMessage();
		size_t remaining_bytes_to_send = client->getMsg().getRemainingBytesInCurrentMessage();
        std::string send_buffer_ptr = client->getMsg().getCurrentMessageCstrOffset();
		
		std::cout<<"checking the message from que before send ["<< msg <<"] and the fd = "<<fd<<"\n";
		ssize_t bytes_sent = send(fd, send_buffer_ptr.c_str(), remaining_bytes_to_send, MSG_NOSIGNAL);
        if (bytes_sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Socket buffer is full. Cannot send more right now.
                std::cout << "DEBUG::DEBUG send() returned EAGAIN/EWOULDBLOCK for FD " << fd << ". Pausing write.\n";
                return; // <<< EXIT send_message. Epoll will re-trigger.
            } 
            // Fatal error (e.g., connection reset, broken pipe).
            perror("send error");
            //std::cerr << "ERROR: Fatal send error for FD " << fd << ". Disconnecting client.\n";
            // !!!Call remove client function here !!!
            return;
        } else {
			//std::cout << "DEBUG: Before advanceCurrentMessageOffset: " << _bytesSentForCurrentMessage << std::endl;
			client->getMsg().advanceCurrentMessageOffset(bytes_sent); 
		 	if (client->getMsg().getRemainingBytesInCurrentMessage() == 0) {
            // The entire current message has been sent.
            client->getMsg().removeQueueMessage();
            std::cout << "DEBUG: Full message sent for FD " << fd << ". Moving to next message.\n";
            // The outer 'while' loop will now check for the next message in the queue.
	        } else {
	            // Partial send of the current message.
	            // Still more bytes to send for *this* message.
	            // Leave EPOLLOUT enabled. The next time send_message is called,
	            // it will automatically pick up from the new offset.
	            //std::cout << "ÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖÖDEBUG: Partial send for FD " << fd << ". Sent " << bytes_sent
	            //          << " bytes. Remaining: " << client->getMsg().getRemainingBytesInCurrentMessage() << ".\n";
	            return; // Exit send_message. Epoll will re-trigger for remaining bytes.
			}
		}
	}
	if (client->isMsgEmpty()) {
		if (!client->get_acknowledged()) {
			client->set_acknowledged();		
		}
		updateEpollEvents(fd, EPOLLOUT, false);
	}

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

std::shared_ptr<Channel> Server::get_Channel(const std::string& ChannelName) {
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
	client->getMsg().queueMessage(msg);
	std::cout << "DEBUG NICK Self-Message: Message QUEUED for Client " << client->getNickname() 
              << " (FD: " << client->getFd() << "). Current queue size after: " << client->getMsg().getQue().size() << "\n";
}

void Server::handleQuit(std::shared_ptr<Client> client) {
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
           	//client->getMsg().setType(MsgType::CLIENT_QUIT, {client->getNickname(), client->getClientUname()});
			std::string part_message =  MessageBuilder::generateMessage(MsgType::CLIENT_QUIT, {client->getNickname(), client->getClientUname()});

            broadcastMessageToChannel(channel_ptr, part_message, client); // 'client' is the sender to be excluded from broadcast
        } else {
            std::cerr << "WARNING: Client " << client->getNickname() << " was in channel "
                      << channel_name << " but channel no longer exists on server.\n";
        }
    }
    client->clearJoinedChannels();

    // mark the client for final server-level disconnection.
    client->setQuit();

    // Step 5: Send a final server-generated message *only to the quitting client*.
    std::cout << "SERVER: Client " << client->getNickname() << " marked for disconnection. Cleanup complete.\n";
    // closure of the socket will happen in your main epoll loop's cleanup phase.
}

void Server::broadcastMessageToClients( std::shared_ptr<Client> client, const std::string& msg, bool quit) {
if (!client) {
	    std::cerr << "Server::handleQuit: Received null client pointer.\n";
	    return;
	}
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
	}
    for (const std::string& channel_name : channels_to_process) {
        auto channel_ptr = get_Channel(channel_name); 
        if (channel_ptr) {
			if (quit == true)
				 channel_ptr->removeClient(client->getNickname());

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
	if (quit == true) {
		client->clearJoinedChannels();
	}

}

void Server::handleModeCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params){
 // --- 1. Basic Parameter Count Check (Always first) ---
 	if (params.empty()) {
        client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NEED_MORE_PARAMS, {client->getNickname(), "MODE"}));
		updateEpollEvents(client->getFd(), EPOLLOUT, true);

		return;
    }
    std::string target = params[0];
    bool targetIsChannel = (target[0] == '#');

    // --- 2. Handle Logic based on Target Type  ie channel or not---
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
			broadcastMessageToChannel(channel, MessageBuilder::generateMessage(MsgType::CHANNEL_MODE_CHANGED, messageParams), client);
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
}

void Server::handleCapCommand(const std::string& nickname, std::deque<std::string>& que, bool& capSent){
		
		que.push_back(":localhost CAP " + nickname + " LS :multi-prefix sasl\r\n");
        que.push_back(":localhost CAP " + nickname + " ACK :multi-prefix\r\n");
        que.push_back(":localhost CAP " + nickname + " ACK :END\r\n");
		capSent = true;
		//client->setHasSentCap();
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
			current_client_ptr->getMsg().queueMessage (message_content);
			if (!current_client_ptr->isMsgEmpty() == true) {
				updateEpollEvents(current_client_ptr->getFd(), EPOLLOUT, true);
			}


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
// could we add if msg empty check here , then all cases where we call updatepoll must be called before message qued
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
        return; // No change needed this checks if epollout was already triggered and avoids doing it again 
    }
    it->second.events = new_mask;

    // Now, tell the kernel about the updated event set, passing a pointer to the stored struct.
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, fd, &it->second) == -1) {
        perror("epoll_ctl EPOLL_CTL_MOD (setEpollFlagStatus) failed");
    } else {
        std::cout << "DEBUG: Epoll flag status updated for FD " << fd
                  << ". New total mask: 0x" << std::hex << new_mask << std::dec << "\n";
    }

	// debugging only 
	if (it->second.events & EPOLLOUT) {
		auto now = std::chrono::system_clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
		std::cout << "DEBUG: EPOLLOUT triggered at " << ms << " ms for FD " << fd << std::endl;
	}
}



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
