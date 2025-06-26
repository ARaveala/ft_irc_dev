#pragma once
#include <string>
#include <sstream>
#include <iostream>
#include <functional> // for short cuts
#include <deque> // for short cuts
#include <memory>
#include <array> // For std::array

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

/*class Server;
namespace Global {
	inline Server* server = nullptr;
}*/
class Channel; // Forward declaration for Channel class

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
   		OPERATOR,		// 0 (Channel operator status for a client)
		VOICE,			// 1 (Client has voice privilege in a channel)
		FOUNDER,		// 2 (Global client founder, if applicable)
		CLIENT_NONE,	// 3 (No special client mode)
	};
	enum ChannelMode {
    	USER_LIMIT,   	// 0 (+l)
    	INVITE_ONLY,    // 1 (+i)
    	KEY_NEEDED, 	// 2 (+k)
		TOPIC,   		// 3 (+t)
		NONE			// 4 (Default/No mode set)
	};

	constexpr std::array<char, 4> channelModeChars = {'l', 'i', 'k', 't'};
	// If 'o' and 'q' are client-specific modes for channels, they should be managed via ClientMode
	// The clientPrivModeChars are for global client modes (+i for invisible, +w for wallops/notices)
}

namespace clientPrivModes{
	enum mode {
		INVISABLE,		        // 0 (+i for global client invisibility)
		RECEIVES_NOTICES_MODE,  // 1 (+w for wallops/notices) - ADDED
		PLACEHOLDER		        // 2 (Shifted index)
	};
	// Ensure this array corresponds to the enum values
	constexpr std::array<char, 2> clientPrivModeChars = {'i', 'w'}; // Updated to include 'w'
}


namespace IRCillegals
{
    inline constexpr const char* ForbiddenChannelChars = "!:@,";
}
// Function to determine mode type

/**
 * @brief Timeout for client shouyld be 3000 as irssi sends pings every 5 minutes
 * we can set it low to showcase how we error handle in the case of a client disconnect
 *
 */
namespace config {
	constexpr int MAX_CLIENTS = 10;
	constexpr int TIMEOUT_CLIENT = 2000; // this should be larger than epoll timeout
	constexpr int TIMEOUT_EPOLL = -1;
	constexpr int BUFFER_SIZE = 1024;
	constexpr std::size_t CLIENT_NUM_MODES = 4; // Updated for Modes::ClientMode
	constexpr std::size_t CLIENT_PRIV_NUM_MODES = 3; // Updated to 3 for INVISABLE, RECEIVES_NOTICES_MODE, PLACEHOLDER
	constexpr std::size_t CHANNEL_NUM_MODES = 5; // Updated for Modes::ChannelMode
	constexpr std::size_t MSG_TYPE_NUM = 502;
	constexpr int MAX_LEN_CHANNAME = 25;
	constexpr int MAX_LEN_CHANPASSWORD = 20;

	// ADDED: Max failed pings before disconnection
	constexpr int MAX_FAILED_PINGS = 3; // For example, 3 failed pings
}

namespace errVal {
	constexpr int FAILURE = -1;
	constexpr int SUCCESS = 0;
}
/**
 * @brief i do not know if all of these are needed but here
 * is a list of some irc error codes
 *
 */
namespace IRCerr {
	constexpr int ERR_NOSUCHNICK = 401;
	constexpr int ERR_NOSUCHSERVER = 402;
	constexpr int ERR_NOSUCHCHANNEL = 403;
	constexpr int ERR_CANNOTSENDTOCHAN = 404;
	constexpr int ERR_TOOMANYCHANNELS = 405;
	constexpr int ERR_WASNOSUCHNICK = 406;
	constexpr int ERR_NICKNAMEINUSE = 433;
	constexpr int ERR_NOTREGISTERED = 451;
	constexpr int ERR_NEEDMOREPARAMS = 461;
	constexpr int ERR_ALREADYREGISTRED = 462;
	constexpr int ERR_PASSWDMISMATCH = 464;
	constexpr int ERR_UNKNOWNCOMMAND = 421;
	constexpr int ERR_UNKNOWNMODE = 472;
	constexpr int ERR_NOSUCHMODE = 472;
}

namespace IRCMessage {
	inline constexpr const char* error_msg = "ERROR :Server cannot accept connection, closing.\r\n";
	//inline constexpr const char* welcome_msg = ":server 001 testClient :OK\r\n";
	inline constexpr const char* welcome_msg = ":server 001 anon :Welcome to the IRC server\r\n";
	//":server 005 anon PREFIX=(o)@\r\n"
	//":server 005 anon NETWORK=myirc\r\n";
	inline constexpr const char* ping_msg = "PING :server\r\n";
	inline constexpr const char* pong_msg = "PONG :server\r\n";
	inline constexpr const char* pass_msg = "PASS :password\r\n";
	inline std::string get_nick_msg(const std::string& nickname) {
		std::stringstream ss;
        ss << "@localhost new nickname is now :" << nickname << "\r\n";
        return ss.str(); }
	inline constexpr const char* nick_msg = "*:*!Client@localhost NICK :helooo\r\n";
	//inline constexpr const char* nick_msg = ":NICK :helooo\r\n";
	//inline constexpr const char* nick_msg = ":*!Client@localhost NICK :helooo\r\n";
	//inline constexpr const char* nick_msg = "@localhost new nickname name is now :nickname\r\n";
	inline constexpr const char* Client_msg = "Client :Clientname 0 * :realname\r\n";
}
/**
 * @brief creating short cuts so that we dont have to use large lines in parameters
 * (making it pretty i hope)
 *
 */
namespace FuncType {
	using DequeRef1 = std::function<void(std::deque<std::string>&)>;
	//using setMsgRef = std::function<void(MsgType, std::vector<std::string>&)>;
}
