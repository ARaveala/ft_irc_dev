// sources/MessageBuilder.cpp

#include "MessageBuilder.hpp"
#include "config.h" // For other config constants like MSG_TYPE_NUM
#include "IrcResources.hpp" // For MsgType enum
#include <sstream>
#include <algorithm> // For std::transform
#include <cctype>    // For std::tolower

// Static member definition for the server prefix
// Initializing directly with a string literal, as config::SERVER_PREFIX might not be defined.
const std::string MessageBuilder::SERVER_PREFIX = ":ft_irc_server";


// Helper function to build the full message string based on MsgType
std::string MessageBuilder::generateMessage(MsgType type, const std::vector<std::string>& params) {
    switch (type) {
        case MsgType::ERR_NICKNAMEINUSE: // Corrected
            return MessageBuilder::buildNicknameInUse(params[0], params[1]); // params[0]=old_nick, params[1]=new_nick
        // Removed MsgType::RPL_NICK_CHANGE - Nick changes are commands, not numeric replies.
        case MsgType::RPL_INVITING: // Corrected from GET_INVITE
            return MessageBuilder::buildInviteMessage(params[0], params[1], params[2]); // client_nick, target_nick, channel_name
        case MsgType::RPL_WELCOME: // Corrected from WELCOME
            return MessageBuilder::buildWelcomeMessage(params[0]); // clientNickname
        // Removed MsgType::JOIN - JOIN is a command, not a numeric reply.
        case MsgType::ERR_NEEDMOREPARAMS: // Corrected
            return MessageBuilder::buildNeedMoreParams(params[0], params[1]); // clientNickname, command
        case MsgType::ERR_NOSUCHCHANNEL: // Corrected from NO_SUCH_CHANNEL
            return MessageBuilder::buildNoSuchChannel(params[0], params[1]); // clientNickname, channelName
        case MsgType::ERR_NOTONCHANNEL: // Corrected from NOT_ON_CHANNEL
            return MessageBuilder::buildNotInChannel(params[0], params[1]); // clientNickname, channelName
        // Removed MsgType::INVALID_TARGET - Map to ERR_NOSUCHNICK or specific errors as needed
        // Removed MsgType::INVALID_CHANNEL_NAME - Map to ERR_NOSUCHCHANNEL
        case MsgType::RPL_UMODEIS: // Corrected (now exists in MsgType)
            return MessageBuilder::buildUModeIs(params[0], params[1]); // clientNickname, modeString
        // Removed MsgType::CHANNEL_MODE_CHANGED - Mode changes are commands, not numeric replies.
        // Removed MsgType::UPDATE_CHAN - Internal, not a reply.
        // Removed MsgType::USER_LIMIT_CHANGED - Internal, not a reply.
        // Removed MsgType::CLIENT_QUIT - QUIT is a command, not a numeric reply.
        case MsgType::ERR_INVITEONLYCHAN: // Corrected from INVITE_ONLY
            return MessageBuilder::buildInviteOnlyChannel(params[0], params[1]); // clientNickname, channelName
        case MsgType::ERR_PASSWDMISMATCH: // Corrected from INVALID_PASSWORD
            return MessageBuilder::buildIncorrectPasswordMessage(params[0], params[1]); // clientNickname, reason (optional)
        // Removed MsgType::PART - PART is a command, not a numeric reply.
        // Removed MsgType::KICK - KICK is a command, not a numeric reply.
        case MsgType::ERR_CHANNELISFULL: // Corrected from CHANNEL_FULL
            return MessageBuilder::buildChannelIsFull(params[0], params[1]); // clientNickname, channelName
        case MsgType::ERR_UNKNOWNCOMMAND:
            return MessageBuilder::buildUnknownCommand(params[0], params[1]); // clientNickname, command
        case MsgType::ERR_NONICKNAMEGIVEN:
            return MessageBuilder::buildNoNicknameGiven(params[0]); // clientNickname
        case MsgType::ERR_ERRONEUSNICKNAME:
            return MessageBuilder::buildErroneousNickname(params[0], params[1]); // clientNickname, badNick
        case MsgType::ERR_ALREADYREGISTERED:
            return MessageBuilder::buildAlreadyRegistered(params[0]); // clientNickname
        case MsgType::ERR_NOSUCHNICK:
            return MessageBuilder::buildNoSuchNick(params[0], params[1]); // clientNickname, targetNick
        case MsgType::ERR_CANNOTSENDTOCHAN:
            return MessageBuilder::buildCannotSendToChannel(params[0], params[1]); // clientNickname, channelName
        case MsgType::ERR_USERNOTINCHANNEL:
            return MessageBuilder::buildUserNotInChannel(params[0], params[1], params[2]); // clientNickname, targetNick, channelName
        case MsgType::ERR_USERONCHANNEL:
            return MessageBuilder::buildUserOnChannel(params[0], params[1], params[2]); // clientNickname, targetNick, channelName
        case MsgType::ERR_CHANOPRIVSNEEDED:
            return MessageBuilder::buildChannelOpPrivsNeeded(params[0], params[1]); // clientNickname, channelName
        case MsgType::RPL_LUSERCLIENT:
            return MessageBuilder::buildLUserClient(params[0], params[1], params[2], params[3]); // users, services, servers, clientNickname
        case MsgType::RPL_LUSEROP:
            return MessageBuilder::buildLUserOp(params[0], params[1]); // opsCount, clientNickname
        case MsgType::RPL_LUSERUNKNOWN:
            return MessageBuilder::buildLUserUnknown(params[0], params[1]); // unknownCount, clientNickname
        case MsgType::RPL_LUSERCHANNELS:
            return MessageBuilder::buildLUserChannels(params[0], params[1]); // channelsCount, clientNickname
        case MsgType::RPL_LUSERME:
            return MessageBuilder::buildLUserMe(params[0], params[1], params[2]); // clients, servers, clientNickname
        case MsgType::RPL_ADMINME:
            return MessageBuilder::buildAdminMe(params[0], params[1]); // serverName, clientNickname
        case MsgType::RPL_ADMINLOC1:
            return MessageBuilder::buildAdminLoc1(params[0], params[1]); // infoLine, clientNickname
        case MsgType::RPL_ADMINLOC2:
            return MessageBuilder::buildAdminLoc2(params[0], params[1]); // infoLine, clientNickname
        case MsgType::RPL_ADMINEMAIL:
            return MessageBuilder::buildAdminEmail(params[0], params[1]); // emailAddress, clientNickname
        case MsgType::RPL_LOCALUSERS:
            return MessageBuilder::buildLocalUsers(params[0], params[1], params[2]); // currentUsers, maxUsers, clientNickname
        case MsgType::RPL_GLOBALUSERS:
            return MessageBuilder::buildGlobalUsers(params[0], params[1], params[2]); // currentUsers, maxUsers, clientNickname
        case MsgType::RPL_WHOISUSER:
            return MessageBuilder::buildWhoisUser(params[0], params[1], params[2], params[3], params[4]); // nick, user, host, realname, clientNickname
        case MsgType::RPL_WHOISSERVER:
            return MessageBuilder::buildWhoisServer(params[0], params[1], params[2], params[3]); // nick, server, serverInfo, clientNickname
        case MsgType::RPL_WHOISOPERATOR:
            return MessageBuilder::buildWhoisOperator(params[0], params[1]); // nick, clientNickname
        case MsgType::RPL_ENDOFWHOIS:
            return MessageBuilder::buildEndOfWhois(params[0], params[1]); // nick, clientNickname
        case MsgType::RPL_WHOISIDLE:
            return MessageBuilder::buildWhoisIdle(params[0], params[1], params[2], params[3]); // nick, idleTime, signonTime, clientNickname
        case MsgType::RPL_LIST:
            return MessageBuilder::buildListChannel(params[0], params[1], params[2], params[3]); // channel, numClients, topic, clientNickname
        case MsgType::RPL_LISTEND:
            return MessageBuilder::buildListEnd(params[0]); // clientNickname
        case MsgType::RPL_CHANNELMODEIS:
            return MessageBuilder::buildChannelModeIs(params[0], params[1], params[2]); // channel, modeString, clientNickname
        case MsgType::RPL_CREATIONTIME:
            return MessageBuilder::buildCreationTime(params[0], params[1], params[2]); // channel, creationTime, clientNickname
        case MsgType::RPL_NOTOPIC:
            return MessageBuilder::buildNoTopic(params[0], params[1]); // channel, clientNickname
        case MsgType::RPL_TOPIC:
            return MessageBuilder::buildTopic(params[0], params[1], params[2]); // channel, topic, clientNickname
        case MsgType::RPL_TOPICWHOTIME:
            return MessageBuilder::buildTopicWhoTime(params[0], params[1], params[2], params[3]); // channel, setter, time, clientNickname
        case MsgType::RPL_MOTDSTART:
            return MessageBuilder::buildMotdStart(params[0], params[1]); // clientNickname, serverName
        case MsgType::RPL_MOTD:
            return MessageBuilder::buildMotd(params[0], params[1]); // motdLine, clientNickname
        case MsgType::RPL_ENDOFMOTD:
            return MessageBuilder::buildMotdEnd(params[0]); // clientNickname
        case MsgType::RPL_YOUREOPER:
            return MessageBuilder::buildYouAreOper(params[0]); // clientNickname
        case MsgType::RPL_REHASHING:
            return MessageBuilder::buildRehashing(params[0], params[1]); // configFile, clientNickname
        case MsgType::RPL_NAMREPLY:
            return MessageBuilder::buildNameReply(params[0], params[1], params[2], params[3]); // channelType, channelName, nicknameList, clientNickname
        case MsgType::RPL_ENDOFNAMES:
            return MessageBuilder::buildEndOfNames(params[0], params[1]); // channelName, clientNickname

        default:
            return "UNKNOWN MESSAGE TYPE"; // Fallback for unhandled types
    }
}

// Implementations for all MessageBuilder functions (partial for example)

std::string MessageBuilder::buildNicknameInUse(const std::string& oldNick, const std::string& newNick) {
    // Corrected format: :server 433 * <new_nick> :Nickname is already in use.
    // Assuming params[0] is the new_nick and params[1] is the old_nick (from your old usage)
    // The IRC standard format for 433 is usually just the attempted nickname.
    return SERVER_PREFIX + " 433 " + (oldNick.empty() ? "*" : oldNick) + " " + newNick + " :Nickname is already in use.\r\n";
}

std::string MessageBuilder::buildWelcomeMessage(const std::string& clientNickname) {
    return SERVER_PREFIX + " 001 " + clientNickname + " :Welcome to the ft_irc Network, " + clientNickname + "\r\n";
}

std::string MessageBuilder::generateInitMsg() {
    return "CAP LS\r\nNOTICE AUTH :*** Checking ident\r\nNOTICE AUTH :*** Looking up your hostname...\r\nNOTICE AUTH :*** Found your hostname\r\n";
}

std::string MessageBuilder::buildNeedMoreParams(const std::string& clientNickname, const std::string& command) {
    return SERVER_PREFIX + " " + std::to_string(static_cast<int>(MsgType::ERR_NEEDMOREPARAMS)) + " " + clientNickname + " " + command + " :Not enough parameters\r\n";
}

std::string MessageBuilder::buildNoSuchChannel(const std::string& clientNickname, const std::string& channelName) {
    return SERVER_PREFIX + " " + std::to_string(static_cast<int>(MsgType::ERR_NOSUCHCHANNEL)) + " " + clientNickname + " " + channelName + " :No such channel\r\n";
}

std::string MessageBuilder::buildNotInChannel(const std::string& clientNickname, const std::string& channelName) {
    return SERVER_PREFIX + " " + std::to_string(static_cast<int>(MsgType::ERR_NOTONCHANNEL)) + " " + clientNickname + " " + channelName + " :You're not on that channel\r\n";
}

// Dummy for buildInvalidChannelName (removed from switch, but if other parts of code still call it)
std::string MessageBuilder::buildInvalidChannelName(const std::string& clientNickname, const std::string& channelName, const std::string& msg) {
    return SERVER_PREFIX + " " + std::to_string(static_cast<int>(MsgType::ERR_NOSUCHCHANNEL)) + " " + clientNickname + " " + channelName + " " + msg + "\r\n"; // Using ERR_NOSUCHCHANNEL
}

// Dummy for buildInvalidTarget (removed from switch)
std::string MessageBuilder::buildInvalidTarget(const std::string& clientNickname, const std::string& target) {
    return SERVER_PREFIX + " " + std::to_string(static_cast<int>(MsgType::ERR_NOSUCHNICK)) + " " + clientNickname + " :No such nick/channel " + target + "\r\n";
}

std::string MessageBuilder::buildUModeIs(const std::string& clientNickname, const std::string& modeString) {
    return SERVER_PREFIX + " " + std::to_string(static_cast<int>(MsgType::RPL_UMODEIS)) + " " + clientNickname + " " + modeString + "\r\n";
}

std::string MessageBuilder::buildInviteOnlyChannel(const std::string& clientNickname, const std::string& channelName) {
    return SERVER_PREFIX + " " + std::to_string(static_cast<int>(MsgType::ERR_INVITEONLYCHAN)) +
           " " + clientNickname + " " + channelName + " :Cannot join channel (+i)\r\n";
}

std::string MessageBuilder::buildIncorrectPasswordMessage(const std::string& clientNickname, const std::string& reason) {
    // Reason is optional, default to a general message if not provided
    std::string finalReason = reason.empty() ? "Password incorrect." : reason;
    return SERVER_PREFIX + " " + std::to_string(static_cast<int>(MsgType::ERR_PASSWDMISMATCH)) +
           " " + clientNickname + " : " + finalReason + "\r\n";
}

std::string MessageBuilder::buildChannelIsFull(const std::string& clientNickname, const std::string& channelName) {
    return SERVER_PREFIX + " " + std::to_string(static_cast<int>(MsgType::ERR_CHANNELISFULL)) +
           " " + clientNickname + " " + channelName + " :Cannot join channel (+l)\r\n";
}

// Additional Message Builders (from IrcResources.hpp MsgType entries)

std::string MessageBuilder::buildUnknownCommand(const std::string& clientNickname, const std::string& command) {
    return SERVER_PREFIX + " 421 " + clientNickname + " " + command + " :Unknown command\r\n";
}

std::string MessageBuilder::buildNoNicknameGiven(const std::string& clientNickname) {
    return SERVER_PREFIX + " 431 " + clientNickname + " :No nickname given\r\n";
}

std::string MessageBuilder::buildErroneousNickname(const std::string& clientNickname, const std::string& badNick) {
    return SERVER_PREFIX + " 432 " + clientNickname + " " + badNick + " :Erroneous nickname\r\n";
}

std::string MessageBuilder::buildAlreadyRegistered(const std::string& clientNickname) {
    return SERVER_PREFIX + " 462 " + clientNickname + " :Unauthorized command (already registered)\r\n";
}

std::string MessageBuilder::buildNoSuchNick(const std::string& clientNickname, const std::string& targetNick) {
    return SERVER_PREFIX + " 401 " + clientNickname + " " + targetNick + " :No such nick/channel\r\n";
}

std::string MessageBuilder::buildCannotSendToChannel(const std::string& clientNickname, const std::string& channelName) {
    return SERVER_PREFIX + " 404 " + clientNickname + " " + channelName + " :Cannot send to channel\r\n";
}

std::string MessageBuilder::buildUserNotInChannel(const std::string& clientNickname, const std::string& targetNick, const std::string& channelName) {
    return SERVER_PREFIX + " 441 " + clientNickname + " " + targetNick + " " + channelName + " :They aren't on that channel\r\n";
}

std::string MessageBuilder::buildUserOnChannel(const std::string& clientNickname, const std::string& targetNick, const std::string& channelName) {
    return SERVER_PREFIX + " 443 " + clientNickname + " " + targetNick + " " + channelName + " :is already on channel\r\n";
}

std::string MessageBuilder::buildChannelOpPrivsNeeded(const std::string& clientNickname, const std::string& channelName) {
    return SERVER_PREFIX + " 482 " + clientNickname + " " + channelName + " :You're not channel operator\r\n";
}

std::string MessageBuilder::buildLUserClient(const std::string& users, const std::string& services, const std::string& servers, const std::string& clientNickname) {
    return SERVER_PREFIX + " 251 " + clientNickname + " :There are " + users + " users and " + services + " services on " + servers + " servers\r\n";
}

std::string MessageBuilder::buildLUserOp(const std::string& opsCount, const std::string& clientNickname) {
    return SERVER_PREFIX + " 252 " + clientNickname + " " + opsCount + " :operator(s) online\r\n";
}

std::string MessageBuilder::buildLUserUnknown(const std::string& unknownCount, const std::string& clientNickname) {
    return SERVER_PREFIX + " 253 " + clientNickname + " " + unknownCount + " :unknown connection(s)\r\n";
}

std::string MessageBuilder::buildLUserChannels(const std::string& channelsCount, const std::string& clientNickname) {
    return SERVER_PREFIX + " 254 " + clientNickname + " " + channelsCount + " :channels formed\r\n";
}

std::string MessageBuilder::buildLUserMe(const std::string& clients, const std::string& servers, const std::string& clientNickname) {
    return SERVER_PREFIX + " 255 " + clientNickname + " :I have " + clients + " clients and " + servers + " servers\r\n";
}

std::string MessageBuilder::buildAdminMe(const std::string& serverName, const std::string& clientNickname) {
    return SERVER_PREFIX + " 256 " + clientNickname + " " + serverName + " :Administrative info\r\n";
}

std::string MessageBuilder::buildAdminLoc1(const std::string& infoLine, const std::string& clientNickname) {
    return SERVER_PREFIX + " 257 " + clientNickname + " :" + infoLine + "\r\n";
}

std::string MessageBuilder::buildAdminLoc2(const std::string& infoLine, const std::string& clientNickname) {
    return SERVER_PREFIX + " 258 " + clientNickname + " :" + infoLine + "\r\n";
}

std::string MessageBuilder::buildAdminEmail(const std::string& emailAddress, const std::string& clientNickname) {
    return SERVER_PREFIX + " 259 " + clientNickname + " :" + emailAddress + "\r\n";
}

std::string MessageBuilder::buildLocalUsers(const std::string& currentUsers, const std::string& maxUsers, const std::string& clientNickname) {
    return SERVER_PREFIX + " 265 " + clientNickname + " " + currentUsers + " " + maxUsers + " :Current local users " + currentUsers + ", max " + maxUsers + "\r\n";
}

std::string MessageBuilder::buildGlobalUsers(const std::string& currentUsers, const std::string& maxUsers, const std::string& clientNickname) {
    return SERVER_PREFIX + " 266 " + clientNickname + " " + currentUsers + " " + maxUsers + " :Current global users " + currentUsers + ", max " + maxUsers + "\r\n";
}

std::string MessageBuilder::buildWhoisUser(const std::string& nick, const std::string& user, const std::string& host, const std::string& realname, const std::string& clientNickname) {
    return SERVER_PREFIX + " 311 " + clientNickname + " " + nick + " " + user + " " + host + " * :" + realname + "\r\n";
}

std::string MessageBuilder::buildWhoisServer(const std::string& nick, const std::string& server, const std::string& serverInfo, const std::string& clientNickname) {
    return SERVER_PREFIX + " 312 " + clientNickname + " " + nick + " " + server + " :" + serverInfo + "\r\n";
}

std::string MessageBuilder::buildWhoisOperator(const std::string& nick, const std::string& clientNickname) {
    return SERVER_PREFIX + " 313 " + clientNickname + " " + nick + " :is an IRC operator\r\n";
}

std::string MessageBuilder::buildEndOfWhois(const std::string& nick, const std::string& clientNickname) {
    return SERVER_PREFIX + " 318 " + clientNickname + " " + nick + " :End of /WHOIS list.\r\n";
}

std::string MessageBuilder::buildWhoisIdle(const std::string& nick, const std::string& idleTime, const std::string& signonTime, const std::string& clientNickname) {
    return SERVER_PREFIX + " 317 " + clientNickname + " " + nick + " " + idleTime + " " + signonTime + " :seconds idle, signon time\r\n";
}

std::string MessageBuilder::buildListChannel(const std::string& channel, const std::string& numClients, const std::string& topic, const std::string& clientNickname) {
    return SERVER_PREFIX + " 322 " + clientNickname + " " + channel + " " + numClients + " :" + topic + "\r\n";
}

std::string MessageBuilder::buildListEnd(const std::string& clientNickname) {
    return SERVER_PREFIX + " 323 " + clientNickname + " :End of /LIST\r\n";
}

std::string MessageBuilder::buildChannelModeIs(const std::string& channel, const std::string& modeString, const std::string& clientNickname) {
    return SERVER_PREFIX + " 324 " + clientNickname + " " + channel + " " + modeString + "\r\n";
}

std::string MessageBuilder::buildCreationTime(const std::string& channel, const std::string& creationTime, const std::string& clientNickname) {
    return SERVER_PREFIX + " 329 " + clientNickname + " " + channel + " " + creationTime + "\r\n";
}

std::string MessageBuilder::buildNoTopic(const std::string& channel, const std::string& clientNickname) {
    return SERVER_PREFIX + " 331 " + clientNickname + " " + channel + " :No topic is set\r\n";
}

std::string MessageBuilder::buildTopic(const std::string& channel, const std::string& topic, const std::string& clientNickname) {
    return SERVER_PREFIX + " 332 " + clientNickname + " " + channel + " :" + topic + "\r\n";
}

std::string MessageBuilder::buildTopicWhoTime(const std::string& channel, const std::string& setter, const std::string& time, const std::string& clientNickname) {
    return SERVER_PREFIX + " 333 " + clientNickname + " " + channel + " " + setter + " " + time + "\r\n";
}

std::string MessageBuilder::buildInviteMessage(const std::string& clientNickname, const std::string& targetNickname, const std::string& channelName) {
    // This is RPL_INVITING (341) - "Sent to the client after a successful INVITE command"
    return SERVER_PREFIX + " 341 " + clientNickname + " " + targetNickname + " :" + channelName + "\r\n";
}

std::string MessageBuilder::buildMotdStart(const std::string& clientNickname, const std::string& serverName) {
    return SERVER_PREFIX + " 375 " + clientNickname + " :- " + serverName + " Message of the day - \r\n";
}

std::string MessageBuilder::buildMotd(const std::string& motdLine, const std::string& clientNickname) {
    return SERVER_PREFIX + " 372 " + clientNickname + " :" + motdLine + "\r\n";
}

std::string MessageBuilder::buildMotdEnd(const std::string& clientNickname) {
    return SERVER_PREFIX + " 376 " + clientNickname + " :End of MOTD command\r\n";
}

std::string MessageBuilder::buildYouAreOper(const std::string& clientNickname) {
    return SERVER_PREFIX + " 381 " + clientNickname + " :You are now an IRC operator\r\n";
}

std::string MessageBuilder::buildRehashing(const std::string& configFile, const std::string& clientNickname) {
    return SERVER_PREFIX + " 382 " + clientNickname + " " + configFile + " :Rehashing\r\n";
}

std::string MessageBuilder::buildNameReply(const std::string& channelType, const std::string& channelName, const std::string& nicknameList, const std::string& clientNickname) {
    return SERVER_PREFIX + " 353 " + clientNickname + " " + channelType + " " + channelName + " :" + nicknameList + "\r\n";
}

std::string MessageBuilder::buildEndOfNames(const std::string& channelName, const std::string& clientNickname) {
    return SERVER_PREFIX + " 366 " + clientNickname + " " + channelName + " :End of /NAMES list.\r\n";
}
