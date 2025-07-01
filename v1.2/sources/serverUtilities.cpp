#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

#include "Server.hpp"
#include "serverUtilities.hpp"
#include "ServerError.hpp"
#include "config.h"

int setupServerSocket(Server &server)
{
	// Communication between client and server has to be done via TCP/IP (v4 or v6)
	server.setFd(socket(AF_INET, SOCK_STREAM, 0)); // set non blocking here ???
	// 1. ipv4  2.what type of socket (we want stream), 3. for auto type (this case its tcp )
	if (server.getFd() == errVal::FAILURE)
	{
		std::cout<<"something went wrong"<<std::endl;
		return errVal::FAILURE;
	}
	if (fcntl(server.getFd(), F_SETFL, O_NONBLOCK) == errVal::FAILURE) {
		std::cerr << "Failed to set socket to non-blocking mode" << std::endl;
		close(server.getFd());
		return errVal::FAILURE;
	}
	// set sockopt for reuse address. 
	sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY; //can this, should this be different?
    server_address.sin_port = htons(server.getPort());
	
    // 4. Bind and listen	
	if (bind(server.getFd(), (sockaddr*)&server_address, sizeof(server_address)) == errVal::FAILURE) {
        std::cerr << "Bind failed, perhaps in use?" << std::endl;
		//throw ServerException(ErrorType::CLIENT_DISCONNECTED); 
		close(server.getFd());
        return errVal::FAILURE;
    }

    if (listen(server.getFd(), 5) == errVal::FAILURE) {
        std::cerr << "Listen failed" << std::endl;
        close(server.getFd());
        return errVal::FAILURE;
    }
	std::cout<<"server is listening on port"<<server.getPort()<<std::endl;
	return 0;
}