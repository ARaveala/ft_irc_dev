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
std::string toLower(const std::string& input) {
    std::string output = input;
    std::transform(output.begin(), output.end(), output.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return output;
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

bool Server::isTimerFd(int fd){
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

// validators these are tasked with a simple job, it helps prevent repative code all over the files
bool Server::validateChannelExists(const std::shared_ptr<Client>& client, const std::string& channel_name, const std::string& sender_nickname) {
 if(!channelExists(toLower(channel_name))) {
		broadcastMessage(MessageBuilder::generateMessage(MsgType::NO_SUCH_CHANNEL, {sender_nickname, channel_name}), client, nullptr, false, client);
        return false;
    }
	return true;
}

bool Server::validateIsClientInChannel(const std::shared_ptr<Channel> channel, const std::shared_ptr<Client>& client, const std::string& channel_name, const std::string& nickname){
	std::cout<<"DEBUGGIN:: VALIDATE CLIENT IN CHANNEL what is nickn name"<<nickname<<"\n";
	if (!channel->isClientInChannel(nickname)) {
        broadcastMessage(MessageBuilder::generateMessage(MsgType::NOT_ON_CHANNEL, {nickname, channel_name}), client, nullptr, false, client);
        return false;
    }
	return true;
}

bool Server::validateTargetInChannel(const std::shared_ptr<Channel> channel, const std::shared_ptr<Client>& client, const std::string& channel_name, const std::string& target_nickname) {
	if (channel->isClientInChannel(target_nickname)) {
        broadcastMessage(MessageBuilder::generateMessage(MsgType::ERR_USERONCHANNEL, {client->getNickname(), target_nickname, channel_name}), client, nullptr, false, client);
		return false;
	}
	return true;
}

bool Server::validateTargetExists(const std::shared_ptr<Client>& client, const std::shared_ptr<Client>& target, const std::string& sender_nickname, const std::string& target_nickname) {
	if (!target) {
        broadcastMessage(MessageBuilder::generateMessage(MsgType::ERR_NOSUCHNICK, {sender_nickname, target_nickname}), client, nullptr, false, client); //wrong way round?
        return false;
    }
	return true;
}


bool Server::validateModes(const std::shared_ptr<Channel> channel, const std::shared_ptr<Client>& client, Modes::ChannelMode comp) {
	
	if (comp == Modes::NONE && !channel->getClientModes(client->getNickname()).test(Modes::OPERATOR))	{
        broadcastMessage(MessageBuilder::generateMessage(MsgType::NOT_OPERATOR, {client->getNickname(), channel->getName()}), client, nullptr, false, client);
		return false;
	}
	if (channel->CheckChannelMode(comp) && !channel->getClientModes(client->getNickname()).test(Modes::OPERATOR)) {
        broadcastMessage(MessageBuilder::generateMessage(MsgType::NOT_OPERATOR, {client->getNickname(), channel->getName()}), client, nullptr, false, client);
        return false;
    }
	return true;
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
	setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &flag, optlen);
	make_socket_unblocking(client_fd);
	setup_epoll(epollfd, client_fd, EPOLLIN | EPOLLOUT | EPOLLET);
	int timer_fd = setup_epoll_timer(epollfd, config::TIMEOUT_CLIENT);
	if (timer_fd == -1){
		epoll_ctl(epollfd, EPOLL_CTL_DEL, client_fd, nullptr);  // Optional, but clean
    	close(client_fd);
		throw ServerException(ErrorType::ACCEPT_FAILURE, "debuggin: create Client");
	}
	_Clients[client_fd] = std::make_shared<Client>(client_fd, timer_fd);
	std::shared_ptr<Client> client = _Clients[client_fd];
	_timer_map[timer_fd] = client_fd;
	set_current_client_in_progress(client_fd);
	client->setDefaults();
	if (!client->get_acknowledged()){			
		client->getMsg().queueMessage(MessageBuilder::generateInitMsg());
		set_client_count(1);		
	}
	std::cout << "New Client created , fd value is  == " << client_fd << std::endl;
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
			std::pair<MsgType, std::vector<std::string>> result = channel_ptr->promoteFallbackOperator(client_to_remove, true);
			//VALIDATE
			if (result.first != MsgType::NONE){
	    		//std::cout<<"DEBUGGIN::: breoadcasting message of new operator\n";
				broadcastMessage(MessageBuilder::generateMessage(result.first, result.second), client_to_remove, channel_ptr, true, nullptr);
			}
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
            bool channel_became_empty = channel_ptr->removeClientByNickname(client_to_remove->getNickname());
			//NEWNEWbool channel_became_empty = channel_ptr->removeClient(client_to_remove->getNickname());

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

void Server::closeAndResetClient() {
    if (_current_client_in_progress > 0) {
        close(_current_client_in_progress);
        _current_client_in_progress = 0;
    }
}

// ERROR HANDLING INSIDE LOOP
void Server::handle_client_connection_error(ErrorType err_type) {
	switch (err_type){
		case ErrorType::ACCEPT_FAILURE:
			break;
		case ErrorType::EPOLL_FAILURE_1: {
			closeAndResetClient();
			break;
		} case ErrorType::SOCKET_FAILURE: {
			closeAndResetClient();
			break;
		} default:
			closeAndResetClient();
			std::cerr << "server Unknown error occurred" << std::endl;
			break;
	}
}

void Server::handleReadEvent(const std::shared_ptr<Client>& client, int client_fd) { // send client as aparam const ref
    if (!client) { return; }
	set_current_client_in_progress(client_fd);
    char buffer[config::BUFFER_SIZE];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0); // Server does the recv, ONCE per event!
	std::cout << "Debug - Raw Buffer Data: [" << std::string(buffer, bytes_read) << "]" << std::endl;
    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) { return; }
        perror("recv failed in handleReadEvent"); // error
		remove_Client(client_fd);
		return;
    }
    if (bytes_read == 0) {
        std::cout << "Client FD " << client_fd << " disconnected gracefully (recv returned 0).\n";
        remove_Client(client_fd);
		//client->setQuit();
		//updateEpollEvents();
		//handleQuit(client);
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
	broadcastMessage(MessageBuilder::bildPing(), nullptr, nullptr, false, get_Client(fd));
	resetClientFullTimer(1, clientit->second);
	return false;
}

void Server::resetClientFullTimer(int resetVal, std::shared_ptr<Client> client){
	if (!client) {
		return; //thow?
	}
	int client_fd = client->getFd();
	int found_timer_fd = 0;
	client->set_failed_response_counter(resetVal);
	for (const auto& [timer_fd, mapped_client_fd] : _timer_map) {
        if (mapped_client_fd == client_fd)
            found_timer_fd = timer_fd;
    }
	if (found_timer_fd == 0){
    	throw std::runtime_error("No timer found for client_fd " + std::to_string(client_fd));
	}
	resetClientTimer(found_timer_fd, config::TIMEOUT_CLIENT);
}

#include <iomanip>
void Server::send_message(const std::shared_ptr<Client>& client)
{
	int fd;
	fd = client->getFd();
	set_current_client_in_progress(fd);
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
            // !!!Call remove client function here !!! or quit = true?
            return;
        }
		//std::cout << "DEBUG: Before advanceCurrentMessageOffset: " << _bytesSentForCurrentMessage << std::endl;
		client->getMsg().advanceCurrentMessageOffset(bytes_sent); 
		if (client->getMsg().getRemainingBytesInCurrentMessage() == 0) {
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
	// todo handle error
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
		std::cout<<"DEBUGGIN HANDLE JOIN EDITION :: before validate channel , name of the channel evaluated = "<<lower<<"\n";

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
void Server::updateNickname(const std::shared_ptr<Client>& client, const std::string& newNick, const std::string& oldNick) {
    const int& fd = client->getFd();
    std::string old_lc = toLower(oldNick);
    std::string new_lc = toLower(newNick);

    std::cout << "=== [NICK UPDATE] FD: " << fd << " | Old: '" << oldNick << "' → New: '" << newNick << "'\n";
    std::cout << "    Cleaning up old entry: '" << old_lc << "'\n";
    _nickname_to_fd.erase(old_lc);

    client->setNickname(newNick);

    std::cout << "    Inserting new entry: '" << new_lc << "' for FD: " << fd << "\n";
    _nickname_to_fd[new_lc] = fd;

    std::cout << "    ✅ Nickname update complete.\n";
}


// todo be very sure this is handling property fd_to_nick and nick_to_fd.
void Server::handleNickCommand(const std::shared_ptr<Client>& client, std::map<std::string, int>& nick_to_fd, const std::string& param) {
	// new function here , can use it all kinds of ways, this will help prevent segv on early attemps to chnage things before registartion
	auto now = std::chrono::steady_clock::now();
	if (now - client->getRegisteredAt() < std::chrono::seconds(10)) {
		    client->getMsg().queueMessage(":localhost 439 "+client->getNickname()+" :Please wait a moment before changing nick\r\n");
			updateEpollEvents(client->getFd(), EPOLLOUT, true);
		    return;
	}
	// willl be seperated
	MsgType type= client->getMsg().check_nickname(param, client->getFd(), nick_to_fd); 
	if ( type == MsgType::RPL_NICK_CHANGE) {
		const std::string& oldnick = client->getNickname();
		updateNickname(client, param, oldnick);
		if (!client->getHasSentNick()) {
			client->setHasSentNick();
			tryRegisterClient(client);
		} else {
			broadcastMessage(MessageBuilder::generateMessage(MsgType::RPL_NICK_CHANGE, {oldnick, client->getUsername(), client->getNickname()}), client, nullptr, false, nullptr);
		}
	} else if (type != MsgType::NONE){
		if (type == MsgType::NICKNAME_IN_USE && !client->getHasSentNick()){
			updateNickname(client, generateUniqueNickname(nick_to_fd), " ");
			client->setHasSentNick();
			tryRegisterClient(client);
			broadcastMessage(MessageBuilder::generateMessage(MsgType::RPL_NICK_CHANGE, {param, client->getUsername(), client->getNickname()}), nullptr, nullptr, false, client);
			return;
		}
		if (type == MsgType::NEED_MORE_PARAMS){
			broadcastMessage(MessageBuilder::generateMessage(type, {client->getNickname(), "NICK"}), nullptr, nullptr, false, client);
		}
		broadcastMessage(MessageBuilder::generateMessage(type, {client->getNickname(), param}), nullptr, nullptr, false, client);
	}
}

// todo be very sure this is handling property fd_to_nick and nick_to_fd.
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


void Server::handleModeCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params){
 	/*if (params.empty()) {
        client->getMsg().queueMessage(MessageBuilder::generateMessage(MsgType::NEED_MORE_PARAMS, {client->getNickname(), "MODE"}));
		updateEpollEvents(client->getFd(), EPOLLOUT, true);
		return;
    }*/
    std::string target = params[0];
    bool targetIsChannel = (target[0] == '#');
    if (targetIsChannel) {
        std::shared_ptr<Channel> channel;
        try {
            channel = get_Channel(toLower(target));
        } catch (const ServerException& e) {
            if (e.getType() == ErrorType::NO_CHANNEL_INMAP) {
                broadcastMessage(MessageBuilder::generateMessage(MsgType::NO_SUCH_CHANNEL, {target}), client, nullptr, false, client);
				return;
            }
		}
		std::pair<MsgType, std::vector<std::string>> validationResult = channel->initialModeValidation(client->getNickname(), params.size());
		if (validationResult.first != MsgType::NONE) {
			broadcastMessage(MessageBuilder::generateMessage(validationResult.first, validationResult.second),nullptr, nullptr, false, client);
			return;
        }
		validationResult = channel->modeSyntaxValidator(client->getNickname(), params);
		if (validationResult.first != MsgType::NONE) {
           broadcastMessage(MessageBuilder::generateMessage(validationResult.first, validationResult.second), nullptr, nullptr, false, client);
			return;
    	}
		std::vector<std::string> modeparams = channel->applymodes(params);
		std::vector<std::string> messageParams;
		if (!modeparams.empty())
		{
			messageParams.push_back(client->getNickname());
			messageParams.push_back(client->getClientUname());
			messageParams.push_back(channel->getName());
			//std::cout<<"DEBUGGUS::Assus:: what the shiz show me params 1"<<params[1]<<"\n";
			//std::cout<<"DEBUGGUS::Assus:: what the shiz show me params 1"<<modeparams[1]<<"\n";
			if (!modeparams[0].empty())
				messageParams.push_back(modeparams[0]);
			//if(!modeparams[1].empty())
			//	messageParams.push_back(modeparams[1]);
			if (params.size() == 3)
				messageParams.push_back(params[2]);
			else
				messageParams.push_back("");
			broadcastMessage(MessageBuilder::generateMessage(MsgType::CHANNEL_MODE_CHANGED, messageParams), client, channel, false, nullptr);
			if (channel->getwasOpRemoved()) {
				std::pair<MsgType, std::vector<std::string>> result = channel->promoteFallbackOperator(client, false);
				if (result.first != MsgType::NONE){
					channel->setwasOpRemoved();
					broadcastMessage(MessageBuilder::generateMessage(MsgType::CHANNEL_MODE_CHANGED, result.second), client, channel, false, nullptr);
				}
			}
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
		// todo fix so either + or - and we dont accept any others
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
		(void)nickname;
        que.push_back(":localhost CAP * ACK :multi-prefix\r\n");
		que.push_back(":localhost CAP * ACK :END\r\n");
		capSent = true;
}

void Server::handlePing(const std::shared_ptr<Client>& client){
	broadcastMessage(MessageBuilder::bildPong(), client, nullptr, false, client);
	resetClientFullTimer(0, client);

}
void Server::handlePong(const std::shared_ptr<Client>& client){ // first could be nullptr?
	resetClientFullTimer(0, client);
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
    if (new_mask == current_mask) {return; }// No change needed this checks if epollout was already triggered and avoids doing it again 
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

std::string generateUniqueNickname(const std::map<std::string, int>& nickname_to_fd) {
    std::vector<std::string> adjectives = {
        "the_beerdrinking", "the_brave", "the_evil", "the_curious", "the_wild",
        "the_boring", "the_silent", "the_swift", "the_mystic", "the_slow", "the_lucky"
    };
	std::string nickname;
	int randomNum = 0;
    do {
		randomNum = rand() % 100;
        nickname = "anon_" + adjectives[rand() % adjectives.size()] + "_" + std::to_string(randomNum);//static_cast<char>('a' + rand() % 26); // maybe a number, or 3-letter code
    } while (nickname_to_fd.count(toLower(nickname)) > 0); // case-insensitive nick collision check
    return nickname;
}
	// while produced name is not in nick name to fd map
    //std::string newNick = "anon" + adjectives[rand() % adjectives.size()] + static_cast<char>('a' + rand() % 26);
	
   /* std::ofstream configFile("config");  // ✅ Opens the file for writing
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
}*/

void Server::handleWhoIs(std::shared_ptr<Client> requester_client, std::string target_nick) {
    // 1. Find the target client
    std::shared_ptr<Client> target_client = getClientByNickname(target_nick); // tolower?

    // 2. Handle Nick Not Found (ERR_NOSUCHNICK)
    if (!validateTargetExists(requester_client, target_client, requester_client->getNickname() , target_nick)) {return ;}
	/*if (!target_client) {
		broadcastMessage(MessageBuilder::generateMessage(MsgType::ERR_NOSUCHNICK, {requester_client->getNickname(), target_nick}), requester_client, nullptr, false, requester_client);
        return; // Important: stop here if nick not found
    }*/

    // 3. Construct and queue WHOIS replies for the TARGET CLIENT

    // RPL_WHOISUSER (311)
	// Ensure target_client->getHostname() exists and returns the correct string
	// arg again, messagebuilder!"!!!!"
// add this to messageBuilder
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
	        broadcastMessage(MessageBuilder::generateMessage(MsgType::NO_SUCH_CHANNEL, {nickname, ch_name}), client, nullptr, false, client);
            continue; // Move to the next channel in the list
        }
        std::shared_ptr<Channel> channel_ptr = it->second;
        // Check if the client is actually in the channel
        // Your Channel::isClientInChannel *must* also rely on the assumption
        // that 'nickname' is lowercase and compare against lowercase nicknames in the channel.
        if (!validateIsClientInChannel (channel_ptr, client, lower_ch_name, nickname)) { return ;}		
		broadcastMessage(MessageBuilder::generateMessage(MsgType::PART, {nickname, client->getUsername() ,channel_ptr->getName(), part_reason}), nullptr, channel_ptr, false, nullptr);
		std::pair<MsgType, std::vector<std::string>> result = channel_ptr->promoteFallbackOperator(client, true);
		//VALIDATE
		if (result.first != MsgType::NONE){
	       	//std::cout<<"DEBUGGIN::: breoadcasting message of new operator\n";
			broadcastMessage(MessageBuilder::generateMessage(result.first, result.second), client, channel_ptr, true, nullptr);
		}
		bool channel_is_empty = channel_ptr->removeClientByNickname(nickname);
        client->removeJoinedChannel(lower_ch_name);
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

    const std::string& kicker_nickname = client->getNickname();
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
    if (!validateChannelExists(client, channel_name_lower, kicker_nickname)) { return;}
    std::shared_ptr<Channel> channel_ptr = get_Channel(channel_name_lower);
    if (!validateIsClientInChannel (channel_ptr, client, channel_name, kicker_nickname)) { return ;}
	if (!validateModes(channel_ptr, client, Modes::NONE)) { return ;}

    std::shared_ptr<Client> target_client_ptr = getClientByNickname(target_nickname);
	if(!validateTargetExists(client, target_client_ptr, target_nickname, kicker_nickname)) { return; }
   	if (!validateIsClientInChannel (channel_ptr, client, channel_name, target_nickname)) { return ;}
	broadcastMessage(MessageBuilder::generateMessage(MsgType::KICK, {client->getNickname(), client->getUsername(), channel_name, target_nickname, kick_reason}), client, channel_ptr, false, nullptr);
    std::pair<MsgType, std::vector<std::string>> result = channel_ptr->promoteFallbackOperator(target_client_ptr, true);
	//VALIDATE
	if (result.first != MsgType::NONE){
	    //std::cout<<"DEBUGGIN::: breoadcasting message of new operator\n";
		broadcastMessage(MessageBuilder::generateMessage(result.first, result.second), target_client_ptr, channel_ptr, true, nullptr);
	}
	channel_ptr->removeClientByNickname(target_nickname); // Needs to be implemented in Channel
    target_client_ptr->removeJoinedChannel(channel_name_lower); // Needs to be implemented in Client
    if (channel_ptr->isEmpty()) { // Needs to be implemented in Channel
        _channels.erase(channel_name_lower); // Remove the channel from Server's map
        std::cout << "SERVER: Channel '" << channel_name << "' is now empty and has been removed.\n";
    }
}


std::shared_ptr<Client> Server::getClientByNickname(const std::string& nickname){
    std::cout << "SERVER: Looking up FD for nickname: '" << nickname << "' (case-sensitive) in _nickname_to_fd map\n"; // Uncomment for debugging
    // Step 1: Look up the file descriptor using the nickname
    auto fd_it = _nickname_to_fd.find(toLower(nickname));
    if (fd_it == _nickname_to_fd.end()) {
	    std::cout << "SERVER: Looking up FD for nickname: '" << nickname << "' (case-sensitive) in _nickname_to_fd map\n"; // Uncomment for debugging

		return nullptr;
    }
	std::shared_ptr<Client> client;
	try {
		client = get_Client(fd_it->second);
	} catch(const ServerException& e) {
		return nullptr;
	}
    return client; // Return the shared_ptr to the Client
}


void Server::handleTopicCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params) {
    std::cout << "SERVER: handleTopicCommand entered for " << client->getNickname() << std::endl;

    if (!client) {
        std::cerr << "SERVER ERROR: handleTopicCommand called with null client.\n";
        return;
    }
    const std::string& sender_nickname = client->getNickname();
    if (params.empty()) {
        broadcastMessage(MessageBuilder::generateMessage(MsgType::NEED_MORE_PARAMS, {sender_nickname, "TOPIC"}), client, nullptr, false, client);
        return;
    }
    std::string channel_name = toLower(params[0]);
	if (!validateChannelExists(client, channel_name, sender_nickname)) { return;}
    std::shared_ptr<Channel> channel_ptr = get_Channel(channel_name);
	if (!validateIsClientInChannel (channel_ptr, client, channel_name, sender_nickname)) { return ;}

	if (params.size() == 1) {
        std::string current_topic = channel_ptr->getTopic();
        if (current_topic.empty()) {
            broadcastMessage(MessageBuilder::generateMessage(MsgType::RPL_NOTOPIC, {sender_nickname, channel_name}), client, nullptr, false, client);
        } else {
            broadcastMessage(MessageBuilder::generateMessage(MsgType::RPL_TOPIC, {sender_nickname, channel_name, current_topic}), client, nullptr, false, client);
        }
        return; // Done with viewing topic
    }
	if (!validateModes(channel_ptr, client, Modes::TOPIC)) { std::cout<<"DEBUG TOPIC:: mode if false, should get not operator\n"; return;}
    std::string new_topic_content = params[1]; // The topic string starts at params[1]

	// multi word topics work
    if (!new_topic_content.empty() && new_topic_content[0] == ':') {
        new_topic_content = new_topic_content.substr(1); // Remove leading colon if present
    }
	

    // --- All checks passed for setting the topic ---

    // Set the new topic
    channel_ptr->setTopic(new_topic_content);
    std::cout << "SERVER: TOPIC - Set topic for '" << channel_name << "' to: '" << new_topic_content << "'.\n";

	std::string sender_prefix = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname();
    std::string topic_change_message = sender_prefix + " TOPIC " + channel_name + " :" + new_topic_content + "\r\n";

    // Use the versatile broadcastMessage function!
    broadcastMessage(topic_change_message, nullptr, channel_ptr, false, nullptr); // Broadcast to all in channel
}

void Server::handleInviteCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params) {
    std::cout << "SERVER: handleInviteCommand entered for " << client->getNickname() << std::endl;
    if (!client) {
        std::cerr << "SERVER ERROR: handleInviteCommand called with null client.\n";
        return;
    }
    const std::string& sender_nickname = client->getNickname();
    // 1. Check for correct number of parameters
    if (params.size() < 2) {
        broadcastMessage(MessageBuilder::generateMessage(MsgType::NEED_MORE_PARAMS, {sender_nickname, "INVITE"}), client, nullptr, false, client);
        return;
    }
    std::string target_nickname = params[0];
    std::string channel_name = toLower(params[1]);
    std::shared_ptr<Client> target_client_ptr = getClientByNickname(target_nickname);
    if (!validateTargetExists(client, target_client_ptr, target_nickname, sender_nickname)) { return; }
	if (!validateChannelExists(client, channel_name, sender_nickname)) { return;}
    std::shared_ptr<Channel> channel_ptr = get_Channel(channel_name);
	if (!validateIsClientInChannel (channel_ptr, client, channel_name, sender_nickname)) { return ;}
	if (!validateTargetInChannel (channel_ptr, client, channel_name, target_nickname)) { return ;}
   	if (!validateModes(channel_ptr, client, Modes::INVITE_ONLY)) { return;}
    channel_ptr->addInvite(target_nickname); // You'll need to implement this in Channel.cpp and declare in Channel.hpp
	broadcastMessage(MessageBuilder::generateMessage(MsgType::RPL_INVITING, {sender_nickname, target_nickname, channel_name}), client, nullptr, false, client);
	broadcastMessage(MessageBuilder::generateMessage(MsgType::GET_INVITE, {sender_nickname, client->getUsername(), target_nickname, channel_name}), client, nullptr, false, target_client_ptr);
}
