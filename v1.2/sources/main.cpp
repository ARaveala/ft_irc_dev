#include <string>
#include <iostream>
//#include <sys/socket.h>
#include <netinet/in.h>

#include "Server.hpp"
#include "serverUtilities.hpp"
#include "Client.hpp"
#include "general_utilities.hpp"
#include "config.h"
//#include <unistd.h> // close()
//#include <string.h>

#include "ServerError.hpp"
#include "epoll_utils.hpp"
#include <sys/epoll.h>
#include "signal_handler.h"
#include <sys/signalfd.h>
#include "IrcMessage.hpp"
#include <string.h>
//#include "SendException.hpp"
//#include <signal.h>
// irssi commands
// / WINDOW LOG ON
// this can be opend in new terminal tail -f ~/ircTAB
// /help
// /raw open `/file of choice
// open fileofchoice
void debug_helper_print_events(struct epoll_event* events)
{
	std::cout << "EPOLL event for FD " << events->data.fd << ": "
              << ((events->events & EPOLLIN) ? " READ " : "")
              << ((events->events & EPOLLOUT) ? " WRITE " : "")
              << ((events->events & EPOLLHUP) ? " HUP " : "")
              << ((events->events & EPOLLERR) ? " ERROR " : "")
              << std::endl;
	/*std::cout << "EPOLL event for FD " << events[i].data.fd << ": "
    << ((events[i].events & EPOLLIN) ? " READ " : "")
    << ((events[i].events & EPOLLOUT) ? " WRITE " : "")
    << ((events[i].events & EPOLLHUP) ? " HUP " : "")
    << ((events[i].events & EPOLLERR) ? " ERROR " : "")
    << std::endl;*/

}
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
#include <chrono>
int loop(Server &server)
{
	int epollfd = 0;
	epollfd = server.create_epollfd();

	server.set_signal_fd(signal_mask());
	server.setup_epoll(epollfd, server.get_signal_fd(), EPOLLIN);
	struct epoll_event events[config::MAX_CLIENTS];
	while (!should_exit)
	{
		// from epoll fd, in events struct this has niche error handling
		int nfds = epoll_pwait(epollfd, events, config::MAX_CLIENTS, config::TIMEOUT_EPOLL, &sigmask);
		if (nfds == 0)
			std::cout << "epoll_wait returned: " << nfds << " events\n";
		/*if (errno == EINTR) {
			std::cerr << "accept() interrupted by signal, retrying..." << std::endl;
		} else if (errno == EMFILE || errno == ENFILE) {
			std::cerr << "Too many open files—server may need tuning!" << std::endl;
 		}*/
		// if nfds == -1 we have perro we should be able to print with perror.
		for (int i = 0; i < nfds; i++)
		{
			//debug_helper_print_events(&events[i]);
		  //if ((events[i].events & EPOLLIN) &&  (events[i].events & EPOLLOUT))
		//		std::cout<<"IF YOU SEE THIS EVERYTIME THERE IS SOMETHING NOT WORKING , WE FOUND THE SPOT  \n";
			if (events[i].events & EPOLLIN) {
                int fd = events[i].data.fd; // Get the associated file descriptor

				if (fd == server.get_signal_fd()) {
					// must test signals properly still
					if (manage_signal_events(server.get_signal_fd()) == 2)
						break;
				}
				if (fd == server.getFd()) {
					try {
						server.create_Client(epollfd);
						std::cout<<server.get_client_count()<<'\n'; // debugging
					} catch(const ServerException& e)	{
						server.handle_client_connection_error(e.getType());
					}
				}
				else if (server.get_Client(fd)->get_acknowledged() == true && server.get_Client(fd)->getQuit() == false) {
					bool read_to_buffer = server.checkTimers(fd);
					if (read_to_buffer == true)
					{
						try {
							server.handleReadEvent(fd);
						} catch(const ServerException& e) {
							if (e.getType() == ErrorType::CLIENT_DISCONNECTED) {
								server.remove_Client(fd);
								std::cout<<server.get_client_count()<<"debuggin :: this is the new client count <<<<\n"; // debugging
							}
							if (e.getType() == ErrorType::NO_Client_INMAP)
								continue ;
							// here you can catch an error of your choosing if you dont want to catch it in the message handling
						}
					}
				}
			}
			if (events[i].events & EPOLLOUT) {
				int fd = events[i].data.fd;
				try {
					//auto now = std::chrono::system_clock::now();
					//auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
					//std::cout << "DEBUG: send triggered----------------- " << ms << " ms for FD " << fd << std::endl;
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
int main(int argc, char** argv)
{
	int port_number = 6666;
	std::string password = "password";
	//if (argc != 3)
	//	exit(1);

	// if we give arguments we validate the arguments else we
	// use defaults , this must be refined later .
	if (argc == 2)
	{
		std::cout<<"need 2 arguments"<<std::endl;
		exit(1);
	}
	if (argc == 3)
	{
		port_number = validate_port(argv[1]);
		if (validate_password(argv[2]).empty())
		{
			std::cout<<"empty password"<<std::endl;
			exit(1);
		}
		password = argv[2];
	}
	else {
		std::cout<<"Attempting to use default port and password"<<std::endl;
	}
	// instantiate server object with assumed port and password
	Server server(port_number, password);
	if (setupServerSocket(server) == errVal::FAILURE)
		std::cout<<"server socket setup failure"<<std::endl;
	try {
		loop(server); //begin server loop
	}

	catch(const ServerException& e)
	{
		switch (e.getType())
		{
			case ErrorType::CLIENT_DISCONNECTED:
			{

				std::cout<<" caught in main \n";
				std::cerr << e.what() << " caught in main \n";
				break;

			}
			/*case ErrorType::SERVER_shutDown:
			{

				std::cerr << e.what() << '\n';
				break;

			}*/
			case ErrorType::EPOLL_FAILURE_0:
			// set a flag so we dont close server socket as it  is not open
				std::cerr << e.what() << '\n';
				break;
			case ErrorType::EPOLL_FAILURE_1:
			{
				close (server.get_event_pollfd());
				std::cerr << e.what() << '\n';
				break;

			}
			case ErrorType::SOCKET_FAILURE:
			{
				close (server.get_event_pollfd());
				std::cout<<"testing failure"<<std::endl;
				close(server.getFd());
				std::cerr << e.what() << '\n';
				break;

			}
			default:
			{
				std::cerr << e.what() << " caught in main \n";
				std::cerr << "main Unknown error occurred" << '\n';

			}
		}

	} // this is a fail safe catch forf any exception we have forgotten to handle or thrownfrom unknown
	 catch (const std::exception& e) {
		std::cout<<"an exception has been caught in main:: "<<e.what()<<std::endl;
	}
	// clean up
	//server.shutDown();
	return 0;
}