
#include "Server.hpp"

u_int16_t	getAndCheckPortValue(std::string port_string)
{
	int port;

	try
	{
		port = std::stoi(port_string);
	}
	catch(const std::exception& e)
	{
		std::cerr << "Error: Pass in a port value between 1024–49151." << '\n';
		return (1);
	}
	if (port <= 1023 || port >= 49152)
	{
		std::cerr << "Range Error: Pass in a port value between 1024–49151." << '\n';
		return (1);
	}
	return (port);
}

int		passwordCheck(std::string password)
{
	size_t pos = password.find(' ');
	if (pos != std::string::npos)
	{
		std::cout << "Error: Password can't contain spaces." << std::endl;
		return (1);
	}
	if (password == "")
	{
		std::cout << "Error: Password can't be an empty string." << std::endl;
		return (1);
	}
	return (0);
}

int		checkArgumentCount(int argc)
{
	if (argc != 3)
	{
		std::cout << "Usage: ./ircserv <port> <password>" << std::endl;
		return (1);
	}
	else
	{
		return (0);
	}
}
