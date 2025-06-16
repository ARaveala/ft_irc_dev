#pragma once
#include <string>
//#include <bitset>


enum class MsgType {
    NONE = 0,
	WELCOME,
	HOST_INFO,
	SERVER_CREATION,
	SERVER_INFO,
	RPL_NICK_CHANGE = 353,
	NICKNAME_IN_USE = 433,
	NOT_OPERATOR = 482,     // ERR_CHANOPRIVSNEEDED,
	NOT_OPERATOR_NOTICE,
	RPL_TOPIC = 332,
	CLIENT_QUIT,
	INVALID_PASSWORD,
	PASSWORD_APPLIED,
	INVALID_INVITE,
	CHANNEL_MODE_CHANGED,
	CLIENT_MODE_CHANGED,
	USER_LIMIT_CHANGED,
	NEED_MORE_PARAMS =  461, // ERR_NEEDMOREPARAMS,
	NO_SUCH_CHANNEL = 403,  // ERR_NOSUCHCHANNEL,
	NOT_IN_CHANNEL  = 442,   // ERR_NOTONCHANNEL,
	INVALID_TARGET  = 502,   // ERR_USERSDONTMATCH (commonly used for this scenario),
	NO_SUCH_NICK = 401,     // ERR_NOSUCHNICK
	UNKNOWN_MODE  = 472,     // ERR_UNKNOWNMODE

	 // Successful replies for mode listing
    RPL_CHANNELMODEIS = 324, // Channel modes list
    RPL_UMODEIS = 221        // User mode list
};



#define RPL_NICK_CHANGE(oldnick, username, nick) (":" + oldnick + "!" + username + "@localhost NICK " +  nick + "\r\n")
#define NICKNAME_IN_USE(nick) (":localhost 433 "  + nick + " " + nick + "\r\n")

// welcome package
#define WELCOME(nickname) (":server 001 " + nickname + " :Welcome to the IRC server\r\n")
#define HOST_INFO(nickname) (":server 002 " + nickname + " :Your host is localhost, running version 1.0\r\n")
#define SERVER_CREATION(nickname) (":server 003 " + nickname + " :This server was created today\r\n")
#define SERVER_INFO(nickname) (":server 004 " + nickname + " localhost 1.0 o o\r\n")

//channel #join
#define JOIN_CHANNEL(nickname, channelName) (":" + nickname + " JOIN " + channelName + "\r\n")
#define NAMES_LIST(nickname, channelName, clientList) (":localhost 353 " + nickname + " = " + channelName + " :" + clientList + "\r\n")
#define END_NAMES_LIST(nickname, channelName) (":localhost 366 " + nickname + " " + channelName + " :End of /NAMES list\r\n")
#define CHANNEL_TOPIC(nickname, channelName) (":localhost 332 " + nickname + " " + channelName + " : topic is " + topic + "!\r\n")

//mode releated
#define NOT_OPERATOR(nickname, channelName) (":localhost 482 " + nickname + " " + channelName + " :"+ nickname +", You're not channel operator\r\n")

#define CLIENT_QUIT(nickname) (":" + nickname + " QUIT :Client disconnected\r\n")
/*#define RESOLVE_WELCOME_MESSAGE(msgType, params) \
    ((msgType == MsgType::WELCOME) ? (":server 001 " + params[0] + " :Welcome to the IRC server\r\n") : \
    (msgType == MsgType::HOST_INFO) ? (":server 002 " + params[0] + " :Your host is localhost, running version 1.0\r\n") : \
    (msgType == MsgType::SERVER_CREATION) ? (":server 003 " + params[0] + " :This server was created today\r\n") : \
    (msgType == MsgType::SERVER_INFO) ? (":server 004 " + params[0] + " localhost 1.0 o o\r\n") : \
    throw std::runtime_error("Unknown welcome message type"))*/

/*#define RESOLVE_MESSAGE(msgType, params) \
    ((msgType == MsgType::WELCOME) ? WELCOME(params[0]) : \
    (msgType == MsgType::HOST_INFO) ? HOST_INFO(params[0]) : \
    (msgType == MsgType::SERVER_CREATION) ? SERVER_CREATION(params[0]) : \
    (msgType == MsgType::SERVER_INFO) ? SERVER_INFO(params[0]) : \
    (msgType == MsgType::RPL_NICK_CHANGE && params.size() >= 3) ? RPL_NICK_CHANGE(params[0], params[1], params[2]) : \
    (msgType == MsgType::NICKNAME_IN_USE) ? NICKNAME_IN_USE(params[0]) : \
	(msgType == MsgType::CLIENT_QUIT) ? CLIENT_QUIT(params[0]): \
    "ERROR: Unknown message type or incorrect parameters")
*/



// required replies

//EMPTY_NICKNAME,
// INVALID_NICKNAME,
//    CHANNEL_NOT_FOUND,
//PERMISSION_DENIED,
// MESSAGE_TOO_LONG,

// error messages 




/**
 * @brief designed with expectation of DELETION
 * server messages, debugging and testing alternatives 
 * 
 */
#define SERVER_MSG_NICK_CHANGE(oldnick, nick) (":server NOTICE * :User " + oldnick + " changed nickname to " + nick + "\r\n")


/*template <MsgType T>
struct MsgMacro;

template <>
struct MsgMacro<MsgType::RPL_NICK_CHANGE> {
    static constexpr auto value = [](const std::vector<std::string>& params) {
		return RPL_NICK_CHANGE(params[0], );
	}
};

template <>
struct MsgMacro<MsgType::NICKNAME_IN_USE> {
    static constexpr auto value = NICKNAME_IN_USE;
};

// ✅ Function to call the correct macro dynamically
template <MsgType T, typename... Args>
std::string callMessage(Args&&... args) {
    return MsgMacro<T>::value(std::forward<Args>(args)...);
}*/

/*
// this approach takes less overhead than macros, macros are also 
// typically harder to debug and are not type safe . 
std::string formatError(int errorCode, const std::vector<std::string>& params) {
    // static const assures templates do not change and static ensures its created
	// once at compile
	static const std::unordered_map<int, std::string> errorTemplates = {
        {433, ":server 433 {NICK} {NICK}\r\n"},
        {401, ":server 401 {NICK} :No such nick/channel\r\n"},
        {403, ":server 403 {CHANNEL} :Channel does not exist\r\n"}
    };

    std::string formattedMsg = errorTemplates[errorCode];

    // Replace placeholders dynamically
    for (size_t i = 0; i < params.size(); ++i) {
        std::string placeholder = "{" + std::to_string(i) + "}";  // "{0}", "{1}", etc.
        size_t pos = formattedMsg.find(placeholder);
        if (pos != std::string::npos) {
            formattedMsg.replace(pos, placeholder.length(), params[i]);
        }
    }
    return formattedMsg;
}

std::string formatProtocolMessage(const std::string& templateStr, const std::unordered_map<std::string, std::string>& params) {
    std::string formattedMsg = templateStr;

    for (const auto& [placeholder, value] : params) {
        size_t pos;
        while ((pos = formattedMsg.find("{" + placeholder + "}")) != std::string::npos) {
            formattedMsg.replace(pos, placeholder.length() + 2, value); // `{NICK}` is 6 chars, `{CHANNEL}` is 9 chars
        }
    }

    return formattedMsg;
}

//usage example
std::string message = formatProtocolMessage(IRCMessages::ERR_NICKNAME_IN_USE, {{"NICK", "CoolUser"}});
send(clientFd, message.c_str(), message.length(), 0);
*/
