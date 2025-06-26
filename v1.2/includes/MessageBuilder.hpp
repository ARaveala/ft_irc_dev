#pragma once

#include <string>
#include <vector>
#include "IrcResources.hpp" // For MsgType enum

class MessageBuilder {
public:
    // Static member for the server prefix used in messages
    static const std::string SERVER_PREFIX; // Declaration

    // Helper function to build the full message string based on MsgType
    static std::string generateMessage(MsgType type, const std::vector<std::string>& params);
    static std::string generateInitMsg();
    static std::string generatewelcome(const std::string& clientNickname); // Add this declaration as it's used in CommandDispatcher

    // Declarations for all helper functions used in generateMessage
    static std::string buildNicknameInUse(const std::string& oldNick, const std::string& newNick);
    static std::string buildWelcomeMessage(const std::string& clientNickname);
    static std::string buildNeedMoreParams(const std::string& clientNickname, const std::string& command);
    static std::string buildNoSuchChannel(const std::string& clientNickname, const std::string& channelName);
    static std::string buildNotInChannel(const std::string& clientNickname, const std::string& channelName);
    static std::string buildInvalidChannelName(const std::string& clientNickname, const std::string& channelName, const std::string& msg); // Keep if still used
    static std::string buildInvalidTarget(const std::string& clientNickname, const std::string& target); // Keep if still used
    static std::string buildUModeIs(const std::string& clientNickname, const std::string& modeString);
    static std::string buildInviteOnlyChannel(const std::string& clientNickname, const std::string& channelName);
    static std::string buildIncorrectPasswordMessage(const std::string& clientNickname, const std::string& reason);
    static std::string buildChannelIsFull(const std::string& clientNickname, const std::string& channelName);
    static std::string buildUnknownCommand(const std::string& clientNickname, const std::string& command);
    static std::string buildNoNicknameGiven(const std::string& clientNickname);
    static std::string buildErroneousNickname(const std::string& clientNickname, const std::string& badNick);
    static std::string buildAlreadyRegistered(const std::string& clientNickname);
    static std::string buildNoSuchNick(const std::string& clientNickname, const std::string& targetNick);
    static std::string buildCannotSendToChannel(const std::string& clientNickname, const std::string& channelName);
    static std::string buildUserNotInChannel(const std::string& clientNickname, const std::string& targetNick, const std::string& channelName);
    static std::string buildUserOnChannel(const std::string& clientNickname, const std::string& targetNick, const std::string& channelName);
    static std::string buildChannelOpPrivsNeeded(const std::string& clientNickname, const std::string& channelName);
    static std::string buildLUserClient(const std::string& users, const std::string& services, const std::string& servers, const std::string& clientNickname);
    static std::string buildLUserOp(const std::string& opsCount, const std::string& clientNickname);
    static std::string buildLUserUnknown(const std::string& unknownCount, const std::string& clientNickname);
    static std::string buildLUserChannels(const std::string& channelsCount, const std::string& clientNickname);
    static std::string buildLUserMe(const std::string& clients, const std::string& servers, const std::string& clientNickname);
    static std::string buildAdminMe(const std::string& serverName, const std::string& clientNickname);
    static std::string buildAdminLoc1(const std::string& infoLine, const std::string& clientNickname);
    static std::string buildAdminLoc2(const std::string& infoLine, const std::string& clientNickname);
    static std::string buildAdminEmail(const std::string& emailAddress, const std::string& clientNickname);
    static std::string buildLocalUsers(const std::string& currentUsers, const std::string& maxUsers, const std::string& clientNickname);
    static std::string buildGlobalUsers(const std::string& currentUsers, const std::string& maxUsers, const std::string& clientNickname);
    static std::string buildWhoisUser(const std::string& nick, const std::string& user, const std::string& host, const std::string& realname, const std::string& clientNickname);
    static std::string buildWhoisServer(const std::string& nick, const std::string& server, const std::string& serverInfo, const std::string& clientNickname);
    static std::string buildWhoisOperator(const std::string& nick, const std::string& clientNickname);
    static std::string buildEndOfWhois(const std::string& nick, const std::string& clientNickname);
    static std::string buildWhoisIdle(const std::string& nick, const std::string& idleTime, const std::string& signonTime, const std::string& clientNickname);
    static std::string buildListChannel(const std::string& channel, const std::string& numClients, const std::string& topic, const std::string& clientNickname);
    static std::string buildListEnd(const std::string& clientNickname);
    static std::string buildChannelModeIs(const std::string& channel, const std::string& modeString, const std::string& clientNickname);
    static std::string buildCreationTime(const std::string& channel, const std::string& creationTime, const std::string& clientNickname);
    static std::string buildNoTopic(const std::string& channel, const std::string& clientNickname);
    static std::string buildTopic(const std::string& channel, const std::string& topic, const std::string& clientNickname);
    static std::string buildTopicWhoTime(const std::string& channel, const std::string& setter, const std::string& time, const std::string& clientNickname);
    static std::string buildInviteMessage(const std::string& clientNickname, const std::string& targetNickname, const std::string& channelName);
    static std::string buildMotdStart(const std::string& clientNickname, const std::string& serverName);
    static std::string buildMotd(const std::string& motdLine, const std::string& clientNickname);
    static std::string buildMotdEnd(const std::string& clientNickname);
    static std::string buildYouAreOper(const std::string& clientNickname);
    static std::string buildRehashing(const std::string& configFile, const std::string& clientNickname);
    static std::string buildNameReply(const std::string& channelType, const std::string& channelName, const std::string& nicknameList, const std::string& clientNickname);
    static std::string buildEndOfNames(const std::string& channelName, const std::string& clientNickname);
};
