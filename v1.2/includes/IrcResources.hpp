#pragma once
#include <string>
//#include <bitset>

enum class MsgType {
    NONE = 0,
	RPL_NICK_CHANGE,
	NICKNAME_IN_USE,
};
//std::bitset<8> errorState;
// one call to call them all

//#define EXPAND_MACRO(x) x  // ✅ Expands the macro name
//#define CALL_MSG_DYNAMIC(MsgType, ...) EXPAND_MACRO(MsgType##)(__VA_ARGS__)
//#define CALL_MSG(msg, ...) MsgType(__VA_ARGS__);
#define RPL_NICK_CHANGE(oldnick, username, nick) (":" + oldnick + "!" + username + "@localhost NICK " +  nick + "\r\n")
#define NICKNAME_IN_USE(nick) (":localhost 433 "  + nick + " " + nick + "\r\n")

#define RESOLVE_MESSAGE(msgType, params) \
    ((msgType) == MsgType::RPL_NICK_CHANGE ? RPL_NICK_CHANGE(params[0], params[1], params[2]) : \
    (msgType) == MsgType::NICKNAME_IN_USE ? NICKNAME_IN_USE(params[0]) : \
    throw std::runtime_error("Unknown message type"))
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
