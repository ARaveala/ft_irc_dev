#include "user.hpp"

User::User() {/*default constructor*/}

User::User(int fd) : _fd(fd) { }

User::~User() {/*default deconstructor*/}

std::string User::recieve_message(int fd) {}