#include "Server.hpp"

void Server::capLs(int fd, int index)
{
	if (!validateClientRegistration(fd, index))
		return ;
	sendResponse(":localhost CAP * LS :cap-notify", fd);
}