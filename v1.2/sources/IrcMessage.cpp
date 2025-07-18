#include "IrcMessage.hpp"
#include <iostream>
#include <sstream>
#include <cstddef>
#include <stdexcept>
#include <algorithm> // Required for std::find

#include "IrcResources.hpp"
#include <unistd.h>
#include <string.h>

#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include <regex>
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
const std::string IrcMessage::getParam(unsigned long index) const { return _paramsList[index];}

// definition of illegal nick_names
std::set<std::string> const IrcMessage::_illegal_nicknames = {
       "ping", "pong", "server", "root", "nick", "services", "god",
    "admin", "operator", "op", "system", "console", "bot",
    "null", "undefined", "localhost", "irc", "help", "whois"
};


void IrcMessage::advanceCurrentMessageOffset(ssize_t bytes_sent) {
        _bytesSentForCurrentMessage += std::min(_bytesSentForCurrentMessage + bytes_sent, _messageQue.front().length());//bytes_sent;
}

size_t IrcMessage::getRemainingBytesInCurrentMessage() const {
      		if (_messageQue.empty()) return 0;
      	return  std::max((size_t)0, _messageQue.front().length() - _bytesSentForCurrentMessage); // prevent overflow and returning of neg values
}

const char* IrcMessage::getCurrentMessageCstrOffset() const {
       if (_messageQue.empty()) return nullptr;
        size_t safe_offset = std::min(_bytesSentForCurrentMessage, _messageQue.front().length());
	    return _messageQue.front().c_str() + safe_offset;
}

bool IrcMessage::isValidNickname(const std::string& nick) {
    if (nick.empty() || nick.length() > 25) return false;

    const std::string allowedSpecial = "-[]\\`^{}_";

    // first character: must be a letter or an allowed special character
    char first = nick[0];
    if (!std::isalpha(static_cast<unsigned char>(first)) && allowedSpecial.find(first) == std::string::npos)
        return false;
    // remaining characters alphanumerics or allowed special characters no emojis
    for (char c : nick) {
        if (!std::isalnum(static_cast<unsigned char>(c)) &&
            allowedSpecial.find(c) == std::string::npos)
            return false;
    }
    return true;
}

MsgType IrcMessage::check_nickname(std::string nickname, int fd, const std::map<std::string, int>& nick_to_fd) {
    auto toLower = [](const std::string& input) -> std::string {
        std::string lower;
        lower.reserve(input.size());
        for (char c : input)
            lower += std::tolower(static_cast<unsigned char>(c));
        return lower;
    };
    std::string nickname_lower = toLower(nickname);
    if (nickname.empty()){
        return MsgType::NEED_MORE_PARAMS; 
	}
	if (!isValidNickname(nickname)) { 
        return MsgType::ERR_ERRONEUSNICKNAME; 
    }
    if (_illegal_nicknames.count(nickname_lower) > 0) {
        return MsgType::ERR_ERRONEUSNICKNAME;
    }
    auto nick_it = nick_to_fd.find(nickname_lower);
    if (nick_it != nick_to_fd.end()) {
        if (nick_it->second != fd) {
            return MsgType::NICKNAME_IN_USE;
        } else {
            return MsgType::NONE;
        }
    }
    return MsgType::RPL_NICK_CHANGE;
}


/**
 * @brief Call this when a client disconnects to clean up their nickname entry.
 * @param fd file discriptor.
 * @param fd_to_nick map of fd<-nickname.
 * @return void
 */
void IrcMessage::remove_fd(int fd, std::map<int, std::string>& fd_to_nick) {
    auto it = fd_to_nick.find(fd);
    if (it != fd_to_nick.end()) {
		std::string old_nickname = it->second;
		std::cout << "#### Removing fd " << fd << " and nickname '" << old_nickname << "' due to disconnect." << std::endl;
        _nickname_to_fd.erase(old_nickname);
		fd_to_nick.erase(it);
        std::cout << "#### Cleaned up entries for fd " << fd << "." << std::endl;
    } else {
         std::cout << "#### No nickname found for fd " << fd << " upon disconnect." << std::endl;
    }
}

bool IrcMessage::parse(const std::string& rawMessage)
{
    // Clear previous state
    _prefix.clear();
    _command.clear();
    _paramsList.clear();

	// Hold rawMessage
    std::string message_content = rawMessage;

	// Printer
	// std::cout	<< "IrcMessage::parse - Raw message= "
	// 			<< rawMessage
	// 			<< " message_content = "
	// 			<< message_content
	// 			<< std::endl;

				
	// Remove any trailing \r or \n characters
    while (!message_content.empty() &&
           (message_content.back() == '\r' || message_content.back() == '\n')) {
        message_content.pop_back();
    }

	// Only newlines or empty message received
    if (message_content.empty()) {
        std::cerr << "IrcMessage::parse: Received empty line or only newlines." << std::endl;
        return false;
    }

	// Initialise
    std::stringstream	ss(message_content);
    std::string 		current_token;

    //Check for prefix (starts with ':')
    // Read the first token (will skip leading spaces, which is fine)
    ss >> current_token;

    if (current_token.empty()) {
        // This should not happen if message_content is not empty, but good for robustness
        return false;
    }

	// Store prefix without the leading ':'
	if (current_token[0] == ':') {
		_prefix = current_token.substr(1);

	// Attempt to read the command - must exist after a prefix
	if (!(ss >> _command) || _command.empty()) {
			// std::cerr << "Error: Prefix present but no command found after it." << std::endl;
			_prefix.clear(); // Clear prefix if command is missing
			return false;
	}
    } else {
        // No prefix, the first token is the command
        _command = current_token;
    }

    // Command is mandatory as per IRC RFC.
    if (_command.empty()) {
         // This check is a bit redundant if previous checks pass, but harmless.
         return false;
    }

    // 4. Process parameters
    // After reading the command, the stream pointer is at the start of parameters or end of line.
    // We need to get the rest of the line, including any leading spaces that follow the command
    // and any internal spaces within a trailing parameter.

    std::string	remainder_of_line;
    std::getline(ss, remainder_of_line); // Read all remaining characters on the line

    // Trim leading whitespace from remainder_of_line
    size_t first_non_space = remainder_of_line.find_first_not_of(" ");
    if (first_non_space == std::string::npos) {
        // No parameters or only whitespace after the command
		if (_paramsList.empty()) {
    		_paramsList.push_back("");  // Ensure at least one param exists
		}
        return true; // Parsing successful, no parameters
    }
    remainder_of_line = remainder_of_line.substr(first_non_space);

    // --- PARAMETER PARSING ---
    std::stringstream params_ss(remainder_of_line);
    std::string param_token;

    // Loop through parameters
    while (params_ss >> param_token) { // Read token by token
        if (param_token[0] == ':') {
            // Found a leading colon for a parameter. This means it's the trailing parameter.
            // All remaining content in the stringstream, starting from the character
            // after the colon, is part of this single parameter.

            // Get the part AFTER the colon
            std::string actual_trailing_content = param_token.substr(1);

            // Append any remaining content from the stringstream
            // (There might be a space after the ':token' if it's not at the end of the line)
            std::string rest_of_trailing;
            std::getline(params_ss, rest_of_trailing); // Get the rest of the line

            // Concatenate the initial part (after colon) with the rest.
            // If rest_of_trailing is not empty, it implies there was a space, so add it back.
            if (!rest_of_trailing.empty()) {
                actual_trailing_content += rest_of_trailing;
            }
            
            _paramsList.push_back(actual_trailing_content);
            break; // After processing the trailing parameter, we are done with parameters.
        } else {
            // Not a trailing parameter, just a regular middle parameter.
            _paramsList.push_back(param_token);
        }
    }
	if (_paramsList.empty()) {
    	_paramsList.push_back("");  // Ensure at least one param exists
	}
    return true; // Parsing successful
}

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
        ss << " "; // All parameters are space-separated

        // If this is the LAST parameter, it always gets a leading colon.
        // This is the standard behavior for "trailing" parameters in IRC.
        if (i == _paramsList.size() - 1) {
            ss << ":" << _paramsList[i]; // Add the colon and the parameter value
        } else {
            // Otherwise, it's a middle parameter, just add its value
            ss << _paramsList[i];
        }
    }

    // 4. Add the CRLF terminator
    ss << "\r\n";

    return ss.str();
}

// we can delete this jw
// void IrcMessage::printMessage(const IrcMessage& msg)
// {
//     std::cout << "  Prefix: '" << msg.getPrefix() << "'" << std::endl;
//     std::cout << "  Command: '" << msg.getCommand() << "'" << std::endl;
//     std::cout << "  Parameters:" << std::endl;
//     const std::vector<std::string>& params = msg.getParams();
//     if (params.empty()) {
//         std::cout << "    (No parameters)" << std::endl;
//     } else {
//         for (size_t i = 0; i < params.size(); ++i) {
//             std::cout << "    [" << i << "]: '" << params[i] << "'" << std::endl;
//         }
//     }
//     std::cout << "---" << std::endl; // Separator for messages
// }

// ANSI escape codes for colors
#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

void IrcMessage::printMessage(const IrcMessage& msg)
{
	bool useColor = isatty(fileno(stdout)); // detetrmine if stdout is a terminal
	// Define color codes
	std::string boldCyan = useColor ? BOLDCYAN : "";
	std::string boldGreen = useColor ? BOLDGREEN : "";
	std::string boldBlue = useColor ? BOLDBLUE : "";
	std::string boldBlack = useColor ? BOLDBLACK : "";
	std::string boldWhite = useColor ? BOLDWHITE : "";
	std::string magenta = useColor ? MAGENTA : "";
	std::string yellow = useColor ? YELLOW : "";
	std::string red = useColor ? RED : "";
	std::string cyan = useColor ? CYAN : "";
	std::string reset = useColor ? RESET : "";
    std::cout << boldCyan << "  Prefix: " << reset  << "'" << yellow << msg.getPrefix() << reset  << "'" << std::endl;
    std::cout << boldGreen << "  Command: " << reset  << "'" << magenta << msg.getCommand() << reset << "'" << std::endl;
    std::cout << boldBlue << "  Parameters:" << reset  << std::endl;
    const std::vector<std::string>& params = msg.getParams();
    if (params.empty()) {
        std::cout << "    " << red << "(No parameters)" << reset  << std::endl;
    } else {
        for (size_t i = 0; i < params.size(); ++i) {
            std::cout << "    [" << boldWhite << i << reset  << "]: '" << cyan << params[i] << reset  << "'" << std::endl;
        }
    }
    std::cout << boldBlack << "---" << reset  << std::endl; // Separator for messages
}