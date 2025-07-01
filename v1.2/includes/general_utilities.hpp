#pragma once

#include <string>

bool parse_and_validate_arguments(int argc, char** argv, int& port_out, std::string& password_out);
int validate_port(const char* port);
std::string validate_password(const char* password);