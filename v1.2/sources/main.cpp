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

/**
 * @brief This loop functions job is to keep the server running and accepting connections.
 * It will also help manage incoming messages from clients that will be redirected to Client class methods 
 * and from there too message handling class/function. 
 * 
 * @param server the instantiated server object that will be used to manage the server and its data
 * @return int 
 * 
 * @note epoll_pwait() gives us signal handling , potentially we can add signal fds to this epoll() events,
 * we might be able to just catch them here, and avoid possible interuption issues.
 * 
 * other things to consider setsocket options? would these be helpfull ?
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
	while (true)
	{
		// from epoll fd, in events struct this has niche error handling 
		int nfds = epoll_pwait(epollfd, events, config::MAX_CLIENTS, config::TIMEOUT_EPOLL, &sigmask);
		if (nfds != 0)
			std::cout << "epoll_wait returned: " << nfds << " events\n";
			/*if (errno == EINTR) {
		std::cerr << "accept() interrupted by signal, retrying..." << std::endl;
	} else if (errno == EMFILE || errno == ENFILE) {
		std::cerr << "Too many open files—server may need tuning!" << std::endl;
 	}*/
		// if nfds == -1 we have perro we should be able to print with perror.
		for (int i = 0; i < nfds; i++)
		{
			std::cout << "EPOLL event for FD " << events[i].data.fd << ": "
          << ((events[i].events & EPOLLIN) ? " READ " : "")
          << ((events[i].events & EPOLLOUT) ? " WRITE " : "")
          << ((events[i].events & EPOLLHUP) ? " HUP " : "")
          << ((events[i].events & EPOLLERR) ? " ERROR " : "")
          << std::endl;

			if (events[i].events & EPOLLIN) {
                int fd = events[i].data.fd; // Get the associated file descriptor
				
				if (fd == server.get_signal_fd()) { 
					struct signalfd_siginfo si;
					read(server.get_signal_fd(), &si, sizeof(si));
					if (si.ssi_signo == SIGUSR1) {
						std::cout << "SIGNAL received! EPOLL CAUGHT IT YAY..." << std::endl;
						// how to force certian signals to catch and handle
						//server.shutdown();
						break;
					}
				}
				if (fd == server.getFd()) {
					try {
						server.create_Client(epollfd);
						std::cout<<server.get_client_count()<<'\n'; // debugging
					} catch(const ServerException& e)	{
						server.handle_client_connection_error(e.getType());
					}
				}
				else {
					bool read_to_buffer = server.checkTimers(fd);
					if (read_to_buffer == true)
					{
						try {
							//std::cout<<" reciveing message\n";
							server.get_Client(fd)->receive_message(fd, server); // add msg object here
							//std::cout<<" message recived\n";

							//if (server.get_Client(fd)->isMsgEmpty())
							//readyEpollout(fd, server.getFd());
							std::cout<<"after ecive message ";
							for (const auto& [fd, ev] : server.get_struct_map()) {
								std::cout << "FD " << fd << " → Events: "
										<< ((ev.events & EPOLLIN) ? " READ " : "")
										<< ((ev.events & EPOLLOUT) ? " WRITE " : "")
										<< ((ev.events & EPOLLERR) ? " ERROR " : "")
										<< std::endl;
							}
						} catch(const ServerException& e) {
							if (e.getType() == ErrorType::CLIENT_DISCONNECTED) {
								server.remove_Client(epollfd, fd);
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
				//std::cout<<"creating a new ptr to client in epollout !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<"\n";
				std::shared_ptr<Client> client;
				try
				{
					client = server.get_Client(fd);  // Get the client object
					
				}
				catch(const ServerException& e)
				{
					if (e.getType() == ErrorType::NO_Client_INMAP)
						continue ;
				}
				
				
				//std::cout<<"did we manage to access the client here \n";
				while (!client->isMsgEmpty()) {
					std::string msg = client->getMsg().getQueueMessage();
					//int flag = 1;
					//socklen_t len = sizeof(flag);
					//setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&flag, sizeof(flag));

					ssize_t bytes_sent = send(fd, msg.c_str(), msg.length(), 0); //safesend
					//getsockopt(fd, SOL_SOCKET, SO_ERROR, &flag, &len);
					if (bytes_sent == -1) {
						if (errno == EAGAIN || errno == EWOULDBLOCK) break;  // 🔥 No more space, stop writing
						else perror("send error");
					}
					if (bytes_sent > 0) {
						client->getMsg().removeQueueMessage();  // ✅ Remove sent message
					}
					struct epoll_event event;
					event.events = EPOLLIN | EPOLLOUT | EPOLLET;
					event.data.fd = fd;
					epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
					// Disable EPOLLOUT if no more messages need to be sent
					/*if (client->isMsgEmpty()) {
						struct epoll_event ev;
						ev.events = EPOLLIN;  // ✅ Go back to read-only mode
						ev.data.fd = fd;
						epoll_ctl(server.getFd(), EPOLL_CTL_MOD, fd, &ev);
					}*/
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
	// set up global pointer to server for clean up
	Global::server = &server;
	
	//server.set_signal_fd(signal_mask());
	// set up server socket through utility function
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
			/*case ErrorType::SERVER_SHUTDOWN:
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
    close(server.getFd());
	return 0;
}