#pragma once
#include <string>
#include <vector>
#include "IrcResources.hpp"

namespace MessageBuilder {
	
	std::string generateMessage(MsgType type, const std::vector<std::string>& params);
    // General purpose error/reply messages
    std::string buildNicknameInUse(const std::string& nick, const std::string& attempted);
    std::string buildErrorReply(const std::string& sender_prefix, const std::string& reply_code,
                                const std::string& recipient_nickname, const std::string& message_text);
    std::string buildNotOperator(const std::string& nickname, const std::string& channelName);
    std::string buildNoSuchNick(const std::string& nickname, const std::string& target); // For 401
    std::string buildNeedMoreParams(const std::string& nickname, const std::string& command); // For 461

    // Welcome Package
	std::string generatewelcome(const std::string& nickname);
	std::string generateInitMsg();
    std::string buildWelcome(const std::string& nickname);
    std::string buildHostInfo(const std::string& nickname);
    std::string buildServerCreation(const std::string& nickname);
    std::string buildServerInfo(const std::string& nickname);

	std::string buildCapResponse(const std::string& clientNickname, const std::string& requestedCaps);

    // Channel related messages
    //std::string buildJoinChannel(const std::string& nickname_prefix, const std::string& channelName);
    std::string buildJoinChannel(const std::string& nickname, const std::string& username, const std::string& channelName, const std::string& clientList, const std::string& topic);
	std::string buildNamesList(const std::string& nickname, const std::string& channelName, const std::string& clientList);
    std::string buildEndNamesList(const std::string& nickname, const std::string& channelName);
    // Note: CHANNEL_TOPIC macro is missing a 'topic' parameter. This will be added correctly.
    std::string buildChannelTopic(const std::string& nickname, const std::string& channelName, const std::string& topic);

    // Client related messages
	//std::string buildNickChange2(const std::string& oldnick, const std::string& newnick);
    std::string buildNickChange(const std::string& oldnick, const std::string& username, const std::string& newnick);
    std::string buildClientQuit(const std::string& nickname, const std::string& username);

    // Server notices
    std::string buildServerNoticeNickChange(const std::string& oldnick, const std::string& newnick);

	std::string buildNoSuchChannel(const std::string& clientNickname, const std::string& channelName);
	std::string buildNoSuchNick(const std::string& nickname, const std::string& target); // <-- THIS LINE WAS OVERLOOKED BY YOUR PARTNER!
    std::string buildErroneousNickname(const std::string& clientCurrentNick, const std::string& badNick);
	std::string buildInviting(const std::string& sender_nickname, const std::string& target_nickname, const std::string& channel_name);	
	std::string buildGetInvite(const std::string& nickname, const std::string& username, const std::string& target, const std::string& channel_name);
	
	std::string buildNotInChannel(const std::string& clientNickname, const std::string& channelName);
	std::string buildInvalidTarget(const std::string& clientNickname, const std::string& target);
	std::string buildInvalidChannelName(const std::string& clientNickname, const std::string& channelName, const std::string& msg);
	std::string buildUModeIs(const std::string& clientNickname, const std::string& modeString);
	std::string buildChannelModeIs(const std::string& clientNickname, const std::string& channelName, const std::string& modeString, const std::string& modeParams = "");
	std::string buildChannelModeChange(const std::string& nickname, const std::string& username, const std::string channelname, const std::string& modes, const std::string& targets);
	std::string buildChanUpdate(const std::string& nickname, const std::string& username, const std::string& channelName);
	std::string buildChannelIsFull(const std::string& clientNickname, const std::string& channelName);	
	
	std::string buildClientModeChange(const std::string channelname, const std::string& modes);
   
	std::string buildInviteOnlyChannel(const std::string& clientNickname, const std::string& channelName);
	std::string buildNeedMoreParams(const std::string& clientNickname, const std::string& commandName);
	std::string buildIncorrectPasswordMessage(const std::string& clientNickname, const std::string& channelName);
	std::string buildRegistartionEnd(const std::string& nickname);

	std::string buildPrivMessage(const std::string& clientNickname, const std::string& username, const std::string& where, const std::string& msg);
	std::string buildPart(const std::string& clientNickname, const std::string& username, const std::string& channelName, const std::string& partReason);
	std::string buildKick(const std::string& clientNickname, const std::string& username, const std::string& channelName, const std::string& target, const std::string& kickReason);
	std::string buildUserOnChannel(const std::string& inviter_nickname, const std::string& target_nickname, const std::string& channel_name);
	std::string bildPing();
	std::string bildPong();
	// You can add more specific builders as needed, like:
    // std::string buildPrivMsg(const std::string& sender_prefix, const std::string& target, const std::string& message);
} // namespace MessageBuilder
