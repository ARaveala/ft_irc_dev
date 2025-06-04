#include "MessageBuilder.hpp"

#include <tuple>
#include <functional>

/*template <typename Func>
std::string callBuilder(Func&& func, const std::vector<std::string>& params) {
    switch (params.size()) {
        case 1: return func(params[0]);
        case 2: return func(params[0], params[1]);
        case 3: return func(params[0], params[1], params[2]);
        default: return "Error: Unsupported number of parameters!";
    }
}*/
// Makes a sequence of numbers matching indexes, expands to individual arguments , calls fucn with those argumenst. 
template <typename Func, size_t... Indices>
std::string invokeWithVector(Func&& func, const std::vector<std::string>& params, std::index_sequence<Indices...>) {
    return func(params[Indices]...);  // Expands params dynamically
}
// Takes a function, with (ARgs...) = any numbere of arguments , finds the ammount of arguments  and checks the size matches 
// calls invoke to split em.
template <typename Ret, typename... Args>
std::string callBuilder(std::function<Ret(Args...)> func, const std::vector<std::string>& params) {
    if (params.size() < sizeof...(Args)) { 
        return "Error: Incorrect number of parameters! Expected " + std::to_string(sizeof...(Args)) +
               ", but received " + std::to_string(params.size()) + ".";
    }

    return invokeWithVector(func, params, std::make_index_sequence<sizeof...(Args)>{});
}

namespace MessageBuilder {

std::string generateMessage(MsgType type, const std::vector<std::string>& params) {
    switch (type) {
        case MsgType::NICKNAME_IN_USE:
            return callBuilder(std::function<std::string(const std::string&)>(MessageBuilder::buildNicknameInUse), params);

        case MsgType::RPL_NICK_CHANGE:
            return callBuilder(std::function<std::string(const std::string&, const std::string&, const std::string&)>(MessageBuilder::buildNickChange), params);

        default:
            return "Error: Unknown message type";
    }
}

    // Helper for common server prefix (you can make this a constant or pass it)
    const std::string SERVER_PREFIX = ":ft_irc"; // Or ":ft_irc" as used in some of your macros
	const std::string SERVER_AT = "@ft_irc";
	const std::string QUIT_MSG = "Client disconnected";
    // General purpose error/reply messages
    std::string buildNicknameInUse(const std::string& nick) {
        return SERVER_PREFIX + " 433 "  + nick + " " + nick + "\r\n"; // Changed first nick to '*' as per RFC
    }

    // A more generic error builder, useful for 401, 461 etc.
    std::string buildErrorReply(const std::string& sender_prefix, const std::string& reply_code,
                                const std::string& recipient_nickname, const std::string& message_text) {
        return sender_prefix + " " + reply_code + " " + recipient_nickname + " :" + message_text + "\r\n";
    }

    std::string buildNotOperator(const std::string& nickname, const std::string& channelName) {
        // Your original macro used "localhost" as prefix, then used "nickname" in the message.
        // Let's use the standard error reply for consistency.
        return SERVER_PREFIX + " 482 " + nickname + " " + channelName + " :You're not channel operator\r\n";
    }

    std::string buildNoSuchNickOrChannel(const std::string& nickname, const std::string& target) {
        return SERVER_PREFIX + " 401 " + nickname + " " + target + " :No such nick/channel\r\n";
    }

    std::string buildNeedMoreParams(const std::string& nickname, const std::string& command) {
        return SERVER_PREFIX + " 461 " + nickname + " " + command + " :Not enough parameters\r\n";
    }

    // Welcome Package
    std::string buildWelcome(const std::string& nickname) {
        return ":server 001 " + nickname + " :Welcome to the IRC server " + nickname + "!\r\n"; // Added nick to end as per RFC
    }

    std::string buildHostInfo(const std::string& nickname) {
        return ":server 002 " + nickname + " :Your host is ft_irc, running version 1.0\r\n";
    }

    std::string buildServerCreation(const std::string& nickname) {
        return ":server 003 " + nickname + " :This server was created today\r\n";
    }

    std::string buildServerInfo(const std::string& nickname) {
        return ":server 004 " + nickname + " ft_irc 1.0 o o\r\n";
    }

    // Channel related messages
    // Note: The JOIN_CHANNEL macro was missing the user@host prefix.
    // It's crucial for correct IRC protocol messages. Assuming sender_prefix is "nick!user@host".
    std::string buildJoinChannel(const std::string& nickname_prefix, const std::string& channelName) {
        return nickname_prefix + " JOIN :" + channelName + "\r\n"; // Channel names are sometimes prefixed with ':'
    }

    std::string buildNamesList(const std::string& nickname, const std::string& channelName, const std::string& clientList) {
        return ":ft_irc 353 " + nickname + " = " + channelName + " :" + clientList + "\r\n";
    }

    std::string buildEndNamesList(const std::string& nickname, const std::string& channelName) {
        return ":ft_irc 366 " + nickname + " " + channelName + " :End of /NAMES list\r\n";
    }

    // Added 'topic' parameter that was missing in your macro
    std::string buildChannelTopic(const std::string& nickname, const std::string& channelName, const std::string& topic) {
        return ":ft_irc 332 " + nickname + " " + channelName + " :" + topic + "\r\n";
    }

    // Client related messages
    // Note: The RPL_NICK_CHANGE macro is for *broadcasting* a nick change.
    // It takes oldnick as the prefix, and newnick as the argument.
    std::string buildNickChange(const std::string& oldnick, const std::string & username, const std::string& newnick) {
        return ":" + oldnick + "!" + username + "@ft_irc NICK " +  newnick + "\r\n";
    }
    std::string buildClientQuit(const std::string& nickname, const std::string& username) {
        return ":" + nickname +"!" + username + SERVER_AT + " QUIT :" + QUIT_MSG + "\r\n";
    } //"nickname!username@localhost"

    // Server notices not sure if needed 
    std::string buildServerNoticeNickChange(const std::string& oldnick, const std::string& newnick) {
        return ":server NOTICE * :User " + oldnick + " changed nickname to " + newnick + "\r\n";
    }

} // namespace MessageBuilder