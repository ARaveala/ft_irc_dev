#include "IrcMessage.hpp"
#include <iostream>
#include <sstream>
#include <cstddef>
#include <stdexcept>
#include <algorithm> // Required for std::find
//#include "epoll_utils.hpp"

#include "IrcResources.hpp"
#include <unistd.h>
#include <string.h>

// my added libs
//#include "config.h"
#include <sys/socket.h>
#include "ServerError.hpp" // incase you want to use the exception class
#include "Server.hpp"
#include "SendException.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include <regex>
//#include "ChannelManager.hpp"
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
const std::string IrcMessage::getParam(unsigned long index) const { 
	// needs to check index is not out of bounds
	//if (_paramsList.size() >= index)
	//	return "";
	return _paramsList[index];
 }

// definition of illegal nick_names ai
std::set<std::string> const IrcMessage::_illegal_nicknames = {
    "ping", "pong", "server", "root", "nick", "services", "god"
};
void IrcMessage::setType(MsgType msg, std::vector<std::string> sendParams) {
    _msgState.reset();  // empty all messages before setting a new one
    _msgState.set(static_cast<size_t>(msg));  //activate only one msg
	_activeMsg = msg;
	_params.clear();
	_params = sendParams;
}

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

bool IrcMessage::isActive(MsgType type) {
	    return _msgState.test(static_cast<size_t>(type));
}

MsgType IrcMessage::getActiveMessageType() const {
   		return _activeMsg;  // Returns the currently active message type
}

// we should enum values or alike or we can just send the correct error message straight from here ?
// check_and_set_nickname definition, std::string& nickref
bool IrcMessage::check_and_set_nickname(std::string nickname, int fd, std::map<int, std::string>& fd_to_nick, std::map<std::string, int>& nick_to_fd) {

    // 1. Check for invalid characters
	// check nickname exists
	std::cout << "#### Nickname '" << nickname  << fd << ": Empty." << std::endl;

	if (nickname.empty()) {
		std::cout << "#### Nickname '" << nickname << "' rejected for fd " << fd << ": Empty." << std::endl;
        return false;
    }
	// check nickname is all lowercase
    for (char c : nickname) {
         if (!std::islower(static_cast<unsigned char>(c))) {
			if (c == '_')
				continue;
             std::cout << "#### Nickname '" << nickname << "' rejected for fd " << fd << ": Contains non-lowercase chars." << std::endl;
             return false;
         }
    }

    std::string processed_nickname = nickname; // TODO do we need this allocation? no i dont think so 

    // 2. check legality
    if (_illegal_nicknames.count(processed_nickname) > 0) {
        std::cout << "#### Nickname '" << nickname << "' rejected for fd " << fd << ": Illegal name." << std::endl;
        return false;
    }

    // check if nickname exists for anyone
    auto nick_it = nick_to_fd.find(processed_nickname);
	if (nick_it != nick_to_fd.end()) {
        // Nickname exists. Is it the same Client trying to set their current nick?
        if (nick_it->second == fd) {
            // FD already head requested nickname.
            std::cout << "#### Nickname '" << nickname << "' for fd " << fd << ": Already set. No change needed." << std::endl;
            return true;
        } else {
			setType(MsgType::NICKNAME_IN_USE, {nickname});
            // Nickname is taken by some cunt else
            std::cout << "#### Nickname '" << nickname << "' rejected for fd " << fd << ": Already taken by fd " << nick_it->second << "." << std::endl;
            return false;
        }
    }

    // Check if the FD has an old nickname with an iterator
    auto fd_it = fd_to_nick.find(fd);
	std::string old_nickname;
    if (fd_it != fd_to_nick.end()){
        // This FD already has a nickname. We need to remove the old one from both maps.
		// find out nickname
        old_nickname = fd_it->second;
        std::cout << "#### FD " << fd << " had old nickname '" << old_nickname << "', removing entries." << std::endl;

        // Remove the old nickname -> fd entry using the old nickname as key
        // Use erase(key) which is safe even if the key somehow wasn't found
        nick_to_fd.erase(old_nickname);

        // Remove the old fd -> nickname entry using the iterator we already have
        fd_to_nick.erase(fd_it);

        std::cout << "#### Removed old nickname '" << old_nickname << "' for fd " << fd << "." << std::endl;

    } else {
        // FD does not currently have a nickname.
         std::cout << "#### FD " << fd << " does not have an existing nickname." << std::endl;
    }

    // update both maps
    std::cout << "#### Setting nickname '" << nickname << "' for fd " << fd << "." << std::endl;
	nick_to_fd.insert({processed_nickname, fd});
    fd_to_nick.insert({fd, processed_nickname});
	setType(MsgType::RPL_NICK_CHANGE, {old_nickname, "kitty", nickname});
	std::cout << "#### old_nickname '" << old_nickname << "' set successfully for fd " << fd << "." << std::endl;
    std::cout << "#### Nickname '" << nickname << "' set successfully for fd " << fd << "." << std::endl;
	std::cout << "#### param 1 '" << _params[0] << "' num 2 " << _params[1] << " number 3" << _params[2]<<std::endl;
	//nickref = _params[0];
	return true;
}

std::string IrcMessage::get_nickname(int fd, std::map<int, std::string>& fd_to_nick) const {
     auto it = fd_to_nick.find(fd);
     if (it != fd_to_nick.end()) {
         return it->second; // Return the nickname
     }
     return ""; //todo this looks odd - it returns nothing, but not eg null?
}

/*std::map<int, std::string>& IrcMessage::get_fd_to_nickname() {
	return _fd_to_nickname;
}*/

int IrcMessage::get_fd(const std::string& nickname) const {
     std::string processed_nickname = to_lowercase(nickname);

     auto it = _nickname_to_fd.find(processed_nickname);
     if (it != _nickname_to_fd.end()) {
         return it->second;
     }
     return -1; // nickname not found
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

//asdf
bool IrcMessage::parse(const std::string& rawMessage)
{
    // 1. Clear previous state
    _prefix.clear();
    _command.clear();
    _paramsList.clear();

    std::string message_content = rawMessage;
	std::cout<<"¤¤¤¤¤¤¤ showing raw message= "<<rawMessage<<" showing message_content = "
	<< message_content<<"\n";
    // Remove any trailing \r or \n characters
    // This makes the parsing more robust to different newline styles.
    while (!message_content.empty() &&
           (message_content.back() == '\r' || message_content.back() == '\n')) {
        message_content.pop_back();
    }

    if (message_content.empty()) {
        // Only newlines or empty message received
        // std::cerr << "Debug: Received empty line or only newlines." << std::endl;
        return false;
    }

    std::stringstream ss(message_content);

    std::string current_token;

    // 3. Check for prefix (starts with ':')
    // Read the first token (will skip leading spaces, which is fine)
    ss >> current_token;

    if (current_token.empty()) {
        // This should not happen if message_content is not empty, but good for robustness
        return false;
    }

    if (current_token[0] == ':') {
        _prefix = current_token.substr(1); // Store prefix (without the leading ':')

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

    std::string remainder_of_line;
    std::getline(ss, remainder_of_line); // Read all remaining characters on the line

    // Trim leading whitespace from remainder_of_line
    size_t first_non_space = remainder_of_line.find_first_not_of(" ");
    if (first_non_space == std::string::npos) {
        // No parameters or only whitespace after the command
        return true; // Parsing successful, no parameters
    }
    remainder_of_line = remainder_of_line.substr(first_non_space);

    // --- REVISED PARAMETER PARSING ---
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
    // --- END REVISED PARAMETER PARSING ---

    return true; // Parsing successful
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
    std::cout << BOLDCYAN << "  Prefix: " << RESET << "'" << YELLOW << msg.getPrefix() << RESET << "'" << std::endl;
    std::cout << BOLDGREEN << "  Command: " << RESET << "'" << MAGENTA << msg.getCommand() << RESET << "'" << std::endl;
    std::cout << BOLDBLUE << "  Parameters:" << RESET << std::endl;
    const std::vector<std::string>& params = msg.getParams();
    if (params.empty()) {
        std::cout << "    " << RED << "(No parameters)" << RESET << std::endl;
    } else {
        for (size_t i = 0; i < params.size(); ++i) {
            std::cout << "    [" << BOLDWHITE << i << RESET << "]: '" << CYAN << params[i] << RESET << "'" << std::endl;
        }
    }
    std::cout << BOLDBLACK << "---" << RESET << std::endl; // Separator for messages
}