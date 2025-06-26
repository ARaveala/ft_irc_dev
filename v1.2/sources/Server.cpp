// sources/Server.cpp

#include <iostream> // For std::cout, std::cerr
#include <string>   // For std::string, std::to_string
#include <vector>   // For std::vector
#include <memory>   // For std::shared_ptr, std::unique_ptr
#include <algorithm> // For std::transform, std::find
#include <cctype>   // For std::isprint, std::tolower
#include <sys/socket.h> // For accept, recv, setsockopt
#include <netinet/tcp.h> // For TCP_NODELAY
#include <netinet/in.h>  // Added for IPPROTO_TCP definition
#include <errno.h> // For errno, EAGAIN, EWOULDBLOCK
#include <unistd.h> // For close
#include <set> // For std::set

#include "Server.hpp"
#include "Client.hpp"
#include "CommandDispatcher.hpp"
#include "MessageBuilder.hpp"
#include "ServerError.hpp"
#include "config.h" // For config constants
#include "IrcMessage.hpp" // For IrcMessage::to_lowercase

// Global helper for unique nickname generation
// Defined here so it's clearly visible and available before create_Client
std::string generateUniqueNickname(const std::map<std::string, int>& nickname_to_fd) {
    std::string base = "anonthe_";
    int counter = 0;
    std::string new_nick;
    do {
        new_nick = base + std::to_string(counter++);
    } while (nickname_to_fd.count(IrcMessage::to_lowercase(new_nick))); // Use IrcMessage::to_lowercase
    return new_nick;
}

// Global helper for lowercase conversion. This is fine to keep here.
std::string toLower(const std::string& input) {
    std::string output = input;
    std::transform(output.begin(), output.end(), output.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return output;
}


// Server Constructors
Server::Server() :
    _passwordRequired(true), // Always true as per brief requirement for explicit password
    _port(0), // Default port, should be set by parameterized constructor
    _client_count(0),
    _fd(-1),
    _current_client_in_progress(-1),
    _signal_fd(-1),
    _epoll_fd(-1),
    _server_name("ft_irc_server"),
    _password("") // Default empty password for default constructor
{
    _commandDispatcher = std::make_unique<CommandDispatcher>(this);
    std::cout << "#### Server instance created (default constructor).\n";
}

Server::Server(int port , const std::string& password) :
    _passwordRequired(true), // Always true as per brief requirement
    _port(port),
    _client_count(0),
    _fd(-1),
    _current_client_in_progress(-1),
    _signal_fd(-1),
    _epoll_fd(-1),
    _server_name("ft_irc_server"),
    _password(password)
{
    _commandDispatcher = std::make_unique<CommandDispatcher>(this);
    std::cout << "#### Server instance created on port " << _port << ".\n";
}

Server::~Server(){
    std::cout << "#### Server instance destroyed.\n";
    shutDown(); // Ensure proper shutdown
}

// Implementations for core Server methods

void Server::create_Client(int epollfd) {
    int client_fd = accept(getFd(), nullptr, nullptr);
    if (client_fd < 0) {
        throw ServerException(ErrorType::ACCEPT_FAILURE, "Error: accept failed in create_Client");
    }
    int flag = 1;
    socklen_t optlen = sizeof(flag);
    setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &flag, optlen);
    make_socket_unblocking(client_fd);
    setup_epoll(epollfd, client_fd, EPOLLIN | EPOLLOUT | EPOLLET);
    int timer_fd = setup_epoll_timer(epollfd, config::TIMEOUT_CLIENT);
    if (timer_fd == -1){
        epoll_ctl(epollfd, EPOLL_CTL_DEL, client_fd, nullptr);
        close(client_fd);
        throw ServerException(ErrorType::ACCEPT_FAILURE, "Error: timer_fd setup failed in create_Client");
    }
    // Pass _passwordRequired from the Server to the Client constructor
    _Clients[client_fd] = std::make_shared<Client>(client_fd, timer_fd, _passwordRequired);
    std::shared_ptr<Client> client = _Clients[client_fd];
    _timer_map[timer_fd] = client_fd;
    set_current_client_in_progress(client_fd);

    // Set a default nickname for the client (using generateUniqueNickname)
    // Using IrcMessage::to_lowercase for consistency if that's the desired lowercasing method.
    client->setNickname(generateUniqueNickname(_nickname_to_fd)); // Pass _nickname_to_fd
    client->setDefaults(); // Ensure other defaults are set if needed

    _fd_to_nickname.insert({client_fd, IrcMessage::to_lowercase(client->getNickname())});
    _nickname_to_fd.insert({IrcMessage::to_lowercase(client->getNickname()), client_fd});

    if (!client->get_acknowledged()){
        client->getMsg().queueMessage(MessageBuilder::generateInitMsg());
        set_client_count(1);
    }
    std::cout << "New Client created , fd value is  == " << client_fd << " Nickname: " << client->getNickname() << std::endl;
}

void Server::handlePassCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params) {
    if (client->getHasRegistered()) {
        client->sendMessage(":" + getServerName() + " 462 " + client->getNickname() + " :Unauthorized command (already registered)");
        return;
    }

    if (params.empty()) {
        client->sendMessage(":" + getServerName() + " 461 " + client->getNickname() + " PASS :Not enough parameters");
        return;
    }

    const std::string& clientPassword = params[0];

    const size_t MIN_PASS_LENGTH = 6;
    const size_t MAX_PASS_LENGTH = 20;

    if (clientPassword.length() < MIN_PASS_LENGTH) {
        client->sendMessage(":" + getServerName() + " 464 " + client->getNickname() + " :Password too short. Minimum " + std::to_string(MIN_PASS_LENGTH) + " characters required.");
        client->disconnect("Password policy violation: too short.");
        return;
    }

    if (clientPassword.length() > MAX_PASS_LENGTH) {
        client->sendMessage(":" + getServerName() + " 464 " + client->getNickname() + " :Password too long. Maximum " + std::to_string(MAX_PASS_LENGTH) + " characters allowed.");
        client->disconnect("Password policy violation: too long.");
        return;
    }

    for (char c : clientPassword) {
        if (!std::isprint(static_cast<unsigned char>(c))) {
            client->sendMessage(":" + getServerName() + " 464 " + client->getNickname() + " :Password contains invalid or non-printable characters.");
            client->disconnect("Password policy violation: invalid characters.");
            return;
        }
    }

    if (clientPassword == _password) {
        client->setIsAuthenticatedByPass(true);
    } else {
        client->sendMessage(":" + getServerName() + " 464 " + client->getNickname() + " :Password incorrect.");
        client->disconnect("Password incorrect.");
        return;
    }
}

void Server::handleReadEvent(const std::shared_ptr<Client>& client, int client_fd) {
    if (!client) { return; }
    set_current_client_in_progress(client_fd);
    char buffer[config::BUFFER_SIZE];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    std::cout << "Debug - Raw Buffer Data: [" << std::string(buffer, bytes_read) << "]" << std::endl;
    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) { return; }
        perror("recv failed in handleReadEvent");
        remove_Client(client_fd);
        return;
    }
    if (bytes_read == 0) {
        std::cout << "Client FD " << client_fd << " disconnected gracefully (recv returned 0).\n";
        handleQuit(client);
        return;
    }
    client->appendIncomingData(buffer, bytes_read);

    while (client->extractAndParseNextCommand()) {
        if (client->getQuit()) {
            std::cout << "DEBUG: Client FD " << client_fd << " marked for quit, stopping command processing loop.\n";
            break;
        }

        resetClientTimer(client_fd, config::TIMEOUT_CLIENT);
        client->set_failed_response_counter(-1);

        _commandDispatcher->dispatchCommand(client, client->getMsg().getParams());

        if (_Clients.find(client_fd) == _Clients.end()) {
             std::cout << "DEBUG: Client FD " << client_fd << " removed during command dispatch, stopping processing.\n";
             return;
        }
    }
}

void Server::broadcastMessage(const std::string& message_content, std::shared_ptr<Client> sender, std::shared_ptr<Channel> target_channel,
    bool skip_sender,
    std::shared_ptr<Client> individual_recipient
) {
    if (message_content.empty()) {
        std::cerr << "WARNING: Attempted to broadcast an empty message. Skipping.\n";
        return;
    }
    std::set<std::shared_ptr<Client>> recipients;
    if (individual_recipient) {
        recipients.insert(individual_recipient);
    } else if (target_channel) {
        recipients = getChannelRecipients(target_channel, sender, skip_sender);
    } else if (sender) {
        recipients = getSharedChannelRecipients(sender, skip_sender);
    } else {
        std::cerr << "Error: Invalid call to broadcastMessage. Must provide an individual recipient, a target channel, or a sender.\n";
        return;
    }

    for (const auto& recipientClient : recipients) {
        if (!recipientClient) {
            std::cerr << "WARNING: Attempted to queue message for a null recipient client.\n";
            continue;
        }
        bool wasEmpty = recipientClient->isMsgEmpty();
        recipientClient->getMsg().queueMessage(message_content);
        if (wasEmpty) {
            std::cout << "DEBUG: Message queued for FD " << recipientClient->getFd() << " (" << recipientClient->getNickname() << "). Updating epoll events.\n";
            updateEpollEvents(recipientClient->getFd(), EPOLLOUT, true);
        } else {
            std::cout << "DEBUG: Message queued for FD " << recipientClient->getFd() << " (" << recipientClient->getNickname() << "). Queue not empty.\n";
        }
    }
}

// ... (other Server method implementations) ...

// Implementations of dummy/placeholder functions (ensure these are in Server.cpp)
// (Only providing the actual implementations here, if they are not already in your refactored code)
int Server::getPort() const { return _port; }
int Server::getFd() const { return _fd; }
void Server::setFd(int fd) { _fd = fd; }
void Server::set_client_count(int val) {
    if (val > 0) _client_count += val;
    else _client_count = val;
}
int Server::get_client_count() const { return _client_count; }
void Server::set_current_client_in_progress(int fd) { _current_client_in_progress = fd; }
int Server::get_current_client_in_progress() const { return _current_client_in_progress; }

const std::string& Server::getServerName() const { return _server_name; } // This definition must be here.

std::shared_ptr<Client> Server::get_Client(int fd) {
    auto it = _Clients.find(fd);
    if (it != _Clients.end()) {
        return it->second;
    }
    return nullptr;
}

std::map<int, std::string>& Server::get_fd_to_nickname() { return _fd_to_nickname; }
std::map<std::string, int>& Server::get_nickname_to_fd() { return _nickname_to_fd; }
std::map<int, std::shared_ptr<Client>>& Server::get_map() { return _Clients; }

std::shared_ptr<Client> Server::getClientByNickname(const std::string& nickname) {
    auto it = _nickname_to_fd.find(IrcMessage::to_lowercase(nickname));
    if (it != _nickname_to_fd.end()) {
        return get_Client(it->second);
    }
    return nullptr;
}

int Server::get_fd(const std::string& nickname) const {
    auto it = _nickname_to_fd.find(IrcMessage::to_lowercase(nickname));
    if (it != _nickname_to_fd.end()) {
        return it->second;
    }
    return -1; // Indicate not found
}
void Server::set_signal_fd(int fd) { _signal_fd = fd; }
int Server::get_signal_fd() const { return _signal_fd; }
void Server::set_event_pollfd(int epollfd) { _epoll_fd = epollfd; }
int Server::get_event_pollfd() const { return _epoll_fd; }

void Server::set_nickname_in_map(std::string nick, int fd) {
    _nickname_to_fd[IrcMessage::to_lowercase(nick)] = fd;
    _fd_to_nickname[fd] = IrcMessage::to_lowercase(nick);
}

std::shared_ptr<Channel> Server::get_Channel(const std::string& channelName) {
    auto it = _channels.find(IrcMessage::to_lowercase(channelName));
    if (it != _channels.end()) {
        return it->second;
    }
    return nullptr;
}

// These definitions for get_channels_map() should only be in Server.hpp if they are inline.
// They were defined twice, causing redefinition errors. Removed from here.
/*
std::map<std::string, std::shared_ptr<Channel>>& Server::get_channels_map() { return _channels; }
const std::map<std::string, std::shared_ptr<Channel>>& Server::get_channels_map() const { return _channels; }
*/


bool Server::isTimerFd(int fd) {
    (void)fd; // Suppress unused parameter warning
    return _timer_map.count(fd) > 0;
}

void Server::closeAndResetClient() { /* Dummy or implement real logic */ } // Placeholder
void Server::remove_Client(int client_fd) {
    std::cout << "Removing client with fd: " << client_fd << " - START removal process.\n";
    std::shared_ptr<Client> client = get_Client(client_fd);
    if (!client) {
        std::cerr << "WARNING: Attempted to remove client FD " << client_fd << " but it was not found in _Clients map. Already removed?\n";
        return;
    }
    std::cout << "DEBUG: Client found (Nickname: " << client->getNickname() << "). Proceeding with resource cleanup.\n";

    std::cout << "DEBUG: Attempting to remove client FD " << client_fd << " from epoll.\n";
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr) == -1) {
        perror("epoll_ctl DEL client_fd failed in remove_Client");
    } else {
        std::cout << "DEBUG: Client FD " << client_fd << " successfully removed from epoll.\n";
    }

    if (client->get_timer_fd() != -1) {
        std::cout << "DEBUG: Attempting to remove timer FD " << client->get_timer_fd() << " from epoll.\n";
        if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client->get_timer_fd(), nullptr) == -1) {
            perror("epoll_ctl DEL timer_fd failed in remove_Client");
        } else {
            std::cout << "DEBUG: Timer FD " << client->get_timer_fd() << " successfully removed from epoll.\n";
        }
        close(client->get_timer_fd());
        _timer_map.erase(client->get_timer_fd());
        std::cout << "DEBUG: Timer FD " << client->get_timer_fd() << " closed and map entry removed.\n";
    } else {
        std::cout << "DEBUG: No timer FD associated with client FD " << client_fd << ".\n";
    }

    std::string nickname = IrcMessage::to_lowercase(client->getNickname()); // Use IrcMessage::to_lowercase
    std::cout << "DEBUG: Cleaning up nickname mappings for nickname: " << nickname << " (FD: " << client_fd << ").\n";
    if (_nickname_to_fd.count(nickname)) {
        _nickname_to_fd.erase(nickname);
        std::cout << "DEBUG: Nickname '" << nickname << "' removed from _nickname_to_fd.\n";
    }
    if (_fd_to_nickname.count(client_fd)) {
        _fd_to_nickname.erase(client_fd);
        std::cout << "DEBUG: FD " << client_fd << " removed from _fd_to_nickname.\n";
    }

    std::cout << "DEBUG: Removing client FD " << client_fd << " from joined channels.\n";
    std::map<std::string, std::weak_ptr<Channel>> clientJoinedChannels = client->getJoinedChannels();
    for (const auto& pair : clientJoinedChannels) {
        std::shared_ptr<Channel> channel = pair.second.lock();
        if (channel) {
            std::cout << "DEBUG: Calling channel->removeClient for channel: " << channel->getName() << " (Client Nickname: " << client->getNickname() << ").\n";
            channel->removeClient(client->getNickname()); // Corrected: Pass nickname (string)
        } else {
            std::cout << "DEBUG: Skipping channel cleanup for a stale weak_ptr (channel already gone?).\n";
        }
    }
    client->clearJoinedChannels();
    std::cout << "DEBUG: Client FD " << client_fd << " cleared from its joined channels list.\n";

    std::cout << "DEBUG: Closing client socket FD " << client_fd << ".\n";
    if (close(client_fd) == -1) {
        perror("close client_fd failed in remove_Client");
    } else {
        std::cout << "DEBUG: Client socket FD " << client_fd << " successfully closed.\n";
    }

    std::cout << "DEBUG: Removing client FD " << client_fd << " from _Clients map.\n";
    _Clients.erase(client_fd);
    std::cout << "DEBUG: Client FD " << client_fd << " removed from _Clients map.\n";

    set_client_count(-1);
    std::cout << "Removing client with fd: " << client_fd << " - END. Current client count: " << get_client_count() << "\n";
}

// Dummy implementations for validators and other functions to suppress unused parameter warnings
bool Server::validateChannelExists(const std::shared_ptr<Client>& client, const std::string& channel_name, const std::string& sender_nickname) {
    (void)client; (void)channel_name; (void)sender_nickname;
    return true; /* Dummy */
}
bool Server::validateIsClientInChannel(const std::shared_ptr<Channel> channel, const std::shared_ptr<Client>& client, const std::string& channel_name, const std::string& nickname) {
    (void)channel; (void)client; (void)channel_name; (void)nickname;
    return true; /* Dummy */
}
bool Server::validateTargetInChannel(const std::shared_ptr<Channel> channel, const std::shared_ptr<Client>& client, const std::string& channel_name, const std::string& target_nickname) {
    (void)channel; (void)client; (void)channel_name; (void)target_nickname;
    return true; /* Dummy */
}
bool Server::validateTargetExists(const std::shared_ptr<Client>& client, const std::string& sender_nickname, const std::string& target_nickname) {
    (void)client; (void)sender_nickname; (void)target_nickname;
    return true; /* Dummy */
}
bool Server::validateModes(const std::shared_ptr<Channel> Channel, const std::shared_ptr<Client>& client, Modes::ChannelMode comp) {
    (void)Channel; (void)client; (void)comp;
    return true; /* Dummy */
}
void Server::handle_client_connection_error(ErrorType err_type) {
    (void)err_type;
    /* Dummy */
}
void Server::shutDown() {
    std::cout << "Shutting down server..." << std::endl;
    for (auto const& [fd, client_ptr] : _Clients) {
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
        close(fd);
        if (client_ptr && client_ptr->get_timer_fd() != -1) {
            epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_ptr->get_timer_fd(), nullptr);
            close(client_ptr->get_timer_fd());
        }
    }
    _Clients.clear();
    _timer_map.clear();
    _nickname_to_fd.clear();
    _fd_to_nickname.clear();
    _channels.clear();

    if (_fd != -1) {
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, _fd, nullptr);
        close(_fd);
        _fd = -1;
    }
    if (_signal_fd != -1) {
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, _signal_fd, nullptr);
        close(_signal_fd);
        _signal_fd = -1;
    }
    if (_epoll_fd != -1) {
        close(_epoll_fd);
        _epoll_fd = -1;
    }
    std::cout << "Server shutdown complete.\n";
}
bool Server::checkTimers(int fd) {
    (void)fd;
    return false; /* Dummy */
}
void Server::resetClientFullTimer(int resetVal, std::shared_ptr<Client> client) {
    (void)resetVal; (void)client;
    /* Dummy */
}
int Server::setup_epoll(int epoll_fd, int fd, uint32_t events) {
    (void)epoll_fd; (void)fd; (void)events;
    return 0; /* Dummy */
}
int Server::setup_epoll_timer(int epoll_fd, int timeout_seconds) {
    (void)epoll_fd; (void)timeout_seconds;
    return -1; /* Dummy */
}
int Server::create_epollfd() { return -1; /* Dummy */ }
int Server::createTimerFD(int timeout_seconds) {
    (void)timeout_seconds;
    return -1; /* Dummy */
}
void Server::resetClientTimer(int timer_fd, int timeout_seconds) {
    (void)timer_fd; (void)timeout_seconds;
    /* Dummy */
}
int Server::make_socket_unblocking(int fd) {
    (void)fd;
    return 0; /* Dummy */
}
void Server::send_message(const std::shared_ptr<Client>& client) {
    (void)client;
    /* Dummy */
}
bool Server::channelExists(const std::string& channelName) const {
    (void)channelName;
    return false; /* Dummy */
}
void Server::updateEpollEvents(int fd, uint32_t flag_to_toggle, bool enable) {
    (void)fd; (void)flag_to_toggle; (void)enable;
    /* Dummy */
}
void Server::handleJoinChannel(const std::shared_ptr<Client>& client, std::vector<std::string> params) {
    (void)client; (void)params;
    /* Dummy */
}
void Server::handlePing(const std::shared_ptr<Client>& client) {
    (void)client;
    /* Dummy */
}
void Server::handlePong(const std::shared_ptr<Client>& client) {
    (void)client;
    /* Dummy */
}
void Server::handleQuit(std::shared_ptr<Client> client) {
    if (client) {
        std::string quit_message = ":" + client->getNickname() + " QUIT :Client Quit\r\n";
        std::set<std::shared_ptr<Client>> shared_recipients = getSharedChannelRecipients(client, true);
        for (const auto& recipient : shared_recipients) {
            recipient->sendMessage(quit_message);
        }
        remove_Client(client->getFd());
    }
}
void Server::handleNickCommand(const std::shared_ptr<Client>& client, std::map<int, std::string>& fd_to_nick, std::map<std::string, int>& nick_to_fd, const std::string& param) {
    (void)client; (void)fd_to_nick; (void)nick_to_fd; (void)param;
    /* Dummy */
}
void Server::handleModeCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params) {
    (void)client; (void)params;
    /* Dummy */
}
void Server::handleCapCommand(const std::string& nickname, std::deque<std::string>& que, bool& capSent) {
    (void)nickname; (void)que; (void)capSent;
    /* Dummy */
}
void Server::handlePartCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params) {
    (void)client; (void)params;
    /* Dummy */
}
void Server::handleKickCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params) {
    (void)client; (void)params;
    /* Dummy */
}
void Server::handleWhoIs(std::shared_ptr<Client> client, std::string param) {
    (void)client; (void)param;
    /* Dummy */
}
void Server::handleTopicCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params) {
    (void)client; (void)params;
    /* Dummy */
}
void Server::handleInviteCommand(std::shared_ptr<Client> client, const std::vector<std::string>& params) {
    (void)client; (void)params;
    /* Dummy */
}
std::set<std::shared_ptr<Client>> Server::getChannelRecipients(std::shared_ptr<Channel> channel, std::shared_ptr<Client> sender, bool skip_sender) {
    (void)channel; (void)sender; (void)skip_sender;
    return {}; /* Dummy */
}
// Corrected signature for getSharedChannelRecipients
std::set<std::shared_ptr<Client>> Server::getSharedChannelRecipients(std::shared_ptr<Client> sender, bool skip_sender) {
    (void)sender; (void)skip_sender;
    return {}; /* Dummy */
}

std::vector<std::string> Server::splitCommaList(const std::string& input) {
    (void)input;
    return {}; /* Dummy */
}
std::pair<MsgType, std::vector<std::string>> Server::validateChannelName(const std::string& channelName, const std::string& clientNick) {
    (void)channelName; (void)clientNick;
    return {MsgType::NONE, {}}; /* Dummy */
}
void Server::createChannel(const std::string& channelName) {
    (void)channelName;
    /* Dummy */
}
