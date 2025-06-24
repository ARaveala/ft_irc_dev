/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fdessoy- <fdessoy-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/08 09:49:45 by akuburas          #+#    #+#             */
/*   Updated: 2025/03/17 11:22:50 by fdessoy-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* ****************************************************************************/
/*  ROFL:ROFL:ROFL:ROFL 													  */
/*          _^___      										 				  */
/* L     __/   [] \    										 			      */
/* LOL===__        \   			MY ROFLCOPTER GOES BRRRRRR				  	  */
/* L      \________]  					by fdessoy-				  			  */
/*         I   I     			(fdessoy-@student.hive.fi)				  	  */
/*        --------/   										  				  */
/* ****************************************************************************/


#include "../inc/Server.hpp"
#include "../inc/Client.hpp"
#include "../inc/Message.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>



extern "C" void sigintHandler(int sig);

bool	isDigit(std::string str)
{
	for (char ch : str)
	{
		if (!std::isdigit(ch))
			return (false);
	}
	return (true);
}

bool	parsing(std::string port, std::string password)
{
	std::string usage = "Usage: ./ircserv <port> <password>";
	if (password.empty())
	{
		std::cout << "Error: password cannot be empty" << std::endl;
		std::cout << usage << std::endl;
		return (false);
	} 
	if (port.empty())
	{
		std::cout << "Error: port cannot be empty" << std::endl;
		std::cout << usage << std::endl;
		return (false);
	}
	try
	{
		if (!isDigit(port) || std::stoi(port) < 1 || std::stoi(port) > 65535)
		{
			std::cout << "Error: port must be only digits within range 1-65535" << std::endl;
			std::cout << usage << std::endl;
			return (false);
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "Error: port must be only digits within range 1-65535" << std::endl;
		std::cout << usage << std::endl;
		return (false);
	}
	return (true);
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cout << "Usage: ./ircserv <port> <password>" << std::endl;
		return (EXIT_FAILURE);
	}
	std::string port = argv[1], password = argv[2];
	if (parsing(port, password) == false)
		return (EXIT_FAILURE);
	Server irc_server(std::stoi(port), password);
	int server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_fd < 0)
	{
		perror("Socket creation failed");
		return (EXIT_FAILURE);
	}
	irc_server.setSocket(server_fd);
	int flags = fcntl(irc_server.getSocket(), F_GETFL);
	if (flags == -1)
	{
		perror("fcntl(F_GETFL) failed");
		close(irc_server.getSocket());
		return (EXIT_FAILURE);
	}
	if (fcntl(irc_server.getSocket(), F_SETFL, flags | O_NONBLOCK) == -1)
	{
		perror("fcntl(F_SETFL) failed");
		close (irc_server.getSocket());
		return (EXIT_FAILURE);
	}
	irc_server.setServerAddr();
	int server_socket = irc_server.getSocket();
	int opt = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
	{
		perror("setsockopt failed");
		std::cout << "Error: " << errno << std::endl;
		close(server_socket);
		return (EXIT_FAILURE);
	}
	std::cout << "Server is set to non-blocking mode" << std::endl;
	if (bind(server_socket, (struct sockaddr *)&irc_server.getServerAddr(), sizeof(irc_server.getServerAddr())) < 0)
	{
		perror("bind failed");
		std::cout << "Error: " << errno << std::endl;
		close(irc_server.getSocket());
		return (EXIT_FAILURE);
	}
	if (listen(irc_server.getSocket(), 100) == -1)
	{
		perror("listening failed");
		std::cout << "Error: " << errno << std::endl;
		close(irc_server.getSocket());
		return (EXIT_FAILURE);
	}
	
	// Set up signal handler for graceful shutdown
	struct sigaction sa;
	sa.sa_handler = sigintHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("Error setting up signal handler");
		close(irc_server.getSocket());
		return EXIT_FAILURE;
	}
	
	std::cout << "Server is listening in non-blocking mode" << std::endl;
	std::cout << "Port: " << port << std::endl;
	std::cout << "Pass: " << password << std::endl;

	irc_server.Run();	
	return (0);
}