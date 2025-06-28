#include "general_utilities.hpp"
#include "config.h"
#include <cctype>
#include <algorithm>
#include <string>
#include <iostream>
#include <limits>   // For std::numeric_limits
//#include <vector>
//#include <unordered_set>
/**
 * both functions must accept char * as main() does not accept std string
 * i have left them open as we could provide some extra checks. 
 */


// https://modern.ircdocs.horse/#connection-setup
// The standard ports for client-server connections are TCP/6667 for plaintext, and TCP/6697 for TLS connections

// int validate_port(char* port_char)
// {
// 	std::string port(port_char);
// 	if (! std::all_of(port.begin(), port.end(), ::isdigit))
// 	{
// 		std::cerr<<"not all characters in port provided are digits"<<std::endl;
// 		return errVal::FAILURE;
// 	}	


// 	//Ports in the range 49152–65535 can also be used for 
// 	// custom or private IRC setups, as these are reserved for dynamic or private use.
// 	// 6660–6669: These are the most commonly used ports for standard, unencrypted IRC connections.
// 	// 6697: This is the standard port for IRC connections secured with SSL/TLS encryption.
// 	int test = stoi(port);
// 	if (test < 6659 || test > 6670)
// 	{
// 		std::cerr<<"ERROR:port number provided is out of range (6660-6669)"<<std::endl;	
// 		return errVal::FAILURE;
// 	}

// 	// if (!port in use) checked in genereal utilities. 
// 	return test;
// }

int validate_port(const char* port_char) {
// int validate_port(char* port_char) {
    // 1. Convert to std::string for easier manipulation
    std::string port_str(port_char);

    // 2. Check if all characters are digits
    // This is crucial to prevent std::stoi from throwing std::invalid_argument for non-numeric input.
    if (port_str.empty() || !std::all_of(port_str.begin(), port_str.end(), ::isdigit)) {
        std::cerr << "Error: Port '" << port_str << "' contains non-digit characters or is empty." << std::endl;
        return errVal::FAILURE; // Use your defined error value
    }

    // 3. Convert to integer with range check during conversion
    int port_num;
    try {
        // std::stoi can throw std::out_of_range if the number is too large for an int
        port_num = std::stoi(port_str);
    } catch (const std::out_of_range& oor) {
        std::cerr << "Error: Port number '" << port_str << "' is too large to be a valid port." << std::endl;
        return errVal::FAILURE;
    }

    // 4. Add specific IRC-related port checks
    if ((port_num >= 6660 && port_num <= 6669) || port_num == 6697) {
        // This port is explicitly allowed for IRC
    } else if (port_num >= 49152 && port_num <= 65535) {
        // This port is within the dynamic/private range, generally acceptable for custom IRC setups
    } else {
        std::cerr << "Error: Port " << port_num << " is not a commonly used IRC port or in the dynamic range." << std::endl;
		std::cerr << "Reccomended to use ports 6660-6669" << std::endl;
        return errVal::FAILURE;
    }

    // If all checks pass, return the validated port number
    return port_num;
}

std::string validate_password(char* password_char)
{
	// printf("%s", password_char);
	std::string password(password_char);
	// check that password is not empty
	if (password.empty())
	{
		std::cerr<<"ERROR::password provided is empty"<<std::endl;
		return "";
	}

	// we could give limits to password here
	// only chars and numbers
		// invisables
		// special characters
	// length
	return password;
}

