#pragma once

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <vector>
#include <algorithm>
#include <functional>
#include <map>
#include <sstream>
#include <ctime>
#include <memory>
#include "Client.hpp"
#include "Channel.hpp"
#include <signal.h>

#define SERVER_NAME "Zorg"
#define JUST_JOINED 1
#define UNIVERSAL_MSG 2
#define NORMAL_MSG 3

class Client;
class Channel;

using CommandHelper = std::function<void(std::shared_ptr<Client>&, const std::string&)>;

class Server {
private:
    static const int MAX_CLIENTS = 1024;
    int _port;
    std::string _name = SERVER_NAME;
    std::string _password;
    int _serverSocket;
    struct sockaddr_in _serverAddr;
    std::vector<struct pollfd> _poll_fds;
    std::vector<std::shared_ptr<Client>> _clients;
    std::map<std::string, std::shared_ptr<Channel>> _channels;
    std::map<std::string, CommandHelper> _commands;

    void disconnectClient(std::shared_ptr<Client>& client, const std::string& reason);
    void cleanupFd(int fd_index);
    void Help(std::shared_ptr<Client>& client, const std::string& message);
    bool handleNewConnection();
    bool handleClientData(size_t index);

public:
    Server(int port, std::string password);
    ~Server();

    int getSocket();
    const sockaddr_in& getServerAddr() const;

    void setSocket(int socket);
    void setServerAddr();

    void Run();
	void SendToClient(const std::shared_ptr<Client>& client, const std::string& message);
    void SendToChannel(const std::string& channelName, const std::string& message, std::shared_ptr<Client> sender, int code);
    void handleMessage(std::shared_ptr<Client>& client, const std::string& message);
    int connectionHandshake(std::shared_ptr<Client>& client, std::vector<std::string> messages, int fd);
    void ModeHelperChannel(std::shared_ptr<Client>& client, std::map<std::string, std::shared_ptr<Channel>>::iterator it, char mode, bool adding, std::string code);
    std::map<std::string, std::string> MapChannels(const std::string& message);

    // Command handlers
    void Ping(std::shared_ptr<Client>& client, const std::string& message);
    void Pong(std::shared_ptr<Client>& client, const std::string& message);
    void Cap(std::shared_ptr<Client>& client, const std::string& message);
    void Nick(std::shared_ptr<Client>& client, const std::string& message);
    void User(std::shared_ptr<Client>& client, const std::string& message);
    void Mode(std::shared_ptr<Client>& client, const std::string& message);
    void Join(std::shared_ptr<Client>& client, const std::string& message);
    void Quit(std::shared_ptr<Client>& client, const std::string& message);
    void Priv(std::shared_ptr<Client>& client, const std::string& message);
    int Pass(std::shared_ptr<Client>& client, const std::string& message);
    void Stats(std::shared_ptr<Client>& client, const std::string& message);
    void Whois(std::shared_ptr<Client>& client, const std::string& message);
    void Part(std::shared_ptr<Client>& client, const std::string& message);
    void Topic(std::shared_ptr<Client>& client, const std::string& message);
    void Invite(std::shared_ptr<Client>& client, const std::string& message);
    void Kick(std::shared_ptr<Client>& client, const std::string& message);
    void Who(std::shared_ptr<Client>& client, const std::string& message);
    void List(std::shared_ptr<Client>& client, const std::string& message);

    void initializeCommandHandlers();
    std::vector<std::string> splitMessages(const std::string& message);
    void checkClientTimeouts();
	bool isShutdownRequested();
	void shutdownServer();
};