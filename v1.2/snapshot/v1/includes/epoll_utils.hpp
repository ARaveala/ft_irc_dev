#pragma once
#include <stdint.h>
//#include <systepoll.h>
int setup_epoll(int epoll_fd, int fd, uint32_t events);
int make_socket_unblocking(int fd);