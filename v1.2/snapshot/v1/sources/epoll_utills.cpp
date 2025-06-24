
#include "epoll_utils.hpp"
#include <sys/epoll.h>
#include <fcntl.h>
#include <stdio.h> //perror

int setup_epoll(int epoll_fd, int fd, uint32_t events)
{
	struct epoll_event event;
	event.events = events; // Monitor for read events
	event.data.fd = fd; // File descriptor to monitor
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
	return 0;
}

int make_socket_unblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl failed to get flags");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl failed to set O_NONBLOCK");
        return -1;
    }
    return 0;
}
