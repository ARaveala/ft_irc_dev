#include <iostream>
#include <sys/signalfd.h>

#include "signal_handler.h"
#include "Server.hpp"
#include "config.h"

int should_exit = 0;
sigset_t sigmask;

int signal_mask(){
	int fd = 0;
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGUSR1);
	sigaddset(&sigmask, SIGTSTP);   // Ctrl+Z (suspend)


	if (sigprocmask(SIG_SETMASK, &sigmask, nullptr) == -1) {
		perror("sigprocmask failed");
		exit(EXIT_FAILURE);
	}
	fd = signalfd(-1, &sigmask, SFD_NONBLOCK);
	if (fd == -1) {
		perror("something wrong with sig fd");
		exit(EXIT_FAILURE);
	}
	std::cout<<"signal fd created = "<<fd<<std::endl;
	return fd;
}

void handle_signal(int signum){
	if (signum == SIGINT || signum == SIGTERM)
	{
		should_exit = 1;
		std::cout<<"signal handler called"<<std::endl;
	}
	
}
/**
 * @brief Set the up signal handler utalizing the sigaction struct that 
 * comes with signal.h , sa handler defines which function shall handle the 
 * signals(this could contain SIG_IN which would ignore just that signal).
 * sa mask defines which signals should be blocked while the handler runs. 
 * 
 */
void setup_signal_handler(){
	struct sigaction sa;
	sa.sa_handler = handle_signal; // SIG_DFL to set to default
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0; // SA_SIGINFO to provide extra information about signal recived

	sigaction(SIGINT, &sa, nullptr);
	sigaction(SIGTERM, &sa, nullptr);
	//_epollEventMap[]
}

int manage_signal_events(int signal_fd) {
    struct signalfd_siginfo si;
    ssize_t s = read(signal_fd, &si, sizeof(si));
    if (s != sizeof(si)) {
        std::cerr << "Failed to read signal info\n";
        return -1;
    }

    std::cout << "SIGNAL received! EPOLL CAUGHT IT YAY..." << std::endl;

    switch (si.ssi_signo) {
        case SIGUSR1:
            std::cout << "Custom SIGUSR1 received\n";
            return 2;

        case SIGTSTP:
            std::cout << "Caught Ctrl+Z (SIGTSTP), handling as no-op\n";
            return -2;

        default:
            std::cout << "Unhandled signal: " << si.ssi_signo << "\n";
            return -1;
    }
}
