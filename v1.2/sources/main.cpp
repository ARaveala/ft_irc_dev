//#include <chrono>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <string>
//#include <string.h>
#include <sys/signalfd.h>

#include "Server.hpp"
#include "serverUtilities.hpp"
#include "Client.hpp"
#include "general_utilities.hpp"
#include "config.h"
#include "ServerError.hpp"
#include "signal_handler.h"
#include "IrcMessage.hpp"

/**
 * @brief This loop functions job is to keep the server running and accepting connections.
 * It will also help manage incoming messages from clients that will be redirected to Client class methods
 * and from there too message handling class/function.
 * 
 * all messages are sent when an epollout event is caught 4
 *
 * @param server the instantiated server object that will be used to manage the server and its data
 * @return int
 *
 * @note epoll_pwait() gives us signal handling utalzing a signal fd, signal tests are needed
 *
 * how to test if everything is non blocking MAX CLIENTS IS MAX 510 DUE TO USING EPOLL FOR TIMER_FD ALSO,
 * SIGNAL FD AND CLIENT FDS AND SERVER FD
 */
int loop(Server &server)
{
	int epollfd = 0;
	epollfd = server.create_epollfd();

	server.set_signal_fd(signal_mask());
	server.setup_epoll(epollfd, server.get_signal_fd(), EPOLLIN);
	struct epoll_event events[config::MAX_CLIENTS];
	while (!should_exit)
	{
		int nfds = epoll_pwait(epollfd, events, config::MAX_CLIENTS, config::TIMEOUT_EPOLL, &sigmask);
		if (nfds == -1 && errno == EINTR) {
		    std::cerr << "epoll interrupted by signal" << std::endl;
			if (manage_signal_events(server.get_signal_fd()) == -1) {
				server.shutDown();
				break;
			}
    		continue;
		}
		for (int i = 0; i < nfds; i++) {
			if (events[i].events & EPOLLIN) {
                int fd = events[i].data.fd;

				if (fd == server.get_signal_fd()) {
					if (manage_signal_events(server.get_signal_fd()) == 2)
						break;
					server.shutDown();
				}
				if (fd == server.getFd()) {
					try {
						server.create_Client(epollfd);
					} catch(const ServerException& e)	{
						server.handle_client_connection_error(e.getType());
					}
				}
				else if (server.isTimerFd(fd)){
					server.checkTimers(fd);
				}
				else if (server.get_Client(fd)->get_acknowledged() && !server.get_Client(fd)->getQuit()) {
					try {
						server.handleReadEvent(server.get_Client(fd), fd);
					} catch(const ServerException& e) {
						if (e.getType() == ErrorType::CLIENT_DISCONNECTED) {
							server.remove_Client(fd);
						}
						if (e.getType() == ErrorType::NO_Client_INMAP)
							continue ;
					}
				}
			}
			if (events[i].events & EPOLLOUT) {
				int fd = events[i].data.fd;
				try {
					server.send_message(server.get_Client(fd));
					std::cout<<"client messages should be sent by now !!!!\n";
					if (server.get_Client(fd)->getQuit() == true)
						server.remove_Client(fd);
				} catch(const ServerException& e) {
					if (e.getType() == ErrorType::NO_Client_INMAP)
						continue ;
				}
			}
		}
	}
	return 0;
}
/**
 * @brief We first set up default values that will benefit us during testing.
 * we then see if we are recieving arguments from command line and validate them.
 * We set up server scoket and store required data in server class
 * @attention We set up a test client to test our servers ability to connect to irssi
 *
 * We create a loop where we read from Client socket into a buffer , print that into a log file and /or terminal
 * so we can see the irc protocol.
 * set up epoll
 *
 * @param argc argument count
 * @param argv 1 = port 2 = password
 * @return int
 */
int main(int argc, char** argv) {
    
    int port_number;
	std::string password;
	if (!parse_and_validate_arguments(argc, argv, port_number, password)) {
        return EXIT_FAILURE;
    }
	LOG_NOTICE("The requested values are:\n  Port: " + std::to_string(port_number)+  "\n  Password: " + password);
	Server server(port_number, password);
	if (setupServerSocket(server) == errVal::FAILURE) { return 0;}
	try { loop(server); }
	catch(const ServerException& e)
	{
		switch (e.getType())
		{
			case ErrorType::CLIENT_DISCONNECTED: {
				std::cerr << e.what() << " caught in main \n";
				break;
			}
			case ErrorType::EPOLL_FAILURE_0:
				std::cerr << e.what() << '\n';
				break;
			case ErrorType::EPOLL_FAILURE_1: {
				close (server.get_event_pollfd());
				std::cerr << e.what() << '\n';
				break;
			}
			case ErrorType::SOCKET_FAILURE: {
				close (server.get_event_pollfd());
				close(server.getFd());
				std::cerr << e.what() << '\n';
				break;
			}
			default: {
				std::cerr << "main Unknown error occurred" << '\n';
			}
		}

	} // this is a fail safe catch forf any exception we have forgotten to handle or thrownfrom unknown
	 catch (const std::exception& e) {
		std::cout<<"an exception has been caught in main:: "<<e.what()<<std::endl;
	}
	return 0;
}