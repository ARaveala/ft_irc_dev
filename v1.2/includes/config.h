#pragma once
#include <string>
#include <sstream>
#include <iostream>
#include <functional> // for short cuts
#include <deque> // for short cuts
#include <memory>
// oof i dont know if we should seperate a bunch of these into their own .h files ??
/**
 * @brief 
 * this is a header file that contains global variables and constants
 *
 * @notes: inline tells the compiler that multiple definitions of this variable
 * are allowed across different translation units, otherwise it would cause linker errors.
 * 
 * constexpr tells the compiler that the value is a constant expression, which will be evaluated 
 * at compile time. 
 * 
 * combining inline with constexpr allows us to define a variable in a header file
 * as 1 global instance of that variable. 
 * 
 * choosing not to use #define is safer as #define is not type safe and can cause
 * unexpected behavior if not used carefully.
 * 
 * alternatily we could define extern const char * instead of inline constexpr
 * but this would require us to define the variable in a .cpp file , since our values 
 * want to be imutable , this option is well suited for us.
 * 
 * this file could be seperated into config and error config, if we want to lower
 * inclusion ammounts in files, lets see
 */
#include <iostream>    // for std::cerr or std::cout
#include <fstream>     // for std::ofstream (log file writing)
#include <string>      // for std::string
#include <ctime>       // for std::time_t, std::localtime, std::strftime

#define LOG_DEBUG(msg) log_inline("DEBUG::", msg)
#define LOG_ERROR(msg) log_inline("ERROR::", msg)
#define LOG_WARN(msg)  log_inline("WARN::", msg)
#define LOG_NOTICE(msg) log_inline("NOTICE::", msg)
#define LOG_MISC(msg) log_inline("MISC::", msg)
#define LOG_VERBOSE(msg) log_inline("VERBOSE_DEBUG::", msg);
// In logger.hpp or a logger.cpp if you split
constexpr bool ENABLE_VERBOSE_LOGS = false;

static std::ofstream logfile("server.log", std::ios::app);

inline void log_inline(const std::string& level, const std::string& message) {
    if (!ENABLE_VERBOSE_LOGS && level == "VERBOSE_DEBUG::") {
        return; // Skip verbose logs if disabled
    }
	std::time_t t = std::time(nullptr);
    char timeBuf[20];
    std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", std::localtime(&t));
    logfile << "[" << timeBuf << "][" << level << "] " << message << std::endl;
}
/*class Server;
namespace Global {
	inline Server* server = nullptr;
}*/
class Channel;

enum class ErrorType {
	CLIENT_DISCONNECTED,
	SERVER_shutDown,
	EPOLL_FAILURE_0,
	EPOLL_FAILURE_1,
	SOCKET_FAILURE,
	ACCEPT_FAILURE,
	NO_Client_INMAP, // next up senderror
	NO_CHANNEL_INMAP,
	TIMER_FD_FAILURE,
	BUFFER_FULL,
	BAD_FD,
	BROKEN_PIPE,
	UNKNOWN

};

//HANDLE HERE seperate modes client and channel are seperate u twat
// reconsider bitset to bool ??? 
namespace Modes {
	enum ClientMode {
   		OPERATOR,		// 0
		FOUNDER,		// 1
		CLIENT_NONE,	// 2
	};
	enum ChannelMode {
    	USER_LIMIT,   	// 0
    	INVITE_ONLY,    // 1 
    	PASSWORD, 		// 2
    	TOPIC,   		// 3
		NONE			// 4 out of bounds saftey?
	};
	constexpr std::array<char, 4> channelModeChars = {'l', 'i', 'k', 't'};
	constexpr std::array<char, 2> clientModeChars = {'o', 'q'};
}

namespace clientPrivModes{
	enum mode {
		INVISABLE,		// 0
		PLACEHOLDER		// 1
	};
	constexpr std::array<char, 2> clientPrivModeChars = {'i'};
}

namespace IRCillegals {
    inline constexpr const char* ForbiddenChannelChars = "!:@,";
}
// Function to determine mode type

/**
 * @brief Timeout for client shouyld be 3000 as irssi sends pings every 5 minutes 
 * we can set it low to showcase how we error handle in the case of a client disconnect
 * 
 */
namespace config {
	constexpr int MAX_CLIENTS = 2;
	constexpr int TIMEOUT_CLIENT = 2000; // this should be larger than epoll timeout
	constexpr int TIMEOUT_EPOLL = -1;
	constexpr int BUFFER_SIZE = 1024;
	constexpr std::size_t CLIENT_NUM_MODES = 4;
	constexpr std::size_t CLIENT_PRIV_NUM_MODES = 1; // maybe if we want a bot this would be useful
	constexpr std::size_t CHANNEL_NUM_MODES = 5;
	constexpr std::size_t MSG_TYPE_NUM = 502;
	constexpr int MAX_LEN_CHANNAME = 25;	
	constexpr int MAX_LEN_CHANPASSWORD = 20;	
}

namespace errVal {
	constexpr int FAILURE = -1;
	constexpr int SUCCESS = 0;
}

