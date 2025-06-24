
#include "Server.hpp"

void signalHandler(int signum) 
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    servRunning = false;
}

int main(int argc, char **argv)
{
	u_int16_t port;

	if (checkArgumentCount(argc) == 1)
		return (1);
	if (passwordCheck(argv[2]) == 1)
		return (1);
	if ((port = getAndCheckPortValue(argv[1])) == 1)
		return (1);

	Server server(argv[2], port);
	if (server.serverInit() != 1) {
		std::cout << "Server initialization failed" << std::endl;
		return (1);
	}
	signal(SIGINT, signalHandler);
	server.serverLoop();

    return (0);
}
