#include "signal_handler.h"
#include <iostream>
#include <sys/signalfd.h>
#include "Server.hpp"
#include "config.h"
sigset_t sigmask;

//may not need anymore
int signal_mask()
{
	int fd = 0;
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGUSR1);
	if (sigprocmask(SIG_SETMASK, &sigmask, nullptr) == -1) {
		perror("sigprocmask failed");
		exit(EXIT_FAILURE);
	}
	fd = signalfd(-1, &sigmask, SFD_NONBLOCK);
	if (fd == -1)
	{
		perror("something wrong with sig fd");
		exit(EXIT_FAILURE);
	}
	std::cout<<"signal fd created = "<<fd<<std::endl;
	return fd;
}

void handle_signal(int signum)
{
	if (signum == SIGINT)// || signum == SIGTERM)
	{
		Global::server->shutdown();
		std::cout<<"signal handler called"<<std::endl;
		exit(0); // clean up required here 
	}	
}
/**
 * @brief Set the up signal handler utalizing the sigaction struct that 
 * comes with signal.h , sa handler defines which function shall handle the 
 * signals(this could contain SIG_IN which would ignore just that signal).
 * sa mask defines which signals should be blocked while the handler runs. 
 * 
 */
void setup_signal_handler()
{
	struct sigaction sa;
	sa.sa_handler = handle_signal; // SIG_DFL to set to default
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0; // SA_SIGINFO to provide extra information about signal recived

	sigaction(SIGINT, &sa, nullptr);
	sigaction(SIGTERM, &sa, nullptr);
	//_epollEventMap[]

}