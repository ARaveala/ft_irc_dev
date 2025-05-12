#pragma once
#include <string>

// required replies
# define RPL_NICK_CHANGE(oldnick, username, nick) (":" + oldnick + "!" + username + "@localhost NICK " +  nick + "\r\n");

// error messages 

# define NICK_INUSE(nick) = (":localhost 433 "  + nick + " " + nick + "\r\n");
/**
 * @brief designed with expectation of DELETION
 * server messages, debugging and testing alternatives 
 * 
 */
# define SERVER_MSG_NICK_CHANGE(oldnick, nick) ":server NOTICE * :User " + oldnick + " changed nickname to " + nick + "\r\n";



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
