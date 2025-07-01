#include "general_utilities.hpp"
#include "config.h"
#include <cctype>
#include <algorithm>
#include <string>
#include <iostream>
#include <limits>   // For std::numeric_limits



bool parse_and_validate_arguments(int argc, char** argv, int& port_out, std::string& password_out) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
        return false;
    }

    int port_number;
    try {
        port_number = std::stoi(argv[1]);
    } catch (const std::out_of_range&) {
        std::cerr << "Error: Port number '" << argv[1] << "' is out of integer range." << std::endl;
        return false;
    } catch (const std::invalid_argument&) {
        std::cerr << "Error: Port '" << argv[1] << "' is not a valid number." << std::endl;
        return false;
    }

    int validated_port = validate_port(argv[1]);
    if (validated_port == -1 || validated_port != port_number) {
        std::cerr << "Error: Invalid port number provided: " << argv[1] << std::endl;
        return false;
    }

    std::string password = validate_password(argv[2]);
    if (password.empty()) {
        std::cerr << "Error: Password requirements not met or invalid password provided." << std::endl;
        return false;
    }

    port_out = validated_port;
    password_out = password;
    return true;
}

int validate_port(const char* port_char) {
    std::string port_str(port_char);

    if (port_str.empty() || !std::all_of(port_str.begin(), port_str.end(), ::isdigit)) {
        std::cerr << "Error: Port '" << port_str << "' contains non-digit characters or is empty." << std::endl;
        return errVal::FAILURE;
    }

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

std::string validate_password(const char* password_char) {
    std::string password(password_char);

    if (password.empty()) {
        std::cerr << "ERROR: Password provided is empty." << std::endl;
        return "";
    }
    size_t first_non_space = password.find_first_not_of(" \t\n\r\f\v");
    size_t last_non_space = password.find_last_not_of(" \t\n\r\f\v");

    if (std::string::npos == first_non_space) { // String contains only whitespace
        std::cerr << "ERROR: Password provided contains only whitespace." << std::endl;
        return "";
    }
    std::string trimmed_password = password.substr(first_non_space, (last_non_space - first_non_space + 1));
    const int MIN_PASSWORD_LENGTH = 4;// move to config
    if (trimmed_password.length() < MIN_PASSWORD_LENGTH) {
        std::cerr << "ERROR: Password must be at least " << MIN_PASSWORD_LENGTH << " characters long." << std::endl;
        return "";
    }

    const int MAX_PASSWORD_LENGTH = 12; //move to onfig
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
