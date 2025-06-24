/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akuburas <akuburas@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/11 22:08:23 by akuburas          #+#    #+#             */
/*   Updated: 2025/03/20 13:26:53 by akuburas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"

static volatile sig_atomic_t s_shutdown = 0;

extern "C" void sigintHandler(int sig) {
    (void)sig;
    s_shutdown = 1;
    std::cout << "\nReceived SIGINT (Ctrl+C). Shutting down server gracefully..." << std::endl;
}

bool Server::isShutdownRequested() {
    return s_shutdown == 1;
}

Server::Server(int port, std::string password) {
    this->_port = port;
    this->_password = password;
    initializeCommandHandlers();
}

void Server::initializeCommandHandlers() {
    _commands["CAP"] = [this](std::shared_ptr<Client>& client, const std::string& message) { Cap(client, message); };
    _commands["NICK"] = [this](std::shared_ptr<Client>& client, const std::string& message) { Nick(client, message); };
    _commands["USER"] = [this](std::shared_ptr<Client>& client, const std::string& message) { User(client, message); };
    _commands["PING"] = [this](std::shared_ptr<Client>& client, const std::string& message) { Ping(client, message); };
    _commands["PONG"] = [this](std::shared_ptr<Client>& client, const std::string& message) { Pong(client, message); };
    _commands["MODE"] = [this](std::shared_ptr<Client>& client, const std::string& message) { Mode(client, message); };
    _commands["PRIVMSG"] = [this](std::shared_ptr<Client>& client, const std::string& message) { Priv(client, message); };
    _commands["JOIN"] = [this](std::shared_ptr<Client>& client, const std::string& message) { Join(client, message); };
    _commands["PART"] = [this](std::shared_ptr<Client>& client, const std::string& message) { Part(client, message); };
    _commands["QUIT"] = [this](std::shared_ptr<Client>& client, const std::string& message) { Quit(client, message); };
    _commands["PASS"] = [this](std::shared_ptr<Client>& client, const std::string& message) { Pass(client, message); };
    _commands["STATS"] = [this](std::shared_ptr<Client>& client, const std::string& message) { Stats(client, message); };
    _commands["WHOIS"] = [this](std::shared_ptr<Client>& client, const std::string& message) { Whois(client, message); };
    _commands["TOPIC"] = [this](std::shared_ptr<Client>& client, const std::string& message) { Topic(client, message); };
    _commands["HELP"] = [this](std::shared_ptr<Client>& client, const std::string& message) { Help(client, message); };
    _commands["INVITE"] = [this](std::shared_ptr<Client>& client, const std::string& message) { Invite(client, message); };
    _commands["KICK"] = [this](std::shared_ptr<Client>& client, const std::string& message) { Kick(client, message); };
    _commands["WHO"] = [this](std::shared_ptr<Client>& client, const std::string& message) { Who(client, message); };
    _commands["LIST"] = [this](std::shared_ptr<Client>& client, const std::string& message) { List(client, message); };
}

Server::~Server() {}

void Server::cleanupFd(int fd_index) {
    if (fd_index < 0 || fd_index >= static_cast<int>(_poll_fds.size())) {
        return;
    }
    
    if (_poll_fds[fd_index].fd >= 0) {
        close(_poll_fds[fd_index].fd);
    }
    
    _poll_fds.erase(_poll_fds.begin() + fd_index);
}

bool Server::handleNewConnection() {
    if (_poll_fds.size() >= MAX_CLIENTS) {
        std::cerr << "Maximum clients reached, rejecting connection" << std::endl;
        return false;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    int client_fd = accept(_serverSocket, (struct sockaddr*)&client_addr, &client_addr_len);
                         
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept failed");
        }
        return false;
    }

    int flags = fcntl(client_fd, F_GETFL, 0);
    if (flags < 0 || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(client_fd);
        return false;
    }

    struct pollfd client_pollfd;
    client_pollfd.fd = client_fd;
    client_pollfd.events = POLLIN;
    client_pollfd.revents = 0;
    _poll_fds.push_back(client_pollfd);

    _clients.push_back(std::make_shared<Client>(client_fd, client_addr));

    std::cout << "[" + _name +"] New client connected: " << client_fd << std::endl;
    return true;
}

bool Server::handleClientData(size_t index) {
    char buffer[1024];
    int fd = _poll_fds[index].fd;
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            auto client_it = std::find_if(_clients.begin(), _clients.end(),
                [fd](const std::shared_ptr<Client>& c) { return c->getClientFd() == fd; });
                
            if (client_it != _clients.end()) {
                disconnectClient(*client_it, "Connection error");
            }
            return false;
        }
        return true;
    }
    
    if (bytes_read == 0) {
        auto client_it = std::find_if(_clients.begin(), _clients.end(),
            [fd](const std::shared_ptr<Client>& c) { return c->getClientFd() == fd; });
            
        if (client_it != _clients.end()) {
            disconnectClient(*client_it, "Client closed connection");
        }
        return false;
    }

    if (bytes_read >= static_cast<ssize_t>(sizeof(buffer) - 1)) {
        std::cerr << "Received more data than buffer size, truncating" << std::endl;
        bytes_read = sizeof(buffer) - 1;
    }
    
    buffer[bytes_read] = '\0';
    auto client_it = std::find_if(_clients.begin(), _clients.end(),
        [fd](const std::shared_ptr<Client>& c) { return c->getClientFd() == fd; });
        
    if (client_it != _clients.end()) {
        (*client_it)->appendBuffer(buffer, bytes_read);
        (*client_it)->setLastActivity();

        if (!(*client_it)->hasCompleteMessage())
			std::cout << fd << " >> [PARTIAL MESSSAGE] " << buffer << std::endl;
		else{
			std::string data = (*client_it)->getAndClearBuffer();
			if (data.size() > 4096) {
				std::cerr << "[" + _name + "] Warning: Unusually large message received (size: " << data.size() << " bytes)" << std::endl;
				return true;
			}
			std::cout << fd << " >> " << data;
			std::vector<std::string> messages = splitMessages(data);

			if (!(*client_it)->getRegistration()) {
				if (!connectionHandshake(*client_it, messages, fd)) {
					disconnectClient(*client_it, "Invalid Password");
					return false;
				}
			} else {
				for (const std::string& message : messages) {
					handleMessage(*client_it, message);
				}
			}
			}
        }
		return true;
    }


void Server::Run() {
    _poll_fds.reserve(MAX_CLIENTS);

    struct pollfd server_pollfd;
    server_pollfd.fd = _serverSocket;
    server_pollfd.events = POLLIN;
    server_pollfd.revents = 0;
    _poll_fds.push_back(server_pollfd);

    time_t lastCheck = std::time(NULL);
    const time_t checkInterval = 60;

    while (!isShutdownRequested()) {
        int poll_result = poll(_poll_fds.data(), _poll_fds.size(), checkInterval * 1000);
        if (poll_result < 0) {
            if (errno == EINTR) continue;
            perror("poll failed");
            return;
        }

        time_t currentTime = std::time(NULL);
        if ((currentTime - lastCheck) >= checkInterval) {
            checkClientTimeouts();
            lastCheck = currentTime;
        }

        for (size_t i = 0; i < _poll_fds.size(); ++i) {
            if (_poll_fds[i].revents & (POLLERR | POLLHUP)) {
                if (_poll_fds[i].fd != _serverSocket) {
                    auto client_it = std::find_if(_clients.begin(), _clients.end(),
                        [fd = _poll_fds[i].fd](const std::shared_ptr<Client>& c) { return c->getClientFd() == fd; });
                    if (client_it != _clients.end()) {
                        disconnectClient(*client_it, "Connection error");
                    }
                    cleanupFd(i);
                    --i;
                    continue;
                }
            }

            if (!(_poll_fds[i].revents & POLLIN))
			{
                continue;
            }

            if (_poll_fds[i].fd == _serverSocket)
			{
                handleNewConnection();
            }
            else
			{
                if (!handleClientData(i))
				{
                        cleanupFd(i);
                        --i;
                }
            }
        }
    }
	shutdownServer();
}

void Server::disconnectClient(std::shared_ptr<Client>& client, const std::string& reason) {
    if (!client) {
        std::cerr << "[Zorg] Attempted to disconnect null client" << std::endl;
        return;
    }
    
    int clientFd = client->getClientFd();
    if (clientFd < 0) {
        std::cerr << "[Zorg] Attempted to disconnect client with invalid fd" << std::endl;
        return;
    }
    
    // Store client info we'll need after modifications
    std::string nick = client->getNick();
    std::string user = client->getUser();
    std::string host = client->getHost();
    
    // Notify other clients
    std::string quitBroadcast = ":" + nick + "!" + user + "@" + host + " QUIT :" + reason + "\r\n";
    for (auto& c : _clients) {
        if (c && c->getClientFd() != clientFd && c->getClientFd() >= 0) {
            SendToClient(c, quitBroadcast);
        }
    }
    
    // Notify the disconnecting client
    SendToClient(client, ":" + _name + " ERROR :Closing Link: " + nick + " " + reason + "\r\n");
    
    // Close the socket
    shutdown(clientFd, SHUT_WR);
    close(clientFd);
    
    // Remove client from all channels
    std::vector<std::string> emptyChannels;
    for (auto& [channelName, channel] : _channels) {
        if (channel->isMember(client)) {
            if (channel->isOperator(client)) {
                channel->removeOperator(client, nullptr);
            }
            channel->removeMember(client);
            
            // If channel is now empty, mark it for removal
            if (channel->isChannelEmpty()) {
                emptyChannels.push_back(channelName);
            }
        }
    }
    
    // Remove empty channels
    for (const auto& channelName : emptyChannels) {
        _channels.erase(channelName);
    }
    
    // Remove from _clients vector using a safe approach
    _clients.erase(
        std::remove_if(_clients.begin(), _clients.end(),
            [fd = clientFd](const std::shared_ptr<Client>& c) { return c->getClientFd() == fd; }),
        _clients.end()
    );
}

void Server::shutdownServer() {
    std::cout << "Performing server cleanup..." << std::endl;
    
    // Create a copy of the clients vector to safely iterate over
    std::vector<std::shared_ptr<Client>> clientsCopy = _clients;
    
    // Disconnect each client directly without modifying _clients yet
    for (auto& client : clientsCopy) {
        if (client) {  // Ensure the client pointer is valid
            // Send the shutdown message directly
            if (client->getClientFd() >= 0) {
                std::string nick = client->getNick();
                SendToClient(client, ":" + _name + " ERROR :Closing Link: " + nick + " Server is shutting down\r\n");
                shutdown(client->getClientFd(), SHUT_WR);
                close(client->getClientFd());
            }
        }
    }
    
    // Now we can safely clear all collections
    _clients.clear();
    _channels.clear();
    _poll_fds.clear();
    
    // Close server socket
    if (_serverSocket >= 0) {
        close(_serverSocket);
        _serverSocket = -1;
    }
    
    std::cout << "Server shutdown complete" << std::endl;
}

void Server::checkClientTimeouts() {
    time_t currentTime = time(NULL);
    const time_t timeout = 300; // 5 minutes in seconds
    const time_t pingTimeout = 10; // 10 seconds to respond to PING

    // Create a vector to store clients that need to be disconnected
    std::vector<std::shared_ptr<Client>> clientsToDisconnect;

    for (auto& client : _clients) {
        if (client->getAwaitingPong()) {
            if ((currentTime - client->getPingTime()) > pingTimeout) {
                clientsToDisconnect.push_back(client);
                continue;
            }
        }
        if ((currentTime - client->getLastActivity()) > timeout) {
            std::cout << "[Zorg] PING dead client " << client->getClientFd() << std::endl;
            SendToClient(client, "PING " + client->getNick() + "\r\n");
            client->setPingStatus(true);
        }
    }

    // Disconnect the clients
    for (auto& client : clientsToDisconnect) {
        disconnectClient(client, "Ping timeout");
    }
}

void Server::SendToClient(const std::shared_ptr<Client>& client, const std::string& message){
	    if (message.empty()) {
        std::cerr << "[Zorg] Empty message to send to client " << client->getClientFd() << std::endl;
        return;
    }
    if (message.size() > 512) {
        std::cerr << "[Zorg] Message too long to send to client " << client->getClientFd() << std::endl;
        return;
    }
    if (client->getClientFd() == -1) {
        std::cerr << "[Zorg] Invalid file descriptor for client " << client->getClientFd() << std::endl;
        return;
    }
	ssize_t bytes_sent = send(client->getClientFd(), message.c_str(), message.length(), 0);
	if (bytes_sent < 0) {
		std::cerr << "[Zorg] Send failed. Error code: " << errno << " - " << strerror(errno) << std::endl;
		std::cerr << "[Zorg] Send failed. Original string was: " << message.c_str() << std::endl;
	}
	else {
		std::cout << client->getClientFd() << " << " << message;
	}
	if (bytes_sent != static_cast<ssize_t>(message.size())) {
		std::cerr << "[Zorg] Warning: Not all bytes were sent to " << client->getClientFd() << std::endl;
	}
}

void Server::SendToChannel(const std::string& channelName, const std::string& message, std::shared_ptr<Client> sender, int code) {
    auto it = _channels.find(channelName);
    if (it == _channels.end())
        return;

    switch (code) {
        case JUST_JOINED:
            if (it->second->isMember(sender))
                SendToClient(sender, message);
            break;
        case NORMAL_MSG:
            for (const auto& member : it->second->getMembers()) {
                if (member != sender) {
                    if (it->second->isMember(sender))
                        SendToClient(member, message);
                }
            }
            break;
        case UNIVERSAL_MSG:
            for (const auto& member : it->second->getMembers()) {
                if (it->second->isMember(sender))
                    SendToClient(member, message);
            }
            break;
        default:
            break;
    }
}

void Server::handleMessage(std::shared_ptr<Client>& client, const std::string& message) {
    if (message.empty()) {
        std::cerr << "[Zorg] Empty message received from client " << client->getClientFd() << client->getNick() << std::endl;
        return;
    }
    std::istringstream stream(message);
    std::string command;
    stream >> command;

    auto handler = _commands.find(command);
    if (handler != _commands.end()) {
        handler->second(client, message);
    }
    else {
        SendToClient(client, ":" + this->_name + " 421 * " + command + " :Unknown command\r\n");
    }
}

int Server::connectionHandshake(std::shared_ptr<Client>& client, std::vector<std::string> messages, int fd) {
    if (messages.empty()) {
        return 1;
    }

    std::cout << "[Zorg] Connection Handshake for Client: " << fd << std::endl;
    for(const std::string& message : messages) {
        std::istringstream stream(message);
        std::string command;
        stream >> command;
        std::transform(command.begin(), command.end(), command.begin(), ::toupper);
        
        std::cout << client->getClientFd() << " >> " << message << std::endl;

        if (command == "PASS") {
            int grant_access = Pass(client, message);
            if (!grant_access) {
                return 0;
            }
            client->setAuthentication(true);
        } else if (command == "NICK") {
            if (!client->getAuthentication())
                SendToClient(client, ":" + _name + " 451 * :You have not registered\r\n");
            else
                Nick(client, message);
        } else if (command == "USER") {
            if (!client->getAuthentication())
                SendToClient(client, ":" + _name + " 451 * :You have not registered\r\n");
            else
                User(client, message);
        } else if (command == "CAP") {
            Cap(client, message);
        } else {
            SendToClient(client, ":" + _name + " 451 * :You have not registered\r\n");
        }
    }

    if(client->getAuthentication() && !client->getNick().empty() && !client->getUser().empty()) {
        client->setRegistration(true);
        SendToClient(client, ":" + _name + " 001 " + client->getNick() + " :Welcome to the server\r\n");
        Help(client, "HELP");
    }
    return 1;
}

// Command handlers
void Server::Nick(std::shared_ptr<Client>& client, const std::string& message) {
    std::string command, newNickname;
    std::istringstream stream(message);
    stream >> command >> newNickname;
    if (newNickname.empty()) {
        SendToClient(client, ":" + _name + " 431 " + client->getNick() + " :No nickname given\r\n");
        return;
    }

    bool valid_nickname = true;
    if(newNickname.length() > 9)
        valid_nickname = false;
    if (newNickname.empty() || !(isalpha(newNickname[0]) || newNickname[0] == '_' || newNickname[0] == '|')) {
        valid_nickname = false;
    }
    for (char c : newNickname) {
        if (!(isalnum(c) || c == '-' || c == '_' || c == '|' ||
              c == '[' || c == ']' || c == '\\' || c == '`' ||
              c == '^' || c == '{' || c == '}')) {
            valid_nickname = false;
        }
    }
    if(!valid_nickname)
    {
        if (!client->getNick().empty())
            SendToClient(client, ":" + _name + " 432 " + client->getNick() + " :Erroneous nickname\r\n");
        else
            SendToClient(client, ":" + _name + " 432 " + "*" + " :Erroneous nickname\r\n");
        return;
    }

    for (const auto& existingClient : _clients) {
        if (existingClient->getNick() == newNickname || newNickname == _name) {
            SendToClient(client, ":" + _name + " 433 * " + newNickname + " :Nickname is already in use\r\n");
            return;
        }
    }

    std::string oldNickname = client->getNick();
    client->setNick(newNickname);
    SendToClient(client, ":" + oldNickname + "!" + client->getUser() + "@" + client->getHost() + " NICK :" + client->getNick() + "\r\n");
}

void Server::User(std::shared_ptr<Client>& client, const std::string& message) {
    if(!client->getUser().empty()) {
        SendToClient(client, ":" + _name + " 462 "+ client->getNick() + " USER :You may not reregister\r\n");
        return;
    }

    std::istringstream stream(message);
    std::string command, username, hostname, servername, realname;

    stream >> command >> username >> hostname >> servername;
    getline(stream, realname);
    if (username.empty() || hostname.empty() || servername.empty() || realname.empty()) {
        SendToClient(client, ":" + _name + " 461 "+ client->getNick() + " USER :Not enough parameters\r\n");
        return;
    }

    if(!realname.empty() && realname[0] == ' ')
        realname = realname.substr(1);
    if(!realname.empty() && realname[0] == ':')
        realname = realname.substr(1);

    client->setUser(username);
    client->setRealname(realname);
}

void Server::Mode(std::shared_ptr<Client>& client, const std::string& message) {
    std::istringstream stream(message);
    std::string command, target, modeChanges, targetUser;
    stream >> command >> target >> modeChanges >> targetUser;
    bool channel = false;
    bool user = false;

    // Check if target and mode changes are provided
    if (modeChanges.empty() || target.empty())
        return;

    // Determine if target is channel or user
    if (target[0] == '#') {
        auto it = _channels.find(target);
        channel = (it != _channels.end());
        if (!channel) {
            SendToClient(client, ":" + _name + " 403 " + target + ": Invalid channel name\r\n");
            return;
        }
    } else {
        if (target == client->getNick()) {
            user = true;
        } else {
            // ERR_USERSDONTMATCH
            SendToClient(client, ":" + _name + " 502 " + ":Cannot change mode for another user\r\n");
            return;
        }
    }

    // Handle user modes
    if (user) {
        bool adding = true;
        for (char ch : modeChanges) {
            if (ch == '+')
                adding = true;
            else if (ch == '-')
                adding = false;
            else if (ch == 'i') {
                if (adding) {
                    client->addMode(ch);
                    SendToClient(client, ":" + this->_name + " 221 " + client->getNick() + " +i\r\n");
                } else {
                    client->removeMode(ch);
                }
            } else {
                // ERR_UMODEUNKNOWNFLAG
                SendToClient(client, ":" + this->_name + " 501 " + ":Unknown mode flag\r\n");
            }
        }
    }
    // Handle channel modes
    else if (channel) {
        auto it = _channels.find(target);
        if (it == _channels.end()) {
            // ERR_BADCHANMASK
            SendToClient(client, ":" + _name + " 476 " + client->getNick() + " " + target + ": invalid channel name\r\n");
            return;
        }

        bool adding = true;
        for (char ch : modeChanges) {
            std::string operatorPrivilege = " 324 ";
            std::string noOperatorPrivilege = " 482 ";
            std::string messageSyntax = ":~" + client->getNick() + "!~" + client->getNick() + "@" + client->getHost() + " MODE " + target + " " + modeChanges;

            if (ch == '+')
                adding = true;
            else if (ch == '-')
                adding = false;
            else if (ch == 'i') {
                if (it->second->isOperator(client) && it->second->isMember(client) && targetUser.empty()) {
                    ModeHelperChannel(client, it, ch, adding, operatorPrivilege);
                    SendToChannel(it->second->getName(), messageSyntax + "\r\n", client, UNIVERSAL_MSG);
                } else {
                    SendToClient(client, ":" + this->_name + noOperatorPrivilege + client->getNick() + " " + target + " :you don't have operator privileges to change modes\r\n");
                }
            }
            else if (ch == 'k') {
                if (it->second->isOperator(client) && it->second->isMember(client)) {
                    if (adding) {
                        if (!it->second->getKey().empty() && targetUser.empty()) {
                            SendToClient(client, ":" + this->_name + " 467 " + client->getNick() + " " + target + " :key already set\r\n");
                            return;
                        }
                        it->second->setKey(targetUser);
                        SendToChannel(it->second->getName(), messageSyntax + " " + targetUser + "\r\n", client, UNIVERSAL_MSG);
                    } else {
                        if (targetUser.empty())
                            return;
                        it->second->setKey("");
                        SendToChannel(it->second->getName(), messageSyntax + "\r\n", client, UNIVERSAL_MSG);
                    }
                    ModeHelperChannel(client, it, ch, adding, operatorPrivilege);
                } else {
                    SendToClient(client, ":" + this->_name + noOperatorPrivilege + client->getNick() + " " + target + " :you don't have operator privileges to change modes\r\n");
                }
            }
            else if (ch == 'o') {
                if (adding) {
                    if (it->second->isOperator(client) && it->second->isMember(client) && !targetUser.empty()) {
                        auto targetClient = std::find_if(_clients.begin(), _clients.end(),
                            [&targetUser](const std::shared_ptr<Client>& c) { return c->getNick() == targetUser; });
                            
                        if (targetClient != _clients.end()) {
                            if (it->second->isOperator(*targetClient))
                                return;
                            
                            if (it->second->addOperator(client, *targetClient)) {
                                SendToChannel(it->second->getName(), messageSyntax + " " + targetUser + "\r\n", client, UNIVERSAL_MSG);
                            }
                        } else {
                            SendToClient(client, ":" + this->_name + " 401 " + client->getNick() + " " + target + " :no such nick or channel\r\n");
                        }
                    } else {
                        SendToClient(client, ":" + this->_name + noOperatorPrivilege + client->getNick() + " " + target + " :you don't have operator privileges to change modes\r\n");
                    }
                } else {
                    if (it->second->isOperator(client) && it->second->isMember(client) && !targetUser.empty()) {
                        auto targetClient = std::find_if(_clients.begin(), _clients.end(),
                            [&targetUser](const std::shared_ptr<Client>& c) { return c->getNick() == targetUser; });
                            
                        if (targetClient != _clients.end()) {
                            if (it->second->removeOperator(client, *targetClient)) {
                                SendToChannel(it->second->getName(), messageSyntax + " " + targetUser + "\r\n", client, UNIVERSAL_MSG);
                            }
                        } else {
                            SendToClient(client, ":" + this->_name + " 401 " + client->getNick() + " " + target + " :no such nick or channel\r\n");
                        }
                    }
                }
            }
            else if (ch == 't') {
                if (it->second->isOperator(client) && it->second->isMember(client) && targetUser.empty()) {
                    SendToChannel(it->second->getName(), messageSyntax + "\r\n", client, UNIVERSAL_MSG);
                    ModeHelperChannel(client, it, ch, adding, operatorPrivilege);
                } else {
                    SendToClient(client, ":" + this->_name + noOperatorPrivilege + client->getNick() + " " + target + " :you don't have operator privileges to change modes\r\n");
                }
            }
            else if (ch == 'l') {
                if (it->second->isOperator(client) && it->second->isMember(client)) {
                    ModeHelperChannel(client, it, ch, adding, operatorPrivilege);
                    if (adding) {
                        if (targetUser.empty()) {
                            SendToClient(client, ":" + this->_name + " 461 " + client->getNick() + " " + ":not enough parameters\r\n");
                            continue;
                        }
                        if (it->second->getMaxMembers()) {
                            try {
                                size_t pos;
                                uint64_t nbMembers = std::stoull(targetUser, &pos);
                                if (pos != targetUser.length()) {
                                    SendToClient(client, ":" + this->_name + " 461 " + client->getNick() + " " + ":not enough parameters\r\n");
                                    continue;
                                }
                                if (nbMembers < it->second->getNbMembers()) {
                                    SendToClient(client, ":" + this->_name + " 471 " + client->getNick() + " " + ":channel is already over the limit\r\n");
                                }
                                it->second->limitMaxMembers(nbMembers);
                                SendToChannel(it->second->getName(), messageSyntax + " " + std::to_string(nbMembers) + "\r\n", client, UNIVERSAL_MSG);
                            } catch (const std::invalid_argument&) {
                                SendToClient(client, ":" + this->_name + " 461 " + client->getNick() + " " + ":not enough parameters\r\n");
                                continue;
                            } catch (const std::out_of_range&) {
                                SendToClient(client, ":" + this->_name + " 461 " + client->getNick() + " " + ":not enough parameters\r\n");
                                continue;
                            }
                        }
                    }
                } else {
                    SendToClient(client, ":" + this->_name + noOperatorPrivilege + client->getNick() + " " + target + " :you don't have operator privileges to change modes\r\n");
                }
            }
            else {
                // ERR_UMODEUNKNOWNFLAG
                SendToClient(client, ":" + this->_name + " 501 " + ":Unknown mode flag\r\n");
            }
        }
    }
    else {
        SendToClient(client, ":" + this->_name + " 221 " + "RPL_UMODEIS " + client->getModes() + "\r\n");
        return;
    }
}

void Server::ModeHelperChannel(std::shared_ptr<Client>& client, std::map<std::string, std::shared_ptr<Channel>>::iterator it, char mode, bool adding, std::string code) {
    if (adding) {
        it->second->setModes(mode);
        SendToClient(client, ":" + this->_name + code + it->first + " +" + mode + "\r\n");
    }
    else {
        it->second->removeMode(mode);
        SendToClient(client, ":" + this->_name + code + it->first + " -" + mode + "\r\n");
    }
}

void Server::Priv(std::shared_ptr<Client>& client, const std::string& message) {
    std::stringstream stream(message);
    std::string command, target, messageContent;
    stream >> command >> target;
    if (target.empty()) {
        SendToClient(client, ":" + _name + " 461 " + " PRIVMSG :Not enough parameters\r\n");
        return;
    }

    std::getline(stream, messageContent);
    if (!messageContent.empty() && messageContent[0] == ' ')
        messageContent = messageContent.substr(1);
    if (!messageContent.empty() && messageContent[0] == ':')
        messageContent = messageContent.substr(1);

    if (target[0] == '#') {
        auto it = _channels.find(target);
        if (it == _channels.end()) {
            SendToClient(client, ":" + _name + " 403 " + client->getNick() + " " + target + ": Invalid channel name\r\n");
            return;
        }
        if (it->second->isMember(client)) {
            SendToChannel(target, ":" + client->getNick() + " PRIVMSG " + target + " :" + messageContent + "\r\n", client, NORMAL_MSG);
        }
        else {
            std::string ERR_NOMEMBER = "Zorg: you are not a member of the channel " + target + "\r\n";
            SendToClient(client, ERR_NOMEMBER);
            SendToClient(client, ":" + _name + " 442 " + client->getNick() + " " + target + ": you're not in the channel\r\n");
        }
    }
    else {
        auto it = std::find_if(_clients.begin(), _clients.end(), 
            [&target](const std::shared_ptr<Client>& c) { return c->getNick() == target; });
        
        if (it != _clients.end()) {
            std::string formattedMessage = ":" + client->getNick() + " PRIVMSG " + target + " :" + messageContent + "\r\n";
            SendToClient(*it, formattedMessage);
        }
        else {
            SendToClient(client, ":" + _name + " 401 " + client->getNick() + " " + target + " :No such nick/channel\r\n");
        }
    }
}

void Server::Join(std::shared_ptr<Client>& client, const std::string& message) {
    std::map<std::string, std::string> allChannels = MapChannels(message);
    if (allChannels.empty())
        return;

    for (auto it_map = allChannels.begin(); it_map != allChannels.end(); ++it_map) {
        std::string channel = it_map->first;
        std::string key = it_map->second;
        if (channel.empty() || channel[0] != '#') {
            SendToClient(client, ":" + _name + " 476 " + client->getNick() + " " + channel + ":invalid channel name" + "\r\n");
            return;
        }
		// Convert requested channel to lowercase for comparison
		std::string lowerChannel = channel;
		std::transform(lowerChannel.begin(), lowerChannel.end(), lowerChannel.begin(), ::tolower);

		// Seach for an existing channel case-insensitively
		std::string actualChannelName = channel;
		for (const auto& existingChannel : _channels)
		{
			std::string existingLower = existingChannel.first;
			std::transform(existingLower.begin(), existingLower.end(), existingLower.begin(), ::tolower);
			if (existingLower == lowerChannel)
			{
				actualChannelName = existingChannel.first;
				break;
			}
		}
			
        // Create new channel if it doesn't exist
        auto [it, inserted] = _channels.try_emplace(actualChannelName, std::make_shared<Channel>(channel, key, "", false, false));

        // Check key
        if (!it->second->getKey().empty() && it->second->getKey() != key) {
            SendToClient(client, ":" + _name + " 475 " + client->getNick() + " " + actualChannelName + " :bad channel key\r\n");
            return;
        }

        // Check member limit
        if (it->second->getMaxMembers()) {
            if (it->second->getNbMembers() >= it->second->getNumberMaxMembers()) {
                SendToClient(client, ":" + _name + " 471 " + client->getNick() + " " + actualChannelName + " :channel is full\r\n");
                return;
            }
        }

        // Avoid double join
        if (it->second->isMember(client))
            return;

        it->second->addMember(client);
        SendToChannel(it->second->getName(), ":" + client->getNick() + "!~" + client->getNick() + "@" + client->getHost() + 
                     " JOIN " + it->second->getName() + "\r\n", client, NORMAL_MSG);

        if (it->second->isMember(client)) {
            SendToClient(client, ":" + client->getNick() + " JOIN " + actualChannelName + "\r\n");
            
            // Build names list
            std::string namesList;
            for (const auto& member : it->second->getMembers()) {
                if (it->second->isOperator(member))
                    namesList += "@" + member->getNick() + " ";
                else
                    namesList += member->getNick() + " ";
            }

            SendToClient(client, ":" + _name + " 353 " + client->getNick() + " = " + actualChannelName + " :" + namesList + "\r\n");
            SendToClient(client, ":" + _name + " 366 " + client->getNick() + " " + actualChannelName + " :End of /NAMES list.\r\n");

            // Send topic information
            if (!it->second->getTopic().empty())
			{
                SendToClient(client, ":" + _name + " 332 " + client->getNick() + " " + actualChannelName + " :" + it->second->getTopic() + "\r\n");
                SendToClient(client, ":" + _name + " 333 " + client->getNick() + " " + actualChannelName + " " + it->second->getSetter() + " " + 
                           std::to_string(it->second->getTopicTime()) + "\r\n");
            }
        }
    }
}

void Server::Part(std::shared_ptr<Client>& client, const std::string& message) {
    std::map<std::string, std::string> allChannels = MapChannels(message);
    if (allChannels.empty())
        return;

    for (auto it_map = allChannels.begin(); it_map != allChannels.end(); ++it_map) {
        std::string channel = it_map->first;
        std::string key = it_map->second;

        if (channel.empty() || channel[0] != '#') {
            SendToClient(client, ":" + _name + " 476 " + client->getNick() + " " + channel + " :Invalid channel syntax name\r\n");
            return;
        }

        auto it = _channels.find(channel);
        if (it == _channels.end()) {
            SendToClient(client, ":" + _name + " 403 " + client->getNick() + " " + channel + " :No such channel\r\n");
            return;
        }

        if (!it->second->isMember(client)) {
            SendToClient(client, ":" + _name + " 442 " + client->getNick() + " " + channel + " :You're not on that channel\r\n");
            return;
        }

        std::string partSyntax = ":" + client->getNick() + "!~" + client->getNick() + "@" + client->getHost() + " PART " + channel + "\r\n";
        SendToChannel(channel, partSyntax, client, NORMAL_MSG);
        
        if (it->second->isOperator(client))
            it->second->removeOperator(client, nullptr);
            
        it->second->removeMember(client);
        SendToClient(client, partSyntax);
        
        if (it->second->isChannelEmpty())
            _channels.erase(channel);
    }
}

void Server::Topic(std::shared_ptr<Client>& client, const std::string& message) {
    std::vector<std::string> tokens;
    std::istringstream stream(message);
    std::string token;
    while (stream >> token)
        tokens.push_back(token);

    if (tokens.size() < 2) {
        SendToClient(client, ":" + _name + " 461 " + client->getNick() + " TOPIC :Not enough parameters\r\n");
        return;
    }

    std::string channel_name = tokens[1];
    auto channelIt = _channels.find(channel_name);
    if (channelIt == _channels.end()) {
        SendToClient(client, ":" + _name + " 403 " + client->getNick() + " " + channel_name + " :No such channel\r\n");
        return;
    }

    std::shared_ptr<Channel>& channel = channelIt->second;
    if (!channel->isMember(client)) {
        SendToClient(client, ":" + _name + " 442 " + client->getNick() + " " + channel_name + " :You're not on that channel\r\n");
        return;
    }

    if (tokens.size() == 2) {
        if (channel->getTopic().empty())
            SendToClient(client, ":" + _name + " 331 " + client->getNick() + " " + channel_name + " :No topic is set\r\n");
        else
            SendToClient(client, ":" + _name + " 332 " + client->getNick() + " " + channel_name + " :" + channel->getTopic() + "\r\n");
        return;
    }

    if (!channel->isOperator(client)) {
        SendToClient(client, ":" + _name + " 482 " + client->getNick() + " " + channel_name + " :You're not a channel operator\r\n");
        return;
    }

    std::string new_topic = message.substr(message.find(':') + 1);
    channel->setTopic(new_topic, client->getNick());

    for (const auto& member : channel->getMembers()) {
        SendToClient(member, ":" + client->getNick() + "!" + client->getUser() + "@" + client->getHost() + 
                    " TOPIC " + channel_name + " :" + new_topic + "\r\n");
    }
}

void Server::Invite(std::shared_ptr<Client>& client, const std::string& message) {
    std::istringstream stream(message);
    std::string command, nickname, channelName;
    stream >> command >> nickname >> channelName;

    if (nickname.empty() || channelName.empty()) {
        SendToClient(client, ":" + _name + " 461 " + client->getNick() + " INVITE :Not enough parameters\r\n");
        return;
    }

    if (channelName[0] != '#') {
        SendToClient(client, ":" + _name + " 403 " + client->getNick() + " " + channelName + " :No such channel\r\n");
        return;
    }

    auto targetIt = std::find_if(_clients.begin(), _clients.end(), 
        [&nickname](const std::shared_ptr<Client>& c) { return c->getNick() == nickname; });

    if (targetIt == _clients.end()) {
        SendToClient(client, ":" + _name + " 401 " + client->getNick() + " " + nickname + " :No such nick/channel\r\n");
        return;
    }

    auto channelIt = _channels.find(channelName);
    if (channelIt != _channels.end()) {
        std::shared_ptr<Channel>& channel = channelIt->second;
        
        if (!channel->isMember(client)) {
            SendToClient(client, ":" + _name + " 442 " + client->getNick() + " " + channelName + " :You're not on that channel\r\n");
            return;
        }

        if (channel->isMember(*targetIt)) {
            SendToClient(client, ":" + _name + " 443 " + client->getNick() + " " + nickname + " " + channelName + " :is already on channel\r\n");
            return;
        }

        if (channel->getIsInviteOnly() && !channel->isOperator(client)) {
            SendToClient(client, ":" + _name + " 482 " + client->getNick() + " " + channelName + " :You're not a channel operator\r\n");
            return;
        }
    }

    SendToClient(*targetIt, ":" + client->getNick() + " INVITE " + nickname + " " + channelName + "\r\n");
    SendToClient(client, ":" + _name + " 341 " + client->getNick() + " " + nickname + " " + channelName + "\r\n");
}

void Server::Kick(std::shared_ptr<Client>& client, const std::string& message) {
    std::istringstream stream(message);
    std::string command, channelNames, usersToKick, comment;
    stream >> command >> channelNames >> usersToKick;
    std::getline(stream, comment);

    if (channelNames.empty() || usersToKick.empty()) {
        SendToClient(client, ":" + _name + " 476 " + client->getNick() + " " + channelNames + " :Invalid channel syntax\r\n");
        return;
    }

    std::vector<std::string> channels, users;
    std::istringstream channelStream(channelNames), userStream(usersToKick);
    std::string token;

    while (std::getline(channelStream, token, ','))
        channels.push_back(token);
    while (std::getline(userStream, token, ','))
        users.push_back(token);

    if (channels.size() != 1 && channels.size() != users.size()) {
        SendToClient(client, ":" + _name + " 461 " + client->getNick() + " KICK :Mismatched channels and users\r\n");
        return;
    }

    for (size_t i = 0; i < users.size(); ++i) {
        std::string channelName = (channels.size() == 1) ? channels[0] : channels[i];
        std::string userToKick = users[i];

        if (channelName[0] != '#') {
            SendToClient(client, ":" + _name + " 476 " + client->getNick() + " " + channelName + " :Invalid channel syntax\r\n");
            continue;
        }

        auto channelIt = _channels.find(channelName);
        if (channelIt == _channels.end()) {
            SendToClient(client, ":" + _name + " 403 " + client->getNick() + " " + channelName + " :No such channel\r\n");
            continue;
        }

        std::shared_ptr<Channel>& channel = channelIt->second;
        if (!channel->isMember(client)) {
            SendToClient(client, ":" + _name + " 442 " + client->getNick() + " " + channelName + " :You're not on that channel\r\n");
            continue;
        }

        if (!channel->isOperator(client)) {
            SendToClient(client, ":" + _name + " 482 " + client->getNick() + " " + channelName + " :You're not channel operator\r\n");
            continue;
        }

        auto targetIt = std::find_if(_clients.begin(), _clients.end(),
            [&userToKick](const std::shared_ptr<Client>& c) { return c->getNick() == userToKick; });

        if (targetIt == _clients.end()) {
            SendToClient(client, ":" + _name + " 401 " + client->getNick() + " " + userToKick + " :No such nick\r\n");
            continue;
        }

        std::shared_ptr<Client>& target = *targetIt;
        if (!channel->isMember(target)) {
            SendToClient(client, ":" + _name + " 441 " + client->getNick() + " " + userToKick + " " + channelName + 
                        " :They are not on that channel\r\n");
            continue;
        }
		if (!comment.empty() && comment[0] == ' ' && comment[1] == ':' && comment.size() > 2)
		{
			comment = comment.substr(2);
		}
   		else
        	comment = userToKick;

        std::string kickMessage = ":" + client->getNick() + "!" + client->getUser() + "@" + client->getHost() + 
                                " KICK " + channelName + " " + userToKick + " :" + comment + "\r\n";
        SendToChannel(channelName, kickMessage, client, UNIVERSAL_MSG);

        if (channel->isOperator(target)) {
            channel->removeOperator(target, nullptr);
            if (channel->noOperators()) {
                for (const auto& member : channel->getMembers()) {
                    if (member->getClientFd() != client->getClientFd()) {
                        channel->addOperator(nullptr, member);
                        SendToChannel(channelName, ":" + member->getNick() + " PRIVMSG " + channelName + 
                                    " :The kicked user was an operator. " + member->getNick() + 
                                    " has been promoted.\r\n", nullptr, UNIVERSAL_MSG);
                        break;
                    }
                }
            }
        }
        channel->removeMember(target);
    }
}

void Server::Who(std::shared_ptr<Client>& client, const std::string& message) {
    std::istringstream stream(message);
    std::string command, mask;
    stream >> command >> mask;

    if (mask.empty()) {
        SendToClient(client, ":" + _name + " 315 " + client->getNick() + " :End of /WHO list\r\n");
        return;
    }

    if (mask[0] == '#') {
        auto it = _channels.find(mask);
        if (it == _channels.end()) {
            SendToClient(client, ":" + _name + " 315 " + client->getNick() + " :End of /WHO list\r\n");
            return;
        }

        for (const auto& member : it->second->getMembers()) {
            std::string status = it->second->isOperator(member) ? "H@" : "H";
            std::string response = ":" + _name + " 352 " + client->getNick() + " " + mask + " " + 
                                 member->getUser() + " " + member->getHost() + " " + _name + " " + 
                                 member->getNick() + " " + status + " :0 " + member->getRealname() + "\r\n";
            SendToClient(client, response);
        }
    }

    SendToClient(client, ":" + _name + " 315 " + client->getNick() + " :End of /WHO list\r\n");
}

void Server::List(std::shared_ptr<Client>& client, const std::string& message) {
    static_cast<void>(message);
    
    for (const auto& channel : _channels) {
        SendToClient(client, ":" + _name + " 322 " + client->getNick() + " " + 
                    channel.first + " " + 
                    std::to_string(channel.second->getMembers().size()) + 
                    " :" + channel.second->getTopic() + "\r\n");
    }

    SendToClient(client, ":" + _name + " 323 " + client->getNick() + " :End of /LIST\r\n");
}

void Server::Help(std::shared_ptr<Client>& client, const std::string& message) {
    std::istringstream stream(message);
    std::string command, topic;
    stream >> command >> topic;

    if (topic.empty()) {
        // General help message using NOTICE instead of numeric replies
        SendToClient(client, ":" + _name + " NOTICE " + client->getNick() + " :Available commands:\r\n");
        SendToClient(client, ":" + _name + " NOTICE " + client->getNick() + " :  NICK     - Change your nickname\r\n");
        SendToClient(client, ":" + _name + " NOTICE " + client->getNick() + " :  USER     - Set your username and real name\r\n");
        SendToClient(client, ":" + _name + " NOTICE " + client->getNick() + " :  JOIN     - Join a channel\r\n");
        SendToClient(client, ":" + _name + " NOTICE " + client->getNick() + " :  PART     - Leave a channel\r\n");
        SendToClient(client, ":" + _name + " NOTICE " + client->getNick() + " :  PRIVMSG  - Send a message to a user or channel\r\n");
        SendToClient(client, ":" + _name + " NOTICE " + client->getNick() + " :  MODE     - Set user or channel modes\r\n");
        SendToClient(client, ":" + _name + " NOTICE " + client->getNick() + " :  TOPIC    - View or change channel topic\r\n");
        SendToClient(client, ":" + _name + " NOTICE " + client->getNick() + " :  WHOIS    - Get information about a user\r\n");
        SendToClient(client, ":" + _name + " NOTICE " + client->getNick() + " :  QUIT     - Disconnect from the server\r\n");
        SendToClient(client, ":" + _name + " NOTICE " + client->getNick() + " :  STATS    - Show server statistics\r\n");
        SendToClient(client, ":" + _name + " NOTICE " + client->getNick() + " :  WHO      - List users in a channel\r\n");
        SendToClient(client, ":" + _name + " NOTICE " + client->getNick() + " :  INVITE   - Invite a user to a channel\r\n");
        SendToClient(client, ":" + _name + " NOTICE " + client->getNick() + " :  KICK     - Remove a user from a channel\r\n");
        SendToClient(client, ":" + _name + " NOTICE " + client->getNick() + " :  HELP     - Show this help message\r\n");
        SendToClient(client, ":" + _name + " NOTICE " + client->getNick() + " :Type HELP <command> for more information about a specific command\r\n");
        SendToClient(client, ":" + _name + " NOTICE " + client->getNick() + " :Note: In irssi, commands must be prepended with /quote (e.g., /quote NICK)\r\n");
        return;
    }

    // Convert topic to uppercase for case-insensitive comparison
    std::transform(topic.begin(), topic.end(), topic.begin(), ::toupper);

    // Help messages for specific commands...
    // [Rest of the help messages remain the same, just change client.getNick() to client->getNick()]
}

void Server::Ping(std::shared_ptr<Client>& client, const std::string& message) {
    std::istringstream stream(message);
    std::string command, target;
    stream >> command >> target;

    if (target.empty()) {
        SendToClient(client, ":" + _name + " 409 " + client->getNick() + " :No origin specified\r\n");
        return;
    }

    if (target == _name) {
        SendToClient(client, "PONG " + target + "\r\n");
    } else {
        SendToClient(client, ":" + _name + " 402 " + client->getNick() + " " + target + " :No such server\r\n");
    }
}

void Server::Pong(std::shared_ptr<Client>& client, const std::string& message) {
    (void)message;
    client->setPingStatus(false);
}

void Server::Stats(std::shared_ptr<Client>& client, const std::string& message) {
    std::istringstream stream(message);
    std::string command, stat_option;
    stream >> command >> stat_option;

    if(stat_option.empty()) {
        SendToClient(client, ":" + _name + " You need to provide a specific stats option [N]\r\n");
        return;
    }
    
    if(stat_option == "N") {
        for (const auto& clients : _clients) {
            SendToClient(client, ":" + _name + " STATS N " + std::to_string(clients->getClientFd()) + " :" + clients->getNick() + "\r\n");
        }
        SendToClient(client, ":" + _name + " 219 " + stat_option + " :End of /STATS report\r\n");
    } else {
        SendToClient(client, ":" + _name + " Invalid stats option\r\n");
    }
}

void Server::Whois(std::shared_ptr<Client>& client, const std::string& message) {
    std::istringstream stream(message);
    std::string command, target;
    stream >> command >> target;

    if (target.empty()) {
        SendToClient(client, ":" + _name + " 431 " + client->getNick() + " :No nickname given\r\n");
        return;
    }

    auto targetClient = std::find_if(_clients.begin(), _clients.end(),
        [&target](const std::shared_ptr<Client>& c) { return c->getNick() == target; });

    if (targetClient == _clients.end()) {
        SendToClient(client, ":" + _name + " 401 " + client->getNick() + " " + target + " :No such nick/channel\r\n");
        return;
    }

    SendToClient(client, ":" + _name + " 311 " + client->getNick() + " " + (*targetClient)->getNick() + " " + 
                (*targetClient)->getUser() + " " + (*targetClient)->getHost() + " * :" + 
                (*targetClient)->getRealname() + "\r\n");

    SendToClient(client, ":" + _name + " 312 " + client->getNick() + " " + (*targetClient)->getNick() + 
                " " + _name + " :IRC Server\r\n");

    SendToClient(client, ":" + _name + " 318 " + client->getNick() + " " + (*targetClient)->getNick() + 
                " :End of /WHOIS list\r\n");
}

void Server::Cap(std::shared_ptr<Client>& client, const std::string& message) {
    (void)message;
    SendToClient(client, ":" + this->_name + " CAP * LS :*\r\n");
}

// Basic server getters and setters
void Server::setSocket(int socket) {
    this->_serverSocket = socket;
}

int Server::getSocket() {
    return (this->_serverSocket);
}

const sockaddr_in& Server::getServerAddr() const {
    return (this->_serverAddr);
}

void Server::setServerAddr() {
    std::memset(&this->_serverAddr, 0, sizeof(this->_serverAddr));
    this->_serverAddr.sin_family = AF_INET;
    this->_serverAddr.sin_port = htons(this->_port);
    this->_serverAddr.sin_addr.s_addr = INADDR_ANY;
}

// Message handling
std::vector<std::string> Server::splitMessages(const std::string& message) {
    std::vector<std::string> messages;
    std::istringstream stream(message);
    std::string line;
    
    while (std::getline(stream, line, '\n')) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            messages.push_back(line);
        }   
    }
    return messages;
}

std::map<std::string, std::string> Server::MapChannels(const std::string& message) {
    std::istringstream stream(message);
    std::map<std::string, std::string> channelMap;
    std::string command, channelsList, keysList;

    if (!(stream >> command) || (command != "PART" && command != "JOIN"))
        return {};

    if (!(stream >> channelsList))
        return {};

    std::getline(stream, keysList);
    if (!keysList.empty() && keysList[0] == ' ')
        keysList.erase(0, 1);

    std::vector<std::string> channels, keys;
    std::stringstream chStream(channelsList), keyStream(keysList);

    std::string token;
    while (std::getline(chStream, token, ','))
        channels.push_back(token);

    while (std::getline(keyStream, token, ','))
        keys.push_back(token);

    for (size_t i = 0; i < channels.size(); ++i) {
        std::string respectiveKey = (i < keys.size()) ? keys[i] : "";
        channelMap[channels[i]] = respectiveKey;
    }
    return channelMap;
}

// Command handlers
void Server::Quit(std::shared_ptr<Client>& client, const std::string& message) {
    std::istringstream stream(message);
    std::string command, reason;
    stream >> command >> reason;

    std::string quitMessage = "Client has disconnected";
    if (!reason.empty()) {
        quitMessage = reason;
        if (quitMessage[0] == ':')
            quitMessage = quitMessage.substr(1);
    }
    disconnectClient(client, quitMessage);
    std::cout << "[" + _name + "] Client " << client->getNick() << " has disconnected" << std::endl;
}

int Server::Pass(std::shared_ptr<Client>& client, const std::string& message) {
    std::istringstream stream(message);
    std::string command, password;

    stream >> command >> password;
    if (client->getAuthentication()) {
        SendToClient(client, ":" + _name + " 462 " + client->getNick() + " :You may not reregister\r\n");
        return 1;
    }

    if (password.empty()) {
        SendToClient(client, ":" + _name + " 461 " + client->getNick() + " PASS :Not enough parameters\r\n");
        return 0;
    }   

    if (password != _password) {
        SendToClient(client, ":" + _name + " 464 * :Password Incorrect\r\n");
        return 0;
    }
    
    std::cout << "[" + _name + "] password accepted" << std::endl;
    return 1;
}