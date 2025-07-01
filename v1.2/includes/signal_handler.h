#pragma once
#include <signal.h>

extern sig_atomic_t should_exit;
extern sigset_t sigmask;

int signal_mask();
void handle_signal(int signum);
void setup_signal_handler();
int manage_signal_events(int signal_fd);