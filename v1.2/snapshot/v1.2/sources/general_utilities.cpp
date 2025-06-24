#include "general_utilities.hpp"
#include "config.h"
#include <cctype>
#include <algorithm>
#include <string>
#include <iostream>
//#include <vector>
//#include <unordered_set>
/**
 * both functions must accept char * as main() does not accept std string
 * i have left them open as we could provide some extra checks. 
 */


// https://modern.ircdocs.horse/#connection-setup
// The standard ports for client-server connections are TCP/6667 for plaintext, and TCP/6697 for TLS connections

int validate_port(char* port_char)
{
	std::string port(port_char);
	if (! std::all_of(port.begin(), port.end(), ::isdigit))
	{
		std::cerr<<"not all characters in port provided are digits"<<std::endl;
		return errVal::FAILURE;
	}	


	//Ports in the range 49152–65535 can also be used for 
	// custom or private IRC setups, as these are reserved for dynamic or private use.
	// 6660–6669: These are the most commonly used ports for standard, unencrypted IRC connections.
	// 6697: This is the standard port for IRC connections secured with SSL/TLS encryption.
	int test = stoi(port);
	if (test < 6659 || test > 6670)
	{
		std::cerr<<"ERROR:port number provided is out of range (6660-6669)"<<std::endl;	
		return errVal::FAILURE;
	}

	// if (!port in use) checked in genereal utilities. 
	return test;
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

