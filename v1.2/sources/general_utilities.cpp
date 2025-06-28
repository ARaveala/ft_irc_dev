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

   // 4. Strict check for ONLY common IRC ports
    if (!((port_num >= 6660 && port_num <= 6669) || port_num == 6697)) {
        std::cerr << "Error: Port " << port_num << " is not a standard IRC port." << std::endl;
        std::cerr << "Recommended to use ports 6660-6669 or 6697 for standard IRC connections." << std::endl;
        return errVal::FAILURE;
    }

    // 5. If all checks pass, return the validated port number
    return port_num;
}

// std::string validate_password(char* password_char)
// {
// 	// printf("%s", password_char);
// 	std::string password(password_char);
// 	// check that password is not empty
// 	if (password.empty())
// 	{
// 		std::cerr<<"ERROR::password provided is empty"<<std::endl;
// 		return "";
// 	}

// 	// we could give limits to password here
// 	// only chars and numbers
// 		// invisables
// 		// special characters
// 	// length
// 	return password;
// }

std::string validate_password(const char* password_char) {
    // 1. Convert to std::string for easier manipulation and const correctness
    std::string password(password_char);

    // --- BASIC CHECKS ---

    // 2. Check that password is not empty
    if (password.empty()) {
        std::cerr << "ERROR: Password provided is empty." << std::endl;
        return ""; // Indicate failure by returning an empty string  todo check this is handled in main
    }

    // 3. Trim leading/trailing whitespace and check if it becomes empty
    // If the password is just spaces, trimming will make it empty.
    // This uses a common idiom to find first/last non-whitespace char.
    size_t first_non_space = password.find_first_not_of(" \t\n\r\f\v");
    size_t last_non_space = password.find_last_not_of(" \t\n\r\f\v");

    if (std::string::npos == first_non_space) { // String contains only whitespace
        std::cerr << "ERROR: Password provided contains only whitespace." << std::endl;
        return "";
    }
    // Apply trimming
    std::string trimmed_password = password.substr(first_non_space, (last_non_space - first_non_space + 1));


    // 4. Check for minimum length
    const int MIN_PASSWORD_LENGTH = 4; // min length definition
    if (trimmed_password.length() < MIN_PASSWORD_LENGTH) {
        std::cerr << "ERROR: Password must be at least " << MIN_PASSWORD_LENGTH << " characters long." << std::endl;
        return "";
    }

    // 5. Check for maximum length (optional, but good practice)
    const int MAX_PASSWORD_LENGTH = 12; // max length definition
    if (trimmed_password.length() > MAX_PASSWORD_LENGTH) {
        std::cerr << "ERROR: Password must not exceed " << MAX_PASSWORD_LENGTH << " characters." << std::endl;
        return "";
    }

    // --- CHARACTER TYPE REQUIREMENTS ---

    bool has_lower = false;
    bool has_upper = false;
    bool has_digit = false;
    bool has_special = false;

    for (char c : trimmed_password) {
        if (std::islower(c)) has_lower = true;
        else if (std::isupper(c)) has_upper = true;
        else if (std::isdigit(c)) has_digit = true;
        else if (std::ispunct(c)) has_special = true;
        else if (std::isspace(c)) {
            std::cerr << "ERROR: Password contains invalid whitespace characters." << std::endl;
            return "";
        }
    }

    // We can customize which character types are mandatory
    // Example: Requires at least one of each: lowercase, uppercase, digit, and special char
    // if (!has_lower || !has_upper || !has_digit || !has_special) {
    //     std::cerr << "ERROR: Password must contain at least one lowercase letter, "
    //               << "one uppercase letter, one digit, and one special character." << std::endl;
    //     return "";
    // }

    // In this example we require at least 3 out of 4 character types:
    const int MIN_REQUIRED_CHAR_TYPES = 3; // define how many we will require
    int char_type_count = 0;               // count how many types we have

    if (has_lower) char_type_count++;
    if (has_upper) char_type_count++;
    if (has_digit) char_type_count++;
    if (has_special) char_type_count++;

    if (char_type_count < MIN_REQUIRED_CHAR_TYPES) {
        std::cerr << "ERROR: Password must use at least " << MIN_REQUIRED_CHAR_TYPES
                  << " of the following character types: lowercase, uppercase, digit, special character." << std::endl;
        return "";
    }

    // If all checks pass, return the trimmed and validated password
    return trimmed_password;
}
