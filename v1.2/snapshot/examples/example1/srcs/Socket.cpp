#include "Socket.hpp"

Socket::Socket() : _socketFd(-1) {}

Socket::~Socket() { closeSocket(); }

void Socket::create() 
{
	_socketFd = socket(AF_INET6, SOCK_STREAM, 0);
	if (_socketFd < 0)
		throw std::runtime_error("Failed to create socket");
}

void Socket::setOptions()
{
	int optset = 0;
	if (setsockopt(_socketFd, IPPROTO_IPV6, IPV6_V6ONLY, &optset, sizeof(optset)) == -1)
		throw std::runtime_error("Failed to set IPV6_V6ONLY option");

	optset = 1;
	if (setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR, &optset, sizeof(optset)) == -1)
		throw(std::runtime_error("Failed to set option (SO_REUSEADDR) on socket"));
		
	if (setsockopt(_socketFd, SOL_SOCKET, SO_KEEPALIVE, &optset, sizeof(optset)) == -1)
		throw(std::runtime_error("Failed to set option (SO_KEEPALIVE) on socket"));
}

void Socket::setNonBlocking()
{
	if (fcntl(_socketFd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("Failed to set socket to non-blocking");
}

void Socket::bindSocket(const std::string& port)
{
	sockaddr_in6 Socket_addr;
	bzero(&Socket_addr, sizeof(Socket_addr));

	Socket_addr.sin6_family = AF_INET6;
	Socket_addr.sin6_addr = in6addr_any;
	Socket_addr.sin6_port = htons(std::stoi(port));

	if (bind(_socketFd, (struct sockaddr *)&Socket_addr, sizeof(Socket_addr)) < 0)
		throw std::runtime_error("Failed to bind IPv6 socket");
}

void Socket::startListening(int maxConnections)
{
	if (listen(_socketFd, maxConnections) < 0)
		throw std::runtime_error("Failed to listen on socket");
}

void Socket::closeSocket() {
	if (_socketFd >= 0)
		close(_socketFd);
	_socketFd = -1;
}

void Socket::setIP(const std::string &hostname) {
	struct hostent *host;
	struct in_addr **addr_list;

	host = gethostbyname(hostname.c_str());
	if (host == nullptr) {
		throw std::runtime_error("Error: Could not resolve host");
	}
	addr_list = (struct in_addr **) host->h_addr_list;
	_ip = inet_ntoa(*addr_list[0]);
}
