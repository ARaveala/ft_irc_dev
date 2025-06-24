#pragma once

#include <iostream>
#include <string>

// colors

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BlUE	"\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"

/*
    These functions are used to log the communication between clients and the server.
    So the evaluator can see the real time communication between the server and the clients.
    It will mimic the output style of the log file.
*/

#define LOG_CLIENT(string) {std::cout << "<< " << string << std::endl;}
#define LOG_SERVER(string) {std::cout << YELLOW << ">> " << string << RESET;}

