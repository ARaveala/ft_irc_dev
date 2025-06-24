#include "Poll.hpp"

Poll::Poll() {}

Poll::~Poll() 
{
	for (size_t i = 0; i < getSize(); ++i)
	{
		if (_fds[i].fd != -1)
			close(_fds[i].fd);
	}
	_fds.clear();
}

void Poll::addFd(int fd)
{
	pollfd newFd = {};
	newFd.fd = fd;
	newFd.events = POLLIN;
	newFd.revents = 0;
	_fds.push_back(newFd);
}

void Poll::removeFd(int index)
{
	if (index >= 0 && index < (int)_fds.size())
		_fds.erase(_fds.begin() + index);
}

int Poll::waitPoll()
{
	int pollCount = poll(_fds.data(), _fds.size(), -1);
	if (pollCount < 0)
	{
		if (errno == EINTR)
			return 0;
		perror("Poll failed");
	}
	return pollCount;
}
