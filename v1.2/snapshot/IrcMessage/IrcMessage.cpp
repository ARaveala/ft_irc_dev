#include "IrcMessage.hpp"
#include <iostream>
#include <sstream>
#include <cstddef>
#include <stdexcept>
#include <algorithm> // Required for std::find

// --- Constructor ---
IrcMessage::IrcMessage() {}
// --- Destructor ---
IrcMessage::~IrcMessage() {}

// --- Setters ---
void IrcMessage::setPrefix(const std::string& prefix) { _prefix = prefix; }
void IrcMessage::setCommand(const std::string& command) { _command = command; }

// --- Getters ---
const std::string& IrcMessage::getPrefix() const { return _prefix; }
const std::string& IrcMessage::getCommand() const { return _command; }
const std::vector<std::string>& IrcMessage::getParams() const { return _paramsList; }

// --- Parse Method (Corrected Parameter Handling) ---
bool IrcMessage::parse(const std::string& rawMessage)
{
    // 1. Clear previous state
    _prefix.clear();
    _command.clear();
    _paramsList.clear();

    // 2. Find the CRLF terminator (\r\n)
    size_t crlf_pos = rawMessage.find("\r\n");
    if (crlf_pos == std::string::npos) {
        // std::cerr << "Error: Message missing CRLF terminator." << std::endl; // Keep for debugging
        return false;
    }

    // Work with the message content before CRLF
    std::string message_content = rawMessage.substr(0, crlf_pos);
    if (message_content.empty()) {
         // std::cerr << "Error: Empty message content before CRLF." << std::endl;
         return false;
    }

    std::stringstream ss(message_content); // Use stringstream

    std::string first_token;
    ss >> first_token; // Read the first token

    if (first_token.empty()) {
        // Should ideally not happen if message_content is not empty
        return false;
    }

    // 3. Check for prefix (starts with ':')
    if (first_token[0] == ':') {
        _prefix = first_token.substr(1); // Store prefix (without the leading ':')

        // Attempt to read the command - must exist after a prefix
        std::string command_token;
        if (!(ss >> command_token) || command_token.empty()) {
             // std::cerr << "Error: Prefix present but no command found after it." << std::endl;
             _prefix.clear(); // Clear prefix if command is missing
             return false;
        }
        _command = command_token;

    } else {
        // No prefix, the first token is the command
        _command = first_token;
    }

    // Command is mandatory
    if (_command.empty()) {
         // std::cerr << "Error: No command found." << std::endl;
         return false;
    }

    // 4. Process parameters
    std::string params_part; // String containing all characters after the command/space
    // Use getline to read the REST of the stringstream's line
    std::getline(ss, params_part);

    // Trim leading space(s) that were between command and parameters
    size_t first_param_char_pos = params_part.find_first_not_of(" ");

    if (first_param_char_pos == std::string::npos) {
        // No parameters found (only whitespace or empty string remaining)
        return true; // Parsing successful
    }

    // The actual string containing parameters, starting from the first non-space character
    std::string actual_params_str = params_part.substr(first_param_char_pos);

    // Check for trailing parameter (starts with ':')
    if (!actual_params_str.empty() && actual_params_str[0] == ':') {
        // The rest of the string *after the colon* is a single parameter.
        // Handle the case where only ":" is left (empty trailing parameter).
         if (actual_params_str.length() > 1) {
             _paramsList.push_back(actual_params_str.substr(1)); // Store content after ':'
         } else {
             _paramsList.push_back(""); // The parameter is an empty string (case: "COMMAND :")
         }

    } else {
        // No leading colon, split parameters by spaces
        std::stringstream params_ss(actual_params_str);
        std::string param_token;
        while (params_ss >> param_token) {
            _paramsList.push_back(param_token);
        }
    }

    // If we reached here, parsing of command and parameters was successful
    return true;
}


// --- toRawString Method (Same as before, should work correctly with fixed parsing) ---
std::string IrcMessage::toRawString() const
{
    std::stringstream ss;

    // 1. Add prefix if present
    if (!_prefix.empty()) {
        ss << ":" << _prefix << " ";
    }

    // 2. Add command (command is mandatory according to structure)
    ss << _command;

    // 3. Add parameters
    for (size_t i = 0; i < _paramsList.size(); ++i) {
        // All parameters are space-separated
        ss << " ";

        // Check if this is the last parameter AND if it contains a space or is empty
        bool is_last_param = (i == _paramsList.size() - 1);
        bool needs_trailing_prefix = false;

        if (is_last_param) {
            // Check if the last parameter contains a space or is empty
            if (_paramsList[i].find(' ') != std::string::npos || _paramsList[i].empty()) {
                 needs_trailing_prefix = true;
            }
        }

        if (needs_trailing_prefix) {
            ss << ":"; // Add the trailing prefix
        }

        // Add the parameter value
        ss << _paramsList[i];
    }

    // 4. Add the CRLF terminator
    ss << "\r\n";

    return ss.str();
}
