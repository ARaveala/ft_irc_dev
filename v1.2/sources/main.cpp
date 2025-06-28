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
		int nfds = epoll_pwait(epollfd, events, config::MAX_CLIENTS, config::TIMEOUT_EPOLL, &sigmask);
		if (nfds == -1 && errno == EINTR) {
		    std::cerr << "epoll interrupted by signal" << std::endl;
    		continue;
		}
		for (int i = 0; i < nfds; i++) {
			//debug_helper_print_events(&events[i]);
		  //if ((events[i].events & EPOLLIN) &&  (events[i].events & EPOLLOUT))
		//		std::cout<<"IF YOU SEE THIS EVERYTIME THERE IS SOMETHING NOT WORKING , WE FOUND THE SPOT  \n";
			if (events[i].events & EPOLLIN) {
                int fd = events[i].data.fd;

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
				else if (server.isTimerFd(fd)){
					server.checkTimers(fd);
				}
				else if (server.get_Client(fd)->get_acknowledged() && !server.get_Client(fd)->getQuit()) {
					try {
						server.handleReadEvent(server.get_Client(fd), fd);
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
    // --- 1. Enforce mandatory arguments ---
    // We expect program name (argv[0]), port (argv[1]), and password (argv[2]).
    // So, argc must be exactly 3.
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
        return EXIT_FAILURE;
    }

	// --- 2. Parse and validate port number ---
    int port_number;
    try {
        port_number = std::stoi(argv[1]); // Convert port string to int
    } catch (const std::out_of_range& oor) {
        std::cerr << "Error: Port number '" << argv[1] << "' is out of integer range." << std::endl;
        return EXIT_FAILURE;
    } catch (const std::invalid_argument& ia) {
        std::cerr << "Error: Port '" << argv[1] << "' is not a valid number." << std::endl;
        return EXIT_FAILURE;
    }

    // Now, call your more specific validate_port function.
    // If validate_port handles all parsing and returns -1 for error,
    // you might not need the try-catch above. But this way, `main`
    // has direct control over basic string-to-int conversion errors.
    // Let's assume validate_port does *additional* range/validity checks.
    int validated_port = validate_port(argv[1]); // Pass original argv[1] to it
    if (validated_port == -1 || validated_port != port_number) { // Example: if -1 means error or if validate_port changed the value
        std::cerr << "Error: Invalid port number provided: " << argv[1] << std::endl;
        // Your validate_port should ideally print a more specific error.
        return EXIT_FAILURE;
    }
    // Update port_number with the validated value from your function
    port_number = validated_port;


    // --- 3. Validate password ---
    std::string password = validate_password(argv[2]); // Call your validation function
    if (password.empty()) {
        std::cerr << "Error: Password requirements not met or invalid password provided." << std::endl;
        // Your validate_password function should ideally output specific
        // reasons for failure before returning an empty string.
        return EXIT_FAILURE;
    }
    // If validation passes, the `password` variable now holds the
    // potentially sanitized/validated password string.

    std::cout << "Welcome to the ft_irc./:" << std::endl;
    std::cout << "  Port: " << port_number << std::endl;
    std::cout << "  Password: " << password << std::endl;





	// int port_number = validate_port(argv[1]);

	// if (validate_password(argv[2]).empty())	{
	// 	std::cout<<"empty password"<<std::endl;
	// 	exit(1);
	// 	}
	// std::string password = argv[2];

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