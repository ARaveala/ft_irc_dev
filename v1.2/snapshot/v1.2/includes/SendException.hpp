#pragma once

#include <exception>
#include <string>
#include <sys/socket.h>

#include "config.h"

/**
 * @brief copy paste previouse and make a new error you can throw, remeber to catch the damn thing
 * 
 */
class SendException : public std::exception {
	private:
		ErrorType _type;
		std::string _message; 
	public:
		SendException(ErrorType t, const std::string& msg) : _type(t), _message(msg) {}

    	ErrorType getType() const { return _type; }

    	const char* what() const noexcept override {
			static std::string error_msg; // statics are more persistent
			error_msg = get_error_msg() +(_message.empty() ? "\n" : ": \n" + _message);
			return error_msg.c_str();
		}
	private:
		std::string get_error_msg() const {
		switch (_type) {
           	case ErrorType::BUFFER_FULL: return "Send failed: Send buffer full, try again later";
        	case ErrorType::CLIENT_DISCONNECTED: return "Send failed: Client connection reset";
			case ErrorType::BAD_FD: return "Send failed: Bad file descriptor";
			case ErrorType::BROKEN_PIPE: return "Send failed: Broken pipe (SIGPIPE ignored)";
			case ErrorType::UNKNOWN: return "Send failed:  error unknown";
           	default: return "Unknown Error if grfeat mystery ";
		}
	}
};

/* do we have to split this function away into a .cpp file, since its 1 function*/
inline void safeSend(int fd, const std::string& message) {
    ssize_t bytes_sent = send(fd, message.c_str(), message.size(), 0);
    
    if (bytes_sent == -1) {
			switch (errno) {
				case EAGAIN | EWOULDBLOCK: 
					throw SendException(ErrorType::BUFFER_FULL, "::debugging safesend");
				case ECONNRESET:
				throw SendException(ErrorType::CLIENT_DISCONNECTED, "::debugging safesend close the fd here");
				// close(client_fd);  // Cleanup if needed
				case EBADF:
					throw SendException(ErrorType::BAD_FD, "::debugging safesend");
				case EPIPE:
					throw SendException(ErrorType::BROKEN_PIPE, "::debugging safesend close fd???");	
				default:
					throw SendException(ErrorType::UNKNOWN, "::debugging safesend close fd???");	
			}
    }
}