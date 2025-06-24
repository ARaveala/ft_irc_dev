#include "Server.hpp"

void Server::handle_signal(int sig)
{
	(void)sig;
	Server::signal = true;
}