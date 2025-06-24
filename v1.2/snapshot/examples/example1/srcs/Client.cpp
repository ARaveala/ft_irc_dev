#include "Client.hpp"

Client::Client(){}

Client::Client(int client_socket, const sockaddr_in& client_addr)
	: _nickname(""), _username(""), _socket(client_socket), _IPaddress(client_addr), _isRegistered(false), _isAuthenticated(false) {}

Client::~Client(){}
