#include "Server.hpp"

/*
	Initializes Server
	i. creates server socket
	ii. configures used fds to be reuseable
	iii. Defines server address
	iv. binds server socket
	v. starts listening
	vi. sets other minor parameters
*/
int Server::serverInit() 
{
	// linux
	// _serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	//MacOS
	_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(_serverSocket, F_SETFL, O_NONBLOCK); // for mac user
	if (_serverSocket == -1)
	{
		std::cerr << "Error creating socket" << std::endl;
		return (-1);
	}

	int opt = 1;
	setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // Set the SO_REUSEADDR option

	// Define the server address
	_serverAddr.sin_family = AF_INET;
	_serverAddr.sin_port = htons(_port);
	_serverAddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(_serverSocket, (struct sockaddr*)&_serverAddr, sizeof(_serverAddr)) == -1)
	{
		std::cerr << "Error binding socket" << std::endl;
		return (-2);
	}
	if (listen(_serverSocket, 5) == -1)
	{
		std::cerr << "Error listening on socket" << std::endl;
		return (-3);
	}

	_startTimeStr = getCurrentTime();
	std::cout << "Server started at " << _startTimeStr << std::endl;

	servRunning = true;														// set servRunning to true (global variable)

	return (1); //Success
}
