#pragma once
#include <stdint.h>

class Server;

/**
 * @brief Creates a struct for epoll events such as EPOLLIN
 * for monitoring and adds the created struct to a kernel-managed data structure,
 * this is then monitored by epoll_wait()
 * 
 * @param epollfd instance of epoll created by epoll_create1()
 * @param fd the file descriptor related to the socket
 * @param events event for epoll_wait to follow such as EPOLLIN
 * 
 * @note other events could potentially be added 
 */
int setup_epoll(int epoll_fd, int fd, uint32_t events);

/**
 * @brief We set socket to nonblocking so that recv() dosnt wait for data,
 * if it where to wait for data it would freeze our programe and potentailly stop 
 * multiple client handling.
 * int flags captures the flags , fcntl sets the flag to non blocking. 
 * 
 * @param fd the file descriptor related to the socket
 * 
 * @note Now recv() will return a -1 but it will send a errno we
 * can catch to tell it to continue that is relevant to not reciveing data yet.
 * 
 */
int make_socket_unblocking(int fd);

// int create_epollfd(Server &server);
// int createTimerFD(int timeout_seconds);
// void resetClientTimer(int timer_fd, int timeout_seconds);
// int setup_epoll_timer(int epoll_fd, int timeout_seconds);
// void readyEpollout(int client_fd, int epoll_fd);