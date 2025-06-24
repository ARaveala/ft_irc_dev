#include "./server/Server.h"
#include <iostream>

void execbanner()
{
	std::cout << R"(
███████╗████████╗░░░░░░██╗██████╗░░█████╗░
██╔════╝╚══██╔══╝░░░░░░██║██╔══██╗██╔══██╗
█████╗░░░░░██║░░░█████╗██║██████╔╝██║░░╚═╝
██╔══╝░░░░░██║░░░╚════╝██║██╔══██╗██║░░██╗
██║░░░░░░░░██║░░░░░░░░░██║██║░░██║╚█████╔╝
╚═╝░░░░░░░░╚═╝░░░░░░░░░╚═╝╚═╝░░╚═╝░╚════╝░


░█████╗░░█████╗░███╗░░██╗██████╗░███████╗██╗░░░░░██╗███╗░░██╗
██╔══██╗██╔══██╗████╗░██║██╔══██╗██╔════╝██║░░░░░██║████╗░██║
██║░░██║███████║██╔██╗██║██║░░██║█████╗░░██║░░░░░██║██╔██╗██║
██║░░██║██╔══██║██║╚████║██║░░██║██╔══╝░░██║░░░░░██║██║╚████║
╚█████╔╝██║░░██║██║░╚███║██████╔╝███████╗███████╗██║██║░╚███║
░╚════╝░╚═╝░░╚═╝╚═╝░░╚══╝╚═════╝░╚══════╝╚══════╝╚═╝╚═╝░░╚══╝

███╗░░░███╗████████╗░█████╗░░█████╗░███████╗
████╗░████║╚══██╔══╝██╔══██╗██╔══██╗██╔════╝
██╔████╔██║░░░██║░░░██║░░██║██║░░██║█████╗░░
██║╚██╔╝██║░░░██║░░░██║░░██║██║░░██║██╔══╝░░
██║░╚═╝░██║░░░██║░░░╚█████╔╝╚█████╔╝██║░░░░░
╚═╝░░░░░╚═╝░░░╚═╝░░░░╚════╝░░╚════╝░╚═╝░░░░░

░█████╗░████████╗░█████╗░░█████╗░███████╗
██╔══██╗╚══██╔══╝██╔══██╗██╔══██╗██╔════╝
███████║░░░██║░░░██║░░██║██║░░██║█████╗░░
██╔══██║░░░██║░░░██║░░██║██║░░██║██╔══╝░░
██║░░██║░░░██║░░░╚█████╔╝╚█████╔╝██║░░░░░
╚═╝░░╚═╝░░░╚═╝░░░░╚════╝░░╚════╝░╚═╝░░░░░



	)" << std::endl;
}

void readConfigFile(std::stringstream &streamfile)
{
	std::ifstream config_file(CONFIG_FILE, std::ios::in);
	struct stat fileStat;
    if (stat(CONFIG_FILE, &fileStat) == 0)
	{
		if (S_ISDIR(fileStat.st_mode))
		{
            std::cout << RED << CONFIG_FILE;
			throw std::runtime_error(" is a directory.");
		}
	}
	if (!config_file.is_open())
	{
		std::cout << RED;
		throw std::runtime_error("Couldn't find the config file!!!");
	}
	streamfile << config_file.rdbuf();
	std::string nick,host,pass;
	std::string check_for_empty = streamfile.str();
	if (check_for_empty.empty())
	{
		std::cout << RED;
		throw std::runtime_error("Config file is empty.\nThe server needs at least one operator!!!");
	}
	while (streamfile)
	{
		streamfile >> nick >> host >> pass;
		if (nick.empty() || host.empty() || pass.empty())
		{
			std::cout << RED;
			throw std::runtime_error("Invalid config file!!!");
		}
	}
}

void convertArgs(int &port, std::string &password, char **av)
{
	int index;
	for (index = 0; av[1][index];index++)
	{
		if (!std::isdigit(av[1][index]))
		{
			std::cout << RED;
			throw std::runtime_error("Port must be a digit");
		}
	}
	port = std::stoi(av[1]);
	if (port < 0 || (port >= 0 && port <= 1024) || port > 65536)
	{
		std::cout << RED;
		throw std::runtime_error("Invalid port number, must be between 1025 and 65536");
	}
	if (av[2])
		password = av[2];
}

int main(int ac, char **av)
{
	int port=-1;
	std::string password;	
	try
	{
		switch (ac)
		{
			// for testing purposes
			// case 1:
			// 	port = DEFAULTPORT;
			// 	break;
			// case 2:
			case 3:
				convertArgs(port, password, av);
				break;
			default:
				std::cout << RED << "Usage:\n./ircserv <Port> <Password>" << RESET << std::endl;
				return 1;
		}
	}
	catch (std::exception &e)
	{
		std::cout << e.what() << std::endl;
		std::cout << RESET;
		return 1;
	}
	execbanner();
	try
	{
		std::stringstream streamfile;
		readConfigFile(streamfile);
		Server server(port, password, streamfile);
		std::signal(SIGINT, Server::signalHandler);
		std::signal(SIGQUIT, Server::signalHandler);
		server.initServer();
	}
	catch(std::exception &exp)
	{
		std::cout << exp.what() << std::endl;
		std::cout << RESET;
		return 1;
	}
	return 0;
}
