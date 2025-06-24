
/**
 * the main loop function in storage incase we break it whilst changing it
 * this also has a more c approach over a cpp aproach
 */

 /*int loop(Server &server)
{
	char buffer[1024];	
	// set up test user
	ssize_t bytes_read = 0;
	int on_off = 0;
	int testval = 0;
	// continuouse accepting will mess with the fd
	bool acknowledged = false; // just so we can get a stream of messages that come through

	// This creates an epoll instance and returns its file descriptor
	int epollfd = epoll_create1(0);
	setup_epoll(epollfd, server.getFd(), EPOLLIN);  // epollin is for incoming messages

	make_socket_unblocking(server.getFd());
	//setsockopt????
	struct epoll_event events[10]; // 10 is just for testing
	while (true)
	{
		int nfds = epoll_wait(epollfd, events, 10, -1);
		for (int i = 0; i < nfds; i++)
		{
			if (events[i].events & EPOLLIN) {
				on_off++;
                int fd = events[i].data.fd; // Get the associated file descriptor
                if (fd == server.getFd()) {
                    // Handle new incoming connection
                    int client_fd = accept(server.getFd(), nullptr, nullptr);
					make_socket_unblocking(client_fd);
					// store this fd inn user class
					if (client_fd < 0) {
                        perror("accept failed");
                    } else {
                        // Add the client socket to epoll for monitoring
                        setup_epoll(epollfd, client_fd, EPOLLIN);		
					}
                } else {
                    // handle incoming data on a client socket
					memset(buffer, 0, sizeof(buffer));
					bytes_read = recv(fd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT); // last flag makes recv non blocking 
                    if (bytes_read > 0) {
                        buffer[bytes_read] = '\0';
                        std::cout << "Received: " << buffer << std::endl;
						if (!acknowledged) {
							const char* ok_msg = ":server 001 OK\r\n";
							send(fd, ok_msg, strlen(ok_msg), 0);
							acknowledged = true; // mark as acknowledged
						}
					} else if (bytes_read == 0) {
                        std::cout << "Client disconnected. Closing socket." << std::endl;
                        close(fd); // remove the client socket
                    } else {
                        perror("read failed");
                    }
                }
			}
		}
		if (testval != on_off)
		{
			std::cout<<"on_off has changed = "<<on_off<<std::endl;
			testval = on_off;
		}
	}
	return 0;
}
if (bytes_read > 0)
{
	//buffer.resize(1024);
	std::cout<<"WE READ FROM BUFFER ---"<<buffer<<"----"<<std::endl;
	printf("%s", buffer);
	on_off++;
	if (strstr(buffer, "NICK") != nullptr) {
		const char* nick_response = ":server 001 NICK command accepted\r\n";
		send(nfds, nick_response, strlen(nick_response), 0);
	}

	// Handle PING command
	if (strstr(buffer, "PING") != nullptr) {
		std::cout<<"ping is recieved = "<<std::endl;
		std::string pong_response = "PONG " + std::string(buffer + 5) + "\r\n";
		send(nfds, pong_response.c_str(), pong_response.size(), 0);
	}
}*/