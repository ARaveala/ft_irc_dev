
#include "Server.hpp"

int main(int arc, char **argv)
{
	if (arc != 3)
	{
		std::cout << "Error: please arguments in the following format: \n./ircserv <port> <password>" << std::endl;
		return (1);
	}
	try {
		Server server(argv[1], argv[2]);
		std::signal(SIGINT, server.handle_signal);
		server.run();
	}
	catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		return (1);
	}
	return 0;
}