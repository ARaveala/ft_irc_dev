#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include "Server.hpp"
#include "serverUtilities.hpp"

int setupServerSocket(Server &server)
{
	// Communication between client and server has to be done via TCP/IP (v4 or v6)
	server.setFd(socket(AF_INET, SOCK_STREAM, 0));
	// 1. ipv4  2.what type of socket (we want stream), 3. for auto type (this case its tcp )
	if (server.getFd() == -1)
	{
		std::cout<<"something went wrong"<<std::endl;
		return 1;
	}

	// then set up non blocking 
	if (fcntl(server.getFd(), F_SETFL, O_NONBLOCK) == -1) {
		std::cerr << "Failed to set socket to non-blocking mode" << std::endl;
		close(server.getFd());
		return 1;
	}

	sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(server.getPort());
    // 4. Bind and listen
	/*try
	{
		bind(server.getFd(), (sockaddr*)&server_address, sizeof(server_address) < 0);
	}
	catch(const std::exception& e)
	{
		std::cerr << "Bind failed, perhaps in use?" << std::endl;
		close(server.getFd());
		std::cerr << e.what() << '\n';
	}*/
	
	if (bind(server.getFd(), (sockaddr*)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "Bind failed, perhaps in use?" << std::endl;
        close(server.getFd());
        return 1;
    }

    if (listen(server.getFd(), 5) < 0) {
        std::cerr << "Listen failed" << std::endl;
        close(server.getFd());
        return 1;
    }
	std::cout<<"server is listening on port"<<server.getPort()<<std::endl;
	return 0;
}