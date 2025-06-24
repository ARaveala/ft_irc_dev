#ifndef __HEADERS_H__
#define __HEADERS_H__

#include <iostream>
#include <iomanip>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>	  // std::vector for pollfd structures in the server class
#include <map>		  // std::map for storing clients in the server class
#include <thread>	  // std::thread for running the server in a separate thread
#include <mutex>	  // std::mutex for synchronizing access to the server's clients vector
#include <algorithm>  // std::find_if for checking if a client is already connected
#include <cstdlib>	  // std::exit for exiting the server when all clients disconnect
#include <unistd.h>	  // for sleeping between client checks on UNIX systems (Linux, Mac OS X)
#include <cstdlib>	  // for std::stoi
#include <sys/socket.h> // create an endpoint for communication
#include <sys/types.h>
#include <netinet/in.h>
#include <poll.h> //
#include <fcntl.h>
#include <csignal>
#include <arpa/inet.h>
#include <netinet/in.h> 
#include <netdb.h>
#include <memory>
#include <map>
#include <regex>
#include <sys/stat.h>
#include "./colour.h"
#include "reply.h"

#define DEFAULTPORT 6667
#define NICK_MAX_LENGTH 9
#define CRLF "\r\n"

#endif