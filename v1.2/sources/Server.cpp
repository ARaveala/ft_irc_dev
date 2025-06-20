#include <sys/types.h>
#include <unistd.h>
#include <string.h> //strlen
#include <iostream> // testing with cout
#include <sstream>  // for passing parameters
#include <sys/socket.h>
#include <sys/epoll.h>
#include <map>
#include <memory> // shared pointers
#include <cctype>
//#include "SendException.hpp"
#include <algorithm> // find_if
#include <unordered_set>
#include <cctype>    // Required for std::tolower (character conversion)
#include <vector>
#include <ctime>

#include "config.h"
#include "ServerError.hpp"
#include "Channel.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include "MessageBuilder.hpp"

class ServerException;


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

bool Server::matchTimerFd(int fd){
	if (_timer_map.find(fd) == _timer_map.end()) {
		return false;
	}
	return true;
}
std::vector<std::string> Server::splitCommaList(const std::string& input) {
    std::vector<std::string> result;
    std::stringstream ss(input);
    std::string token;
    while (std::getline(ss, token, ',')) {
        result.push_back(token);
    }
    return result;
}

/**
 * @brief Here a client is accepted , error checked , socket is adusted for non-blocking
 * the client fd is added to the epoll and then added to the Client map. a welcome message
 * is sent as an acknowlegement message back to irssi.
 */

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
	setup_epoll(epollfd, client_fd, EPOLLIN | EPOLLOUT | EPOLLET);
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

void Server::remove_Client(int client_fd) {
    // 1. Get the shared pointer to the client. This client object holds all its state,
    //    including which channels it joined.
    std::shared_ptr<Client> client_to_remove = get_Client(client_fd);
    if (!client_to_remove) {
        std::cerr << "ERROR: remove_Client called for non-existent client FD: " << client_fd << std::endl;
        return;
    }
	 std::cout << "SERVER: REMOVAL OF CLIENT active message should be qued \n";
    //std::string quit_message =  MessageBuilder::generateMessage(MsgType::CLIENT_QUIT, {client_to_remove->getNickname(), client_to_remove->getClientUname()});//client_prefix + " QUIT :Client disconnected\r\n"; // Default reason
	//broadcastMessage(quit_message, client_to_remove, nullptr, true, nullptr);
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
        handleQuit(client);
        return;
    }
    client->appendIncomingData(buffer, bytes_read);
	  // This loop ensures all complete messages are processed.
    while (client->extractAndParseNextCommand()) {
        // This is the point where you know the client is active and sent a valid message.
        resetClientTimer(client_fd, config::TIMEOUT_CLIENT);
        client->set_failed_response_counter(-1);
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
/*bool confirmClientAlive(int timer_fd, int client_fd) {
    set_failed_response_counter(1);

}*/
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
   // clientit->second->sendPing();
    clientit->second->set_failed_response_counter(1);
    resetClientTimer(timerit->first, config::TIMEOUT_CLIENT);
	return false;
}

#include <iomanip>
void Server::send_message(const std::shared_ptr<Client>& client)
{
	int fd;
	fd = client->getFd();
	
	while (!client->isMsgEmpty()) {	
		std::string msg = client->getMsg().getQueueMessage();
		size_t remaining_bytes_to_send = client->getMsg().getRemainingBytesInCurrentMessage();
        std::string send_buffer_ptr = client->getMsg().getCurrentMessageCstrOffset();
		
		std::cout<<"checking the message from que before send ["<< msg <<"] and the fd = "<<fd<<"\n";
		ssize_t bytes_sent = send(fd, send_buffer_ptr.c_str(), remaining_bytes_to_send, MSG_NOSIGNAL);
        if (bytes_sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Socket buffer is full. Cannot send more right now.
                std::cout << "DEBUG::DEBUG send() returned EAGAIN/EWOULDBLOCK for FD " << fd << ". Pausing write.\n";
                return;
            } 
            // Fatal error (e.g., connection reset, broken pipe).
            perror("send error");
            //std::cerr << "ERROR: Fatal send error for FD " << fd << ". Disconnecting client.\n";
            // !!!Call remove client function here !!!
            return;
        }
		//std::cout << "DEBUG: Before advanceCurrentMessageOffset: " << _bytesSentForCurrentMessage << std::endl;
		client->getMsg().advanceCurrentMessageOffset(bytes_sent); 
		if (client->getMsg().getRemainingBytesInCurrentMessage() == 0) {
        // The entire current message has been sent.
        client->getMsg().removeQueueMessage();
        std::cout << "DEBUG: Full message sent for FD " << fd << ". Moving to next message.\n";
        // The outer 'while' loop will now check for the next message in the queue.
	    } else {
	       // Partial send of the current message.
	        return;
		}
	}
	if (client->isMsgEmpty()) {
		if (!client->get_acknowledged()) {
			client->set_acknowledged();		
		}
		updateEpollEvents(fd, EPOLLOUT, false);
	}

}
// should this be a utility ficntion jjust?
std::string toLower(const std::string& input) {
    std::string output = input;
    std::transform(output.begin(), output.end(), output.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return output;
}
//channel realted 

bool Server::channelExists(const std::string& channelName) const {
    return _channels.count(channelName) > 0;
}

void Server::createChannel(const std::string& channelName) {
    std::string lower =  toLower(channelName);
	if (_channels.count(lower) == 0){
		_channels.emplace(lower, std::make_shared<Channel>(channelName));
        std::cout << "Channel '" << channelName << "' created." << std::endl;
		return ;
    }
	// handle error
    std::cerr << "Error: Channel '" << channelName << "' already exists\n";
}

// oohlala check out this, ranged based iteration , no mroe this and that--->>
std::shared_ptr<Channel> Server::get_Channel(const std::string& ChannelName) {
	for (const auto& [name, channel] : _channels) {
    	if (name == ChannelName)
        	return channel;
	}
	throw ServerException(ErrorType::NO_CHANNEL_INMAP, "can not get_Channel()");
}

std::pair<MsgType, std::vector<std::string>> Server::validateChannelName(const std::string& channelName, const std::string& clientNick)
{
	if (channelName.empty()) {
		return {MsgType::NEED_MORE_PARAMS, {clientNick, "JOIN"}};
	}
	if (channelName.size() > config::MAX_LEN_CHANNAME) {
		return{MsgType::INVALID_CHANNEL_NAME, {clientNick, channelName, ":to many characters limit 25"}};
	}
	size_t illegalPos = channelName.find_first_of(IRCillegals::ForbiddenChannelChars);
    if (illegalPos != std::string::npos) {
        char badChar = channelName[illegalPos];
		return{MsgType::INVALID_CHANNEL_NAME, {clientNick, channelName, ":forbidden character = " + std::string(1, badChar)}};
	}
	return {MsgType::NONE, {}};

}
void Server::handleJoinChannel(const std::shared_ptr<Client>& client, std::vector<std::string> params)
{
	 if (!client || params.empty()){return;}
	const std::string& nickname = client->getNickname();
	std::vector<std::string> channels = splitCommaList(params[0]);
	std::vector<std::string> keys = (params.size() > 1) ? splitCommaList(params[1]) : std::vector<std::string>{};
    size_t keyIndex = 0;
    for (const std::string& chanNameRaw : channels) {
	    std::string key = (keyIndex < keys.size()) ? keys[keyIndex++] : "";
	    std::string lower = toLower(chanNameRaw);
		auto validation = validateChannelName(chanNameRaw, nickname);
        if (validation.first != MsgType::NONE) {
			broadcastMessage(MessageBuilder::generateMessage(validation.first, validation.second), client, nullptr, false, client);
			continue;
		}
		if (!channelExists(lower)) { 
			createChannel(chanNameRaw);
			client->setChannelCreator(true);
		}
		auto channel = get_Channel(lower);
		if (channel->isClientInChannel(nickname)) {continue;}
		auto canJoin = channel->canClientJoin(nickname, key);
		if (canJoin) {
			broadcastMessage(MessageBuilder::generateMessage(canJoin->first, canJoin->second),  nullptr, nullptr, false, client);
			continue ;
		}
		client->addChannel(lower, channel);
		if (channel->addClient(client) != 2) {return ;};		
		std::string ClientList = channel->getAllNicknames();
		std::vector<std::string> join = {client->getNickname(), client->getUsername(), channel->getName(), ClientList, channel->getTopic()};
		std::vector<std::string> updateChannel = {client->getNickname(), client->getUsername(), channel->getName()};
		broadcastMessage(MessageBuilder::generateMessage(MsgType::JOIN, join), client, nullptr, false, client);
		broadcastMessage(MessageBuilder::generateMessage(MsgType::UPDATE_CHAN, updateChannel),client, channel, true, nullptr);
	}
}


// todo be very sure this is handling property fd_to_nick and nick_to_fd. I am skipping over this and adding notes below...
void Server::handleNickCommand(const std::shared_ptr<Client>& client, std::map<int, std::string>& fd_to_nick, std::map<std::string, int>& nick_to_fd, const std::string& param) {
	if (!client->getHasSentNick()) {
		client->setHasSentNick();
		return;
	} if (client->getMsg().check_and_set_nickname(param, client->getFd(), fd_to_nick, nick_to_fd)) {
		std::string oldnick = client->getNickname();
		client->setNickname(param);
		client->getMsg().setType(MsgType::RPL_NICK_CHANGE, {oldnick, client->getUsername(), client->getNickname()});	
	} 
	std::string msg = MessageBuilder::generateMessage(client->getMsg().getActiveMessageType(),  client->getMsg().getMsgParams());
	broadcastMessage(msg, nullptr, nullptr, false, client);
	broadcastMessage(msg, client, nullptr, true, nullptr);
}

// AI suggested imporovement to Server::handleNickCommand
// // Assuming client->getMsg().check_and_set_nickname does the following internally:
// // 1. Checks if new_nick is valid and not in use.
// // 2. If valid and new:
// //    a. Removes old_nick from _nickname_to_fd (if old_nick existed).
// //    b. Inserts new_nick with client->getFd() into _nickname_to_fd.
// //    c. Updates client->setNickname(new_nick).
// // 3. Returns true if nickname was successfully set/changed, false otherwise.

// void Server::handleNickCommand(std::shared_ptr<Client> client, const std::string& param) { // Removed map parameters, as these should be member variables
//     // No explicit fd_to_nick and nick_to_fd parameters needed here if they are Server member variables.
//     // The Client's Msg object, or the Client itself, would then access these maps via reference/pointer
//     // passed in during initialization, or the Server would manage the maps directly.

//     // First NICK: If this is the client's initial NICK command (before full registration)
//     // You might set a flag like client->isAuthenticated() or client->getHasNickname()
//     // and process differently than a NICK change for an already registered user.
//     // The `client->getHasSentNick()` seems to handle this.
//     // If it's the *first* NICK, no old nickname to erase from maps.
//     // If it's a *change*, then handle old/new nicks.

//     std::string oldnick = client->getNickname(); // Get existing nickname (might be empty if initial NICK)
//     bool is_initial_nick = oldnick.empty(); // Simple check for initial NICK

//     // Assuming check_and_set_nickname handles the map updates:
//     // It should check param against existing _nickname_to_fd
//     // If new nickname is taken, it should return false.
//     // If valid and not taken:
//     //    If !is_initial_nick, it should erase oldnick from _nickname_to_fd.
//     //    It should insert param into _nickname_to_fd with client->getFd().
//     //    It should call client->setNickname(param).
//     if (client->getMsg().check_and_set_nickname(param, client->getFd(), _nickname_to_fd)) { // Pass Server's _nickname_to_fd
//         // If nickname changed successfully, and it was a change (not initial set)
//         // the client->setNickname(param) would have happened inside check_and_set_nickname.
//         // We now generate the NICK change message
//         std::string nick_change_msg;
//         if (is_initial_nick) {
//             // For initial NICK, typically no prefix or just server prefix is sent to client
//             // and the client is then prompted for USER if not sent yet.
//             // This part depends on your registration flow.
//             // For now, let's assume it proceeds to registration.
//             nick_change_msg = MessageBuilder::generateMessage(MsgType::RPL_NICK_CHANGE, {client->getNickname()}); // Simpler for initial nick
//         } else {
//             // For a NICK change for an already registered user, include old nick prefix.
//             std::string old_prefix = ":" + oldnick + "!" + client->getUsername() + "@" + client->getHostname();
//             nick_change_msg = old_prefix + " NICK :" + client->getNickname() + "\r\n"; // Manual building for NICK message
//         }

//         // Broadcast the NICK change message
//         // All clients (except the changing client) get the prefix with the old nick
//         // The changing client gets it too (or just their own NICK command back)
//         // Your broadcastMessage might need adjustments to handle NICK changes correctly
//         broadcastMessage(nick_change_msg, nullptr, nullptr, false, client); // All clients in same channels as 'client' + client itself
//         // A direct NICK message is usually broadcast to all channels the client is in.
//         // If your broadcastMessage handles that via client->getJoinedChannels(), then it's fine.
//     }
//     // else { // If check_and_set_nickname returned false, it handles sending ERR_NICKNAMEINUSE etc.
//     //    // So no explicit action here, just return.
//     // }
// }


// todo be very sure this is handling property fd_to_nick and nick_to_fd. I am skipping over this and adding notes below...
void Server::handleQuit(std::shared_ptr<Client> client) {
	if (!client) {
        std::cerr << "Server::handleQuit: Received null client pointer.\n";
        return;
    }
	std::string quit_message =  MessageBuilder::generateMessage(MsgType::CLIENT_QUIT, {client->getNickname(), client->getClientUname()});//client_prefix + " QUIT :Client disconnected\r\n"; // Default reason
	broadcastMessage(quit_message, client, nullptr, true, nullptr);
    client->setQuit();
    std::cout << "SERVER: Client " << client->getNickname() << " marked for disconnection, epollout will trigger removal.\n";
    // closure of the socket will happen in your main epoll loop's cleanup phase.
}



// AI suggested imporovement to Server::handleQuit
// void Server::handleQuit(std::shared_ptr<Client> client) {
//     if (!client) {
//         std::cerr << "Server::handleQuit: Received null client pointer.\n";
//         return;
//     }

//     std::string client_nickname = client->getNickname(); // Get nickname *before* any potential cleanup/setting to empty

//     // 1. Broadcast the QUIT message to all relevant channels/clients
//     // This message needs the full prefix of the quitting client.
//     // Example: :anonthe_luckyn!user@host QUIT :Client disconnected\r\n
//     std::string client_prefix = ":" + client_nickname + "!" + client->getUsername() + "@" + client->getHostname();
//     std::string quit_message = client_prefix + " QUIT :Client disconnected\r\n"; // This is the standard format

//     // You might want to get the specific quit reason if provided by the client, otherwise use a default.
//     // For now, "Client disconnected" is fine.

//     // Broadcast to relevant channels (channels the client was in)
//     // Your broadcastMessage might iterate through client->getJoinedChannels()
//     broadcastMessage(quit_message, client, nullptr, true, nullptr); // Assuming this broadcasts to channels client was in.

//     // 2. Remove client from _nickname_to_fd map
//     // This is the crucial step for consistency
//     _nickname_to_fd.erase(client_nickname); // Remove the nickname -> fd mapping

//     // 3. Remove client from all channels they were in
//     // You have client->getJoinedChannels(). Iterate through it and call channel->removeClient().
//     // You need to lock the weak_ptr<Channel> to get a shared_ptr to the Channel.
//     for (const auto& entry : client->getJoinedChannels()) {
//         if (std::shared_ptr<Channel> channel_sptr = entry.second.lock()) {
//             channel_sptr->removeClient(client_nickname); // Assuming removeClient takes nickname
//             // Check if channel is now empty and delete if needed
//             if (channel_sptr->isEmpty()) { // You'll need a Channel::isEmpty() method
//                 _channels.erase(channel_sptr->getName()); // Remove from Server's channel list
//                 std::cout << "SERVER: Channel " << channel_sptr->getName() << " is now empty and removed.\n";
//             }
//         }
//     }

//     // 4. Mark client for disconnection and send any final queued messages
//     updateEpollEvents(client->getFd(), EPOLLOUT, true); // Ensure all pending messages are sent
//     client->setQuit(); // Mark for removal in main loop

//     std::cout << "SERVER: Client " << client_nickname << " fully processed for disconnection.\n";
//     // Actual socket closure and removal from _Clients map happens in your main epoll loop after send/cleanup.
// }



void Server::handleModeCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params){
 	if (params.empty()) {
        client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NEED_MORE_PARAMS, {client->getNickname(), "MODE"}));
		updateEpollEvents(client->getFd(), EPOLLOUT, true);
		return;
    }
    std::string target = params[0];
    bool targetIsChannel = (target[0] == '#');
    if (targetIsChannel) {
        std::shared_ptr<Channel> channel;
        try {
            channel = get_Channel(toLower(target)); // This can throw ServerException(ErrorType::NO_CHANNEL_INMAP)
        } catch (const ServerException& e) {
            if (e.getType() == ErrorType::NO_CHANNEL_INMAP) {
                broadcastMessage(MessageBuilder::generateMessage(MsgType::NO_SUCH_CHANNEL, {target}), client, nullptr, false, client);
				return; // Abort: Channel does not exist
            }
		}
		std::pair<MsgType, std::vector<std::string>> validationResult = channel->initialModeValidation(client->getNickname(), params.size());
		if (validationResult.first != MsgType::NONE) {
			broadcastMessage(MessageBuilder::generateMessage(validationResult.first, validationResult.second),client, channel, false, nullptr);
			return; // Abort: Validation failed or command was for listing
        }
		validationResult = channel->modeSyntaxValidator(client->getNickname(), params);
		if (validationResult.first != MsgType::NONE) {
           broadcastMessage(MessageBuilder::generateMessage(validationResult.first, validationResult.second), client, nullptr, false, client);
			//updateEpollEvents(client->getFd(), EPOLLOUT, true);
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
			broadcastMessage(MessageBuilder::generateMessage(MsgType::CHANNEL_MODE_CHANGED, messageParams), client, channel, false, nullptr);
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

/**
 * @brief Modifies the event flags associated with a file descriptor (FD) in the epoll instance.
 * This function enables or disables a specific epoll event (e.g., EPOLLOUT) for a given FD without altering
 * other active flags. It checks whether the desired state is already set and avoids redundant syscalls.
 * The current event mask is preserved and updated in-place, toggling only the specified event.
 * 
 * @param fd The client’s file descriptor to update.
 * @param flag_to_toggle The epoll event flag to enable or disable (so far only EPOLLOUT).
 * @param enable Set to true to enable the flag; false to disable it.
 * 
 * @note Although currently used for EPOLLOUT, this function supports toggling any epoll event flag in epollEventMap.
 */

void Server::updateEpollEvents(int fd, uint32_t flag_to_toggle, bool enable) {
	auto it = _epollEventMap.find(fd);
    if (it == _epollEventMap.end()) {
        std::cerr << "ERROR: setEpollFlagStatus: FD " << fd << " not found in _epollEventMap. Cannot modify events. This might indicate a missing client cleanup.\n";
        return; // Exit immediately if FD is not tracked
    }
    uint32_t current_mask = it->second.events; // Get the currently stored mask
    uint32_t new_mask;
	if (enable) {
        new_mask = current_mask | flag_to_toggle; // Add the flag (turn it ON)
    } else {
        new_mask = current_mask & ~flag_to_toggle; // Remove the flag (turn it OFF)
    }
    // Optimization: Only call epoll_ctl if the mask has actually changed
    if (new_mask == current_mask) {
        return; // No change needed this checks if epollout was already triggered and avoids doing it again 
    }
    it->second.events = new_mask;
    // Now, tell the kernel about the updated event set, passing a pointer to the stored struct.
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, fd, &it->second) == -1) {
        perror("epoll_ctl EPOLL_CTL_MOD (setEpollFlagStatus) failed");
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
		broadcastMessage(MessageBuilder::generateMessage(MsgType::ERR_NOSUCHNICK, {requester_client->getNickname(), target_nick}), requester_client, nullptr, false, requester_client);
 //       std::string err_msg = ":" + _server_name + " 401 " + requester_client->getNickname() + " " + target_nick + " :No such nick/channel\r\n";
 //       requester_client->getMsg().queueMessage(err_msg);
 //       updateEpollEvents(requester_client->getFd(), EPOLLOUT, true);
        return; // Important: stop here if nick not found
    }

    // 3. Construct and queue WHOIS replies for the TARGET CLIENT

    // RPL_WHOISUSER (311)
	// Ensure target_client->getHostname() exists and returns the correct string
	// arg again, messagebuilder!"!!!!"
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

void Server::handlePartCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params) {
    if (!client) {
        std::cerr << "SERVER ERROR: handlePartCommand called with null client.\n";
        return;
    }
	 // Assumed to be lowercase
	const std::string& nickname = client->getNickname();
    if (params.empty()) {
        broadcastMessage(MessageBuilder::generateMessage(MsgType::NEED_MORE_PARAMS, {nickname, "PART"}), client, nullptr, false, client);
        return;
    }
    std::string part_reason = (params.size() > 1 && !params[1].empty()) ? params[1] : nickname;
	std::vector<std::string> channels_to_part = splitCommaList(params[0]);
    // 3. Process each channel
    for (const std::string& ch_name : channels_to_part) {
        // As per our latest understanding, we assume channel names are already lowercase
        // if your system mandates it for storage and lookups.
        // If not, you'd need a tolower transform here too.
        std::string lower_ch_name = toLower(ch_name); // Use this variable for map lookup if your map keys are lowercase.

        auto it = _channels.find(lower_ch_name);
        if (it == _channels.end()) {
            // ERR_NOSUCHCHANNEL (403)
	        broadcastMessage(MessageBuilder::generateMessage(MsgType::NO_SUCH_CHANNEL, {nickname, ch_name}), client, nullptr, false, client);
            continue; // Move to the next channel in the list
        }

        std::shared_ptr<Channel> channel_ptr = it->second;

        // Check if the client is actually in the channel
        // Your Channel::isClientInChannel *must* also rely on the assumption
        // that 'nickname' is lowercase and compare against lowercase nicknames in the channel.
        if (!channel_ptr->isClientInChannel(nickname)) {
	       	broadcastMessage(MessageBuilder::generateMessage(MsgType::NOT_ON_CHANNEL, {nickname, client->getUsername() ,channel_ptr->getName()}), client, nullptr, false, client);
            continue; // Move to the next channel
        }

        // --- Client is in the channel and valid, proceed with PART logic ---
       	broadcastMessage(MessageBuilder::generateMessage(MsgType::PART, {nickname, client->getUsername() ,channel_ptr->getName(), part_reason}), client, channel_ptr, false, nullptr);
        bool channel_is_empty = channel_ptr->removeClient(nickname);

        // 6. Remove channel from client's own list of joined channels (essential Client object cleanup)
        // This calls the Client::removeJoinedChannel we're about to define.
        client->removeJoinedChannel(lower_ch_name);

        // 7. If channel is now empty, delete the channel from the server's map
        if (channel_is_empty) {
            std::cout << "SERVER: Channel '" << lower_ch_name << "' is now empty. Deleting channel.\n";
            // Erasing the shared_ptr from the map will cause the Channel object
            // to be destructed (assuming no other shared_ptrs hold it).
            _channels.erase(it);
        }
    }
}

void Server::handleKickCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params) {
	std::cout << "SERVER: handleKickCommand entered for " << client->getNickname() << std::endl;
    if (!client) {
        std::cerr << "SERVER ERROR: handleKickCommand called with null client.\n";
        return;
    }

    const std::string& kicker_nickname = client->getNickname(); // The client sending the KICK command
    std::string client_prefix = ":" + kicker_nickname + "!" + client->getUsername() + "@" + client->getHostname();

    // 1. Parameter Check: KICK requires at least 2 parameters (channel, target_nick)
    // Format: KICK <channel> <user> [<reason>]
    if (params.size() < 2) {
		std::cout << "SERVER: KICK - Not enough params from " << kicker_nickname << std::endl;
        broadcastMessage(MessageBuilder::generateMessage(MsgType::NEED_MORE_PARAMS, {kicker_nickname, "KICK"}), client, nullptr, false, client);
        return;
    }

    std::string channel_name = params[0];
	std::string channel_name_lower = toLower(channel_name);
    std::string target_nickname = params[1];
    std::string kick_reason = (params.size() > 2) ? params[2] : kicker_nickname; // Default reason is kicker's nick

    // Ensure channel_name is in the correct case for lookup (your server's invariant)
    // If your server assumes ALL channel names are stored and processed lowercase:
    // std::transform(channel_name.begin(), channel_name.end(), channel_name.begin(), ::tolower); // Uncomment if needed

    // Ensure target_nickname is in the correct case for lookup (your server's invariant)
    // std::transform(target_nickname.begin(), target_nickname.end(), target_nickname.begin(), ::tolower); // Uncomment if needed


    // 2. Channel Existence Check
    auto channel_it = _channels.find(channel_name_lower); // Assuming channel_name is already canonical (e.g., lowercase)
    if (channel_it == _channels.end()) {
        // ERR_NOSUCHCHANNEL (403)
        broadcastMessage(MessageBuilder::generateMessage(MsgType::NO_SUCH_CHANNEL, {kicker_nickname, channel_name}), client, nullptr, false, client);
        return;
    }

    std::shared_ptr<Channel> channel_ptr = channel_it->second;

    // 3. Is the kicker (client) on the channel? (ERR_NOTONCHANNEL)
    if (!channel_ptr->isClientInChannel(kicker_nickname)) {
		broadcastMessage(MessageBuilder::generateMessage(MsgType::NOT_ON_CHANNEL, {kicker_nickname, channel_ptr->getName()}), client, nullptr, false, client);
        std::cout << "SERVER: KICK - Kicker '" << kicker_nickname << "' not on channel '" << channel_name << "'.\n";
        return;
    }

    // 4. Does the kicker have operator privileges on the channel? (ERR_CHANOPRIVSNEEDED)
    // You'll need to implement this function in your Channel class. //noit needed as shown by example below
    if (!channel_ptr->getClientModes(client->getNickname()).test(Modes::OPERATOR)) {
        broadcastMessage(MessageBuilder::generateMessage(MsgType::NOT_OPERATOR, {kicker_nickname, channel_ptr->getName()}), client, nullptr, false, client);
        std::cout << "SERVER: KICK - Kicker '" << kicker_nickname << "' not operator on channel '" << channel_name << "'.\n";
        return;
    }

    // 5. Get shared_ptr to target client & check if target is in channel.
    // First, find the target client's shared_ptr from the server's global client list.
    // You will need a method in Server to get a client by nickname.
    // Example: std::shared_ptr<Client> Server::getClientByNickname(const std::string& nickname);
    std::shared_ptr<Client> target_client_ptr = getClientByNickname(target_nickname);

    if (!target_client_ptr) { // Target client not even connected to the server
        broadcastMessage(MessageBuilder::generateMessage(MsgType::ERR_NOSUCHNICK, {kicker_nickname, target_nickname}), client, nullptr, false, client);
        return;
    }

    // Check if the target is on the specified channel (ERR_USERNOTINCHANNEL)
    if (!channel_ptr->isClientInChannel(target_nickname)) {
        client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::ERR_USERNOTINCHANNEL, {kicker_nickname, target_nickname, channel_name}));
        updateEpollEvents(client->getFd(), EPOLLOUT, true);
        std::cout << "SERVER: KICK - Target '" << target_nickname << "' not on channel '" << channel_name << "'.\n";
        return;
    }

    // --- If we reach here, all validations have passed! ---
    // Now proceed to actual kicking logic (Phase 3)
	broadcastMessage(MessageBuilder::generateMessage(MsgType::KICK, {client->getNickname(), client->getUsername(), channel_name, target_nickname, kick_reason}), client, channel_ptr, false, nullptr);
    // 8. Remove target client from channel's internal data structures (e.g., _ClientModes map)
    channel_ptr->removeClientByNickname(target_nickname); // Needs to be implemented in Channel

    // 9. Remove channel from target client's _joinedChannels list
    target_client_ptr->removeJoinedChannel(channel_name_lower); // Needs to be implemented in Client

    // 10. Check if channel is empty, delete if so.
    if (channel_ptr->isEmpty()) { // Needs to be implemented in Channel
        _channels.erase(channel_name_lower); // Remove the channel from Server's map
        std::cout << "SERVER: Channel '" << channel_name << "' is now empty and has been removed.\n";
    }

    std::cout << "SERVER: KICK command successful. '" << target_nickname
              << "' kicked from '" << channel_name << "' by '" << client->getNickname()
              << "' with reason: '" << kick_reason << "'\n";
}


std::shared_ptr<Client> Server::getClientByNickname(const std::string& nickname){
    // std::cout << "SERVER: Looking up FD for nickname: '" << nickname << "' (case-sensitive) in _nickname_to_fd map\n"; // Uncomment for debugging

    // Step 1: Look up the file descriptor using the nickname
    auto fd_it = _nickname_to_fd.find(nickname);

    if (fd_it == _nickname_to_fd.end()) {
        // std::cout << "SERVER: Nickname '" << nickname << "' not found in _nickname_to_fd.\n"; // Uncomment for debugging
        return nullptr; // Nickname not found, so no such client
    }

    int client_fd = fd_it->second; // Get the file descriptor

    // Step 2: Use the file descriptor to get the shared_ptr<Client> from _Clients
    // std::cout << "SERVER: Found FD " << client_fd << " for nickname '" << nickname << "'. Now looking up in _Clients.\n"; // Uncomment for debugging

    auto client_it = _Clients.find(client_fd);

    if (client_it == _Clients.end()) {
        // This case should ideally not happen if _nickname_to_fd is kept consistent with _Clients.
        // It indicates an inconsistency if a FD is in _nickname_to_fd but not in _Clients.
        std::cerr << "SERVER ERROR: Found FD " << client_fd << " for nickname '" << nickname
                  << "' in _nickname_to_fd, but no client found in _Clients map for this FD.\n";
        return nullptr;
    }

    // std::cout << "SERVER: Client shared_ptr found for nickname '" << nickname << "'.\n"; // Uncomment for debugging
    return client_it->second; // Return the shared_ptr to the Client
}

void Server::handleTopicCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params) {
    std::cout << "SERVER: handleTopicCommand entered for " << client->getNickname() << std::endl;

    if (!client) {
        std::cerr << "SERVER ERROR: handleTopicCommand called with null client.\n";
        return;
    }

    const std::string& sender_nickname = client->getNickname();

    // TOPIC <channel> [<topic>]
    // Params:
    // params[0] = channel name
    // params[1] = new topic (optional)

    if (params.empty()) {
        // ERR_NEEDMOREPARAMS (461) if no channel is provided
        client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NEED_MORE_PARAMS, {sender_nickname, "TOPIC"}));
        updateEpollEvents(client->getFd(), EPOLLOUT, true);
        std::cout << "SERVER: TOPIC - Not enough params from " << sender_nickname << std::endl;
        return;
    }

    std::string channel_name = params[0];

    // 1. Channel Existence Check
    auto channel_it = _channels.find(channel_name);
    if (channel_it == _channels.end()) {
        // ERR_NOSUCHCHANNEL (403)
        client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NO_SUCH_CHANNEL, {sender_nickname, channel_name}));
        updateEpollEvents(client->getFd(), EPOLLOUT, true);
        std::cout << "SERVER: TOPIC - Channel '" << channel_name << "' not found.\n";
        return;
    }

    std::shared_ptr<Channel> channel_ptr = channel_it->second;

    // 2. Client In Channel Check
    if (!channel_ptr->isClientInChannel(sender_nickname)) {
        // ERR_NOTONCHANNEL (442)
        client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NOT_ON_CHANNEL, {sender_nickname, channel_name}));
        updateEpollEvents(client->getFd(), EPOLLOUT, true);
        std::cout << "SERVER: TOPIC - Client '" << sender_nickname << "' not on channel '" << channel_name << "'.\n";
        return;
    }

    // --- Decision Point: View Topic or Set Topic ---

    if (params.size() == 1) {
        // Client wants to VIEW the topic (TOPIC #channel)
        std::string current_topic = channel_ptr->getTopic();
        if (current_topic.empty()) {
            // RPL_NOTOPIC (331) - No topic is set
            client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::RPL_NOTOPIC, {sender_nickname, channel_name}));
            std::cout << "SERVER: TOPIC - No topic set for '" << channel_name << "'.\n";
        } else {
            // RPL_TOPIC (332) - Send the current topic
            client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::RPL_TOPIC, {sender_nickname, channel_name, current_topic}));
            std::cout << "SERVER: TOPIC - Sent topic '" << current_topic << "' for '" << channel_name << "'.\n";

            // RPL_TOPICWHOTIME (333) - Who set the topic and when
            // For this, your Channel class needs _topicSetter (std::string) and _topicSetTime (std::time_t)
            // If you have these:
            // std::string setter_info = channel_ptr->getTopicSetter() + " " + std::to_string(channel_ptr->getTopicSetTime());
            // client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::RPL_TOPICWHOTIME, {sender_nickname, channel_name, setter_info}));
        }
        updateEpollEvents(client->getFd(), EPOLLOUT, true);
        return; // Done with viewing topic
    }

    // --- If we reach here, params.size() > 1, so client wants to SET the topic ---
    std::string new_topic_content = params[1]; // The topic string starts at params[1]

    // You need to handle multi-word topics correctly. If the topic has spaces, it should
    // be everything after the first space after the channel name.
    // For example, if params[1] is ":This is my topic", the colon indicates the rest is the topic.
    // Your command parser should ideally handle this and give you a single string for the topic.
    // Assuming params[1] now holds the full topic string, including potentially initial colon.
    if (!new_topic_content.empty() && new_topic_content[0] == ':') {
        new_topic_content = new_topic_content.substr(1); // Remove leading colon if present
    }

    // 3. Topic Privilege Check (based on channel mode +t)
    // You need a way to get channel modes. Assuming Channel has a getMode() or isModeSet('t').
    // Let's assume you have Channel::isModeSet(char mode_char) or similar.
    // For now, we'll assume a default behavior if modes aren't implemented yet.
    bool topic_is_protected = true; // Default to true if you don't have modes yet

    // If you have channel modes, replace 'true' with a check like channel_ptr->isModeSet('t')
    // e.g., if (channel_ptr->getMode() & TOPIC_PROTECTED_BIT) { topic_is_protected = true; }
    // Or if you have a separate boolean flag: if (channel_ptr->isTopicProtected()) { topic_is_protected = true; }

    if (topic_is_protected && !channel_ptr->isClientOperator(sender_nickname)) {
        // ERR_CHANOPRIVSNEEDED (482)
        client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NOT_OPERATOR, {sender_nickname, channel_name}));
        updateEpollEvents(client->getFd(), EPOLLOUT, true);
        std::cout << "SERVER: TOPIC - Client '" << sender_nickname << "' not operator on channel '" << channel_name << "' with +t mode.\n";
        return;
    }

    // --- All checks passed for setting the topic ---

    // Set the new topic
    channel_ptr->setTopic(new_topic_content);
    std::cout << "SERVER: TOPIC - Set topic for '" << channel_name << "' to: '" << new_topic_content << "'.\n";

    // Store who set the topic and when (for RPL_TOPICWHOTIME)
    // You need to add _topicSetter and _topicSetTime to your Channel class
    // channel_ptr->setTopicSetter(sender_nickname);
    // channel_ptr->setTopicSetTime(std::time(NULL)); // Current Unix timestamp

    // 4. Broadcast Topic Change to all channel members
    // Format: :<sender_nick>!<sender_user>@<sender_host> TOPIC <channel> :<new_topic>
    std::string sender_prefix = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname();
    std::string topic_change_message = sender_prefix + " TOPIC " + channel_name + " :" + new_topic_content + "\r\n";

    // Use the versatile broadcastMessage function!
    broadcastMessage(topic_change_message, nullptr, channel_ptr, false, nullptr); // Broadcast to all in channel

    std::cout << "SERVER: TOPIC command successful. Topic for '" << channel_name
              << "' changed by '" << sender_nickname << "' to: '" << new_topic_content << "'\n";
}


void Server::handleInviteCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params) {
    std::cout << "SERVER: handleInviteCommand entered for " << client->getNickname() << std::endl;

    if (!client) {
        std::cerr << "SERVER ERROR: handleInviteCommand called with null client.\n";
        return;
    }

    const std::string& sender_nickname = client->getNickname();

    // INVITE <nickname> <channel>
    // Params:
    // params[0] = target nickname
    // params[1] = channel name

    // 1. Check for correct number of parameters
    if (params.size() < 2) {
        // ERR_NEEDMOREPARAMS (461)
        client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NEED_MORE_PARAMS, {sender_nickname, "INVITE"}));
        updateEpollEvents(client->getFd(), EPOLLOUT, true);
        std::cout << "SERVER: INVITE - Not enough parameters from " << sender_nickname << std::endl;
        return;
    }

    std::string target_nickname = params[0];
    std::string channel_name = params[1];

    // 2. Check if target nickname exists (is connected to the server)
    std::shared_ptr<Client> target_client_ptr = getClientByNickname(target_nickname); // Assuming you have getClientByNickname
    if (!target_client_ptr) {
        // ERR_NOSUCHNICK (401)
        broadcastMessage(MessageBuilder::generateMessage(MsgType::ERR_NOSUCHNICK, {sender_nickname, target_nickname}), client, nullptr, false, client);
        std::cout << "SERVER: INVITE - Target nickname '" << target_nickname << "' not found.\n";
        return;
    }

    // 3. Check if channel exists
    auto channel_it = _channels.find(channel_name);
    if (channel_it == _channels.end()) {
        // ERR_NOSUCHCHANNEL (403)
        client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NO_SUCH_CHANNEL, {sender_nickname, channel_name}));
        updateEpollEvents(client->getFd(), EPOLLOUT, true);
        std::cout << "SERVER: INVITE - Channel '" << channel_name << "' not found.\n";
        return;
    }
    std::shared_ptr<Channel> channel_ptr = channel_it->second;

    // 4. Check if sender is on the channel
    if (!channel_ptr->isClientInChannel(sender_nickname)) {
        // ERR_NOTONCHANNEL (442)
        client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NOT_ON_CHANNEL, {sender_nickname, channel_name}));
        updateEpollEvents(client->getFd(), EPOLLOUT, true);
        std::cout << "SERVER: INVITE - Sender '" << sender_nickname << "' not on channel '" << channel_name << "'.\n";
        return;
    }

    // 5. Check if target client is already on the channel
    if (channel_ptr->isClientInChannel(target_nickname)) {
        // ERR_USERONCHANNEL (443)
        client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::ERR_USERONCHANNEL, {sender_nickname, target_nickname, channel_name}));
        updateEpollEvents(client->getFd(), EPOLLOUT, true);
        std::cout << "SERVER: INVITE - Target client '" << target_nickname << "' is already on channel '" << channel_name << "'.\n";
        return;
    }

    // --- Privilege Check for Invite-Only Channel (+i mode) ---
    // If the channel is invite-only (+i mode), only channel operators can send invites.
    // Assuming you have Channel::isModeSet('i') or similar.
    bool channel_is_invite_only = false; // Default to false if modes not fully implemented yet

    // Replace 'false' with actual mode check once you have it, e.g.:
    // if (channel_ptr->isModeSet('i')) { channel_is_invite_only = true; }

    if (channel_is_invite_only && !channel_ptr->isClientOperator(sender_nickname)) {
        // ERR_CHANOPRIVSNEEDED (482)
        client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NOT_OPERATOR, {sender_nickname, channel_name}));
        updateEpollEvents(client->getFd(), EPOLLOUT, true);
        std::cout << "SERVER: INVITE - Sender '" << sender_nickname << "' is not operator on invite-only channel '" << channel_name << "'.\n";
        return;
    }

    // --- All checks passed: Perform the Invite! ---

    // 6. Add target client to channel's invite list
    // You have std::deque<std::string> _invites; in your Channel class.
    // You'll need a method to add a nickname to this deque, and check if it's already there.
    // Let's assume you'll add Channel::addInvite(const std::string& nickname);
    channel_ptr->addInvite(target_nickname); // You'll need to implement this in Channel.cpp and declare in Channel.hpp
    std::cout << "SERVER: INVITE - Added '" << target_nickname << "' to invite list for channel '" << channel_name << "'.\n";


    // 7. Send RPL_INVITING (341) to the sender
    client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::RPL_INVITING, {sender_nickname, target_nickname, channel_name}));
    updateEpollEvents(client->getFd(), EPOLLOUT, true);
    std::cout << "SERVER: INVITE - Sent RPL_INVITING to " << sender_nickname << std::endl;

    // 8. Send INVITE message to the target client
    // Format: :<sender_nick>!<sender_user>@<sender_host> INVITE <target_nick> :<channel>
    std::string sender_prefix = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname();
    std::string invite_message = sender_prefix + " INVITE " + target_nickname + " :" + channel_name + "\r\n";

    // Use your broadcastMessage, targeting a single recipient
    broadcastMessage(invite_message, nullptr, nullptr, false, target_client_ptr);
    std::cout << "SERVER: INVITE - Sent INVITE message to target client " << target_nickname << std::endl;

    std::cout << "SERVER: INVITE command successful. '" << target_nickname << "' invited to '" << channel_name << "' by '" << sender_nickname << "'.\n";
}
