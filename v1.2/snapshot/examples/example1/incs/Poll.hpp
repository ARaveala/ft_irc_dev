#pragma once

#include "Socket.hpp"
#include <vector>
#include <poll.h>
#include <unistd.h>

class Poll 
{
	public:
		Poll();
		~Poll();

		void addFd(int fd);
		void removeFd(int index);
		int waitPoll();

		pollfd& getFd(size_t index) { return _fds[index]; }
		std::vector<pollfd>& getFds() { return _fds; }
		size_t getSize() const { return _fds.size(); }

	private:
		std::vector<pollfd> _fds;
};