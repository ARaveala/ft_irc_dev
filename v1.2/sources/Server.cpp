#include <algorithm> // find_if
#include <ctime>
#include <cctype>    // Required for std::tolower (character conversion)
#include <iostream> // testing with cout
//#include <sys/types.h>
#include <unistd.h>
//#include <string.h> //strlen
//#include <sstream>  // for passing parameters
#include <sys/socket.h>
#include <sys/epoll.h>
#include <map>
#include <memory> // shared pointers
#include <netinet/tcp.h>
#include <netinet/in.h>

#include <cstdlib>
#include <fstream>

#include <unordered_set>
#include <vector>

#include "config.h"
#include "ServerError.hpp"
#include "Channel.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include "MessageBuilder.hpp"

class ServerException;

static std::string toLower(const std::string& input) {
	std::string output = input;
	std::transform(output.begin(), output.end(), output.begin(),
				   [](unsigned char c) { return std::tolower(c); });
	return output;
}

Server::Server(){
	std::cout << "Server::Server -  Server instance created.\n";
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
void Server::setFd(int fd){_fd = fd;}
void Server::set_signal_fd(int fd){_signal_fd = fd;}
void Server::set_event_pollfd(int epollfd){	_epoll_fd = epollfd;} // note we may want to check here for values below 0
void Server::set_client_count(int val){	_client_count += val;}
void Server::set_current_client_in_progress(int fd){_current_client_in_progress = fd;}

// ~~~GETTERS
int Server::getFd() const { return _fd;}
int Server::get_signal_fd() const{ return _signal_fd;}
int Server::get_client_count() const{ return _client_count;}
int Server::getPort() const{return _port;}
int Server::get_event_pollfd() const{return _epoll_fd;}
int Server::get_current_client_in_progress() const{	return _current_client_in_progress;}

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

// validators these are tasked with a simple job, it helps prevent repetative code all over the files
bool Server::validateChannelExists(const std::shared_ptr<Client>& client, const std::string& channel_name, const std::string& sender_nickname) {
 if(!channelExists(toLower(channel_name))) {
		broadcastMessage(MessageBuilder::generateMessage(MsgType::NO_SUCH_CHANNEL, {sender_nickname, channel_name}), client, nullptr, false, client);
        return false;
    }
	return true;
}

bool Server::validateIsClientInChannel(const std::shared_ptr<Channel> channel, const std::shared_ptr<Client>& client, const std::string& channel_name, const std::string& nickname){
	LOG_DEBUG("validateIsClientInChannel checking nickname = " + nickname);
	if (!channel->isClientInChannel(toLower(nickname))) {
        broadcastMessage(MessageBuilder::generateMessage(MsgType::NOT_ON_CHANNEL, {nickname, channel_name}), client, nullptr, false, client);
        return false;
    }
	return true;
}

bool Server::validateTargetInChannel(const std::shared_ptr<Channel> channel, const std::shared_ptr<Client>& client, const std::string& channel_name, const std::string& target_nickname) {
	if (channel->isClientInChannel(toLower(target_nickname))) {
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

//add command
bool Server::validateParams(const std::shared_ptr<Client>& client, const std::string& sender_nickname, size_t paramSize, size_t comparison, const std::string& command){
    if (paramSize == 0 || paramSize < comparison) {
        broadcastMessage(MessageBuilder::generateMessage(MsgType::NEED_MORE_PARAMS, {sender_nickname, command}), client, nullptr, false, client);
        return false;
    }
	return true;
}

bool Server::validateClientNotEmpty(std::shared_ptr<Client> client){
    if (!client) {
        LOG_ERROR("handleInviteCommand called with null client.\n");
        return false;
    }
	return true;
}

void Server::removeFdFromEpoll(int fd) {
	if (_epollEventMap.count(fd)) {
    	epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    	_epollEventMap.erase(fd);
    	close(fd);
	}
}
/**
 * @brief Here a client is accepted , error checked , socket is adusted for non-blocking
 * the client fd is added to the epoll and then added to the Client map. a welcome message
 * is sent as an acknowlegement message back to irssi.
 */
void Server::create_Client(int epollfd) {

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
	if (!client->get_acknowledged()){			
		client->getMsg().queueMessage(MessageBuilder::generateInitMsg());
		set_client_count(1);		
	}
	std::cout << "New Client created , fd value is  == " << client_fd << std::endl;
}

void Server::remove_Client(int client_fd) {

    std::shared_ptr<Client> client_to_remove = get_Client(client_fd);
    if (!validateClientNotEmpty(client_to_remove)) {return;}

    std::vector<std::string> joined_channel_names;
    for (const auto& pair : client_to_remove->getJoinedChannels()) {
        if (pair.second.lock()) {
            joined_channel_names.push_back(pair.first);
        }
    }

    for (const std::string& channel_name : joined_channel_names) {
        std::shared_ptr<Channel> channel_ptr;
        try {
            channel_ptr = get_Channel(channel_name);
			validateFallbackOperator(channel_ptr, client_to_remove);
        } catch (const ServerException& e) {
            if (e.getType() == ErrorType::NO_CHANNEL_INMAP) {
                continue;
            }
        }
        if (channel_ptr) {
			LOG_NOTICE("Notifying and removing client " + client_to_remove->getNickname() + " from channel " + channel_name + " due to disconnect.\n");
            bool channel_became_empty = channel_ptr->removeClientByNickname(client_to_remove->getNickname());
            if (channel_became_empty) {
                _channels.erase(channel_name); 
            }
        }
    }
    client_to_remove->clearJoinedChannels();
    _nickname_to_fd.erase(client_to_remove->getNickname());
    _fd_to_nickname.erase(client_fd);
	removeFdFromEpoll(client_to_remove->get_timer_fd());
	removeFdFromEpoll(client_fd);
    _Clients.erase(client_fd);
    _epollEventMap.erase(client_fd);    
    _client_count--;
	LOG_DEBUG("Server::remove_Client - Client has been completely removed. Total clients: " + std::to_string(_client_count));
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

std::map<int, std::shared_ptr<Client>>& Server::get_map() {	return _Clients;}
std::map<int, std::string>& Server::get_fd_to_nickname() {return _fd_to_nickname;}
std::map<std::string, int>& Server::get_nickname_to_fd() {return _nickname_to_fd;}
std::string Server::get_password() const {return _password;}

void Server::closeAndResetClient() {
    if (_current_client_in_progress > 0) {
        close(_current_client_in_progress);
        _current_client_in_progress = 0;
    }
}

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
			LOG_ERROR("server Unknown error occurred");
			break;
	}
}

void Server::handleReadEvent(const std::shared_ptr<Client>& client, int client_fd) {
    if (!client) { return; }
	set_current_client_in_progress(client_fd);
    char buffer[config::BUFFER_SIZE];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) { return; }
		remove_Client(client_fd);
		return;
    }
    if (bytes_read == 0) {
		LOG_DEBUG("Client FD " + std::to_string(client_fd) + " disconnected gracefully (recv returned 0)");
        remove_Client(client_fd);
        return;
    }
    client->appendIncomingData(buffer, bytes_read);
    while (client->extractAndParseNextCommand()) {
        resetClientTimer(client_fd, config::TIMEOUT_CLIENT);
        client->set_failed_response_counter(-1);
        _commandDispatcher->dispatchCommand(client, client->getMsg().getParams());
    }
}

void Server::shutDown() {

	std::vector<int> clientFds;
	for (const auto& entry : _Clients) {clientFds.push_back(entry.first);}
	for (int fd : clientFds){remove_Client(fd);}
	for (std::map<const std::string, std::shared_ptr<Channel>>::iterator it = _channels.begin(); it != _channels.end(); it++){
		it->second->clearAllChannel();
	}

	close(_fd);
	close(_signal_fd);
	close(_epoll_fd);

	_timer_map.clear();
	_Clients.clear();
	_epollEventMap.clear();
	_server_broadcasts.clear();
	std::cout<<"server shutDown completed"<<std::endl;
}

Server::~Server(){
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
	if (timerit == _timer_map.end()) { return true; }
	int client_fd = timerit->second;
    auto clientit = _Clients.find(client_fd);
    if (clientit == _Clients.end()) {
		LOG_ERROR("No client fd matching timer fd found in map , client fd = " +  std::to_string(fd));
		return false;
	}
    if (clientit->second->get_failed_response_counter() == 3) {
		LOG_WARN("failed response counter has reached max, client timeout reached for client fd = " +  std::to_string(fd));
		remove_Client(client_fd);
        _timer_map.erase(fd);
        return false;
    }
	LOG_NOTICE("PING has been qued for client fd = " + std::to_string(fd));
	broadcastMessage(MessageBuilder::bildPing(), nullptr, nullptr, false, get_Client(fd));
	resetClientFullTimer(1, clientit->second);
	return false;
}

void Server::resetClientFullTimer(int resetVal, std::shared_ptr<Client> client){
	if (!validateClientNotEmpty(client)) {return ;}
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
		LOG_DEBUG("checking the message from que before send ["+ msg +"] and the fd = "+ std::to_string(fd));
		ssize_t bytes_sent = send(fd, send_buffer_ptr.c_str(), remaining_bytes_to_send, MSG_NOSIGNAL);
        if (bytes_sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
				LOG_DEBUG("send_message:: socket buffer full. EAGAIN/EWOULDBLOCK for FD " + std::to_string(fd) + ". Pausing write.");
                return;
            }
			broadcastMessage(MessageBuilder::generateMessage(MsgType::CLIENT_QUIT, {client->getNickname(), client->getUsername()}), client, nullptr, true, nullptr);
			remove_Client(fd);
			LOG_ERROR("send_message:: bytes_sent = -1, disconnection client fd = " + std::to_string(fd));
            return;
        }
		client->getMsg().advanceCurrentMessageOffset(bytes_sent); 
		if (client->getMsg().getRemainingBytesInCurrentMessage() == 0) {
        client->getMsg().removeQueueMessage();
		LOG_DEBUG("Full message sent for FD " + std::to_string(fd) + ". Moving to next message.");
	    } else { return; }
	}
	if (client->isMsgEmpty()) {
		if (!client->get_acknowledged()) {
			client->set_acknowledged();		
		}
		updateEpollEvents(fd, EPOLLOUT, false);
	}

}

bool Server::channelExists(const std::string& channelName) const {return _channels.count(channelName) > 0;}

void Server::createChannel(const std::string& channelName) {
    std::string lower =  toLower(channelName);
	if (_channels.count(lower) == 0){
		_channels.emplace(lower, std::make_shared<Channel>(channelName));
        std::cout << "Channel '" << channelName << "' created." << std::endl;
    }
}

std::shared_ptr<Channel> Server::get_Channel(const std::string& ChannelName) {
	std::string channel_name_lower = toLower(ChannelName); 
	for (const auto& [name, channel] : _channels) {
    	if (name == channel_name_lower)
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
void Server::handleJoinChannel(const std::shared_ptr<Client>& client, std::vector<std::string> params){
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

void Server::updateNickname(const std::shared_ptr<Client>& client, const std::string& newNick, const std::string& oldNick) {
    const int& fd = client->getFd();
    std::string old_lc = toLower(oldNick);
    std::string new_lc = toLower(newNick);
    _nickname_to_fd.erase(old_lc);
    client->setNickname(newNick);
    _nickname_to_fd[new_lc] = fd;
}

bool Server::validateRegistrationTime(const std::shared_ptr<Client>& client) {
	auto now = std::chrono::steady_clock::now();
	if (now - client->getRegisteredAt() < std::chrono::seconds(10)) {
		   // client->getMsg().queueMessage(":localhost 439 "+client->getNickname()+" :Please wait a moment before providing innput, server loading......\r\n");
			updateEpollEvents(client->getFd(), EPOLLOUT, true);
		    return false;
	}
	return true;
}

void Server::handleNickCommand(const std::shared_ptr<Client>& client, std::map<std::string, int>& nick_to_fd, const std::string& param) 
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

void Server::handleQuit(std::shared_ptr<Client> client) {
	if (!validateClientNotEmpty(client)) {return;}
	std::string quit_message =  MessageBuilder::generateMessage(MsgType::CLIENT_QUIT, {client->getNickname(), client->getClientUname()});//client_prefix + " QUIT :Client disconnected\r\n"; // Default reason
	broadcastMessage(quit_message, client, nullptr, true, nullptr);
	client->setQuit();
	LOG_DEBUG("handleQuit:: Client " + client->getNickname() + " marked for disconnection, epollout will trigger removal");
}

void Server::handleModeCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params){
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
			if (!modeparams[0].empty())
				messageParams.push_back(modeparams[0]);
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
}

void Server::handleCapCommand(const std::shared_ptr<Client>& client){
        broadcastMessage(MessageBuilder::buildCapResponse(), nullptr, nullptr, false, client);
		client->setHasSentCap();
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
        LOG_ERROR("updateEpollEvents:: setEpollFlagStatus: FD " + std::to_string(fd) + " not found in _epollEventMap. Cannot modify events. This might indicate a missing client cleanup. removing client");
		remove_Client(fd);
		return;
    }
    uint32_t current_mask = it->second.events;
    uint32_t new_mask;
	if (enable) {
        new_mask = current_mask | flag_to_toggle;
    } else {
        new_mask = current_mask & ~flag_to_toggle;
    }
    if (new_mask == current_mask) {return; }
    it->second.events = new_mask;
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, fd, &it->second) == -1) {
		LOG_DEBUG("Updating epoll event mask for FD " + std::to_string(fd) +
          ": old mask=" + std::to_string(current_mask) +
          ", new mask=" + std::to_string(new_mask));
        remove_Client(fd);
    }
}

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

void Server::handleWhoIs(std::shared_ptr<Client> requester_client, std::string target_nick) {
    std::shared_ptr<Client> target_client = getClientByNickname(target_nick); // tolower?
    if (!validateTargetExists(requester_client, target_client, requester_client->getNickname() , target_nick)) {return ;}
	std::vector<std::string> whois_params;
	whois_params.push_back(requester_client->getNickname());
	whois_params.push_back(target_client->getNickname());
	whois_params.push_back(target_client->getClientUname());
	whois_params.push_back(target_client->getfullName());
	whois_params.push_back(std::to_string(target_client->getIdleTime()));
	whois_params.push_back(std::to_string(target_client->getSignonTime()));

	std::string channel_list;
	const std::map<std::string, std::weak_ptr<Channel> >& joined = target_client->getJoinedChannels();

	for (const auto& pair : joined) {
	    if (auto chan = pair.second.lock()) {
	        channel_list += chan->getClientModePrefix(target_client) + pair.first + " ";
	    }
	}
	if (!channel_list.empty())
	    channel_list.pop_back(); // remove trailing space
	whois_params.push_back(channel_list); 
	
	broadcastMessage(MessageBuilder::buildWhois(whois_params), nullptr, nullptr, false, requester_client);
}

void Server::handlePartCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params) {
    
	if (!validateClientNotEmpty(client)) {return ;}
	const std::string& nickname = client->getNickname();
	LOG_DEBUG("handlePartCommand entered for " + nickname);
	if (!validateParams(client, nickname, params.size(), 0, "PART")) {return ;}
    std::string part_reason = (params.size() > 1 && !params[1].empty()) ? params[1] : nickname;
	std::vector<std::string> channels_to_part = splitCommaList(params[0]);
    for (const std::string& ch_name : channels_to_part) {
		if (!validateChannelExists(client, ch_name, nickname)) {return;}
        std::shared_ptr<Channel> channel_ptr = get_Channel(ch_name);
        if (!validateIsClientInChannel (channel_ptr, client, ch_name, nickname)) { return ;}		
		broadcastMessage(MessageBuilder::generateMessage(MsgType::PART, {nickname, client->getUsername() ,channel_ptr->getName(), part_reason}), nullptr, channel_ptr, false, nullptr);
		validateFallbackOperator(channel_ptr, client);
		bool channel_is_empty = channel_ptr->removeClientByNickname(nickname);
        client->removeJoinedChannel(ch_name);
   		if (channel_is_empty) {
        	_channels.erase(toLower(ch_name));
			LOG_NOTICE("Channel " + ch_name + "is now empty and has been removed");
    	}
    }
}

void Server::validateFallbackOperator(const std::shared_ptr<Channel>& channel, const std::shared_ptr<Client>& client){
	std::pair<MsgType, std::vector<std::string>> result = channel->promoteFallbackOperator(client, true);
	if (result.first != MsgType::NONE){
	    //std::cout<<"DEBUGGIN::: breoadcasting message of new operator\n";
		broadcastMessage(MessageBuilder::generateMessage(result.first, result.second), client, channel, true, nullptr);
	}
}

void Server::handleKickCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params) {
    if (!validateClientNotEmpty(client)) {return;}
    const std::string& kicker_nickname = client->getNickname();
	LOG_DEBUG("handleKickCommand entered for " + kicker_nickname);
	
    if (!validateParams(client, kicker_nickname, params.size(), 2, "KICK")) { return; }

    std::string channel_name = params[0];
    std::string target_nickname = params[1];
    std::string kick_reason = (params.size() > 2) ? params[2] : kicker_nickname; // Default reason is kicker's nick
    if (!validateChannelExists(client, channel_name, kicker_nickname)) { return;}
    std::shared_ptr<Channel> channel_ptr = get_Channel(channel_name);
    if (!validateIsClientInChannel (channel_ptr, client, channel_name, kicker_nickname)) { return ;}
	if (!validateModes(channel_ptr, client, Modes::NONE)) { return ;}

    std::shared_ptr<Client> target_client_ptr = getClientByNickname(target_nickname);
	if(!validateTargetExists(client, target_client_ptr, target_nickname, kicker_nickname)) { return; }
   	if (!validateIsClientInChannel (channel_ptr, client, channel_name, target_nickname)) { return ;}
	broadcastMessage(MessageBuilder::generateMessage(MsgType::KICK, {client->getNickname(), client->getUsername(), channel_name, target_nickname, kick_reason}), client, channel_ptr, false, nullptr);
    validateFallbackOperator(channel_ptr, client);
	channel_ptr->removeClientByNickname(target_nickname);
    target_client_ptr->removeJoinedChannel(channel_name);
    if (channel_ptr->isEmpty()) {
        _channels.erase(toLower(channel_name));
		LOG_NOTICE("Channel " + channel_name + "is now empty and has been removed");
    }
}


std::shared_ptr<Client> Server::getClientByNickname(const std::string& nickname){
    LOG_DEBUG("getClientByNickname: Looking up FD for nickname: " + nickname + " (case-sensitive) in _nickname_to_fd map");
    auto fd_it = _nickname_to_fd.find(toLower(nickname));
    if (fd_it == _nickname_to_fd.end()) {
	    LOG_DEBUG("getClientByNickname:  " + nickname + " not found in _nickname_to_fd map");
		return nullptr;
    }
	std::shared_ptr<Client> client;
	try {
		client = get_Client(fd_it->second);
	} catch(const ServerException& e) {
	    LOG_DEBUG("getClientByNickname:  " + nickname + " not found in shared ptr map");
		return nullptr;
	}
    return client;
}


void Server::handleTopicCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params) {
	if (!validateClientNotEmpty(client)) {return ;}
	const std::string& sender_nickname = client->getNickname();

	LOG_DEBUG("handleTopicCommand: entered for " + sender_nickname);

    if (!validateParams(client, sender_nickname, params.size(), 2, "TOPIC")){return ;}

    std::string channel_name = toLower(params[0]);
	if (!validateChannelExists(client, channel_name, sender_nickname)) { return;}
    std::shared_ptr<Channel> channel = get_Channel(channel_name);
	if (!validateIsClientInChannel (channel, client, channel_name, sender_nickname)) { return ;}

	if (params.size() == 1) {
        std::string current_topic = channel->getTopic();
        if (current_topic.empty()) {
            broadcastMessage(MessageBuilder::generateMessage(MsgType::RPL_NOTOPIC, {sender_nickname, channel_name}), client, nullptr, false, client);
        } else {
            broadcastMessage(MessageBuilder::generateMessage(MsgType::RPL_TOPIC, {sender_nickname, channel_name, current_topic}), client, nullptr, false, client);
        }
        return;
    }
	if (!validateModes(channel, client, Modes::TOPIC)) {
		LOG_DEBUG("TOPIC:: topic +t , should get not operator");
		return;
	}
    std::string new_topic_content = params[1];
    if (!new_topic_content.empty() && new_topic_content[0] == ':') {
        new_topic_content = new_topic_content.substr(1);
    }
    channel->setTopic(new_topic_content);
    LOG_DEBUG("TOPIC - Set topic for " + channel_name + " to: " + new_topic_content);
	std::string topic = MessageBuilder::generateMessage(MsgType::RPL_TOPIC, {client->getNickname(), client->getUsername(), channel->getName(), new_topic_content});
    broadcastMessage(topic, nullptr, channel, false, nullptr);
}

void Server::handleInviteCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params) {    
	if (!validateClientNotEmpty(client)) {return ;}
    LOG_NOTICE("handleInviteCommand :entered for "+ client->getNickname());

	const std::string& sender_nickname = client->getNickname();
	
	if (!validateParams(client, sender_nickname, params.size(), 2, "INVITE")){return ;}
    std::string target_nickname = params[0];
    std::string channel_name = toLower(params[1]);
    
	std::shared_ptr<Client> target_client = getClientByNickname(target_nickname);

    if (!validateTargetExists(client, target_client, target_nickname, sender_nickname)) { return; }
	if (!validateChannelExists(client, channel_name, sender_nickname)) { return;}
    
	std::shared_ptr<Channel> channel = get_Channel(channel_name);
	if (!validateIsClientInChannel (channel, client, channel_name, sender_nickname)) { return ;}
	if (!validateTargetInChannel (channel, client, channel_name, target_nickname)) { return ;}
   	if (!validateModes(channel, client, Modes::INVITE_ONLY)) { return;}
    channel->addInvite(target_nickname);
	
	std::string msgInvite = MessageBuilder::generateMessage(MsgType::RPL_INVITING, {sender_nickname, target_nickname, channel_name});
    std::string msgNotice = MessageBuilder::generateMessage(MsgType::GET_INVITE, {sender_nickname, client->getUsername(), target_nickname, channel_name});
	
	broadcastMessage(msgInvite, client, nullptr, false, client);
	broadcastMessage(msgNotice, client, nullptr, false, target_client);
}

void Server::handlePrivMsg(const std::vector<std::string>& params, const std::shared_ptr<Client>& client) {
	if (!params[0].empty()) {
		std::string contents = MessageBuilder::buildPrivMessage(client->getNickname(), client->getUsername(), params[0], params[1]);
		if (params[0][0] == '#') {
			std::cout<<"DEBUGGIN PRIVMESSAGE CHANNEL EDITION :: before validate channel\n";
			if (!validateChannelExists(client, params[0], client->getNickname())) { return;}
			broadcastMessage(contents, client, get_Channel(params[0]), true, nullptr);
		} else {
			std::shared_ptr<Client> target = getClientByNickname(params[0]);
			if (!validateTargetExists(client, target, client->getNickname(), params[0])) { return ;}
			broadcastMessage(contents, client, nullptr, true, target);				
		}
	}	
}

void Server::handleUser(const std::shared_ptr<Client>& client, const std::vector<std::string>& params) {
		client->setClientUname(params[0]);
		client->setRealname(params[3]);
		client->setHasSentUser();
		tryRegisterClient(client);
}