#pragma once
#include <string>
//#include <vector>


namespace MessageBuilder {

    // General purpose error/reply messages
    std::string buildNicknameInUse(const std::string& nick);
    std::string buildErrorReply(const std::string& sender_prefix, const std::string& reply_code,
                                const std::string& recipient_nickname, const std::string& message_text);
    std::string buildNotOperator(const std::string& nickname, const std::string& channelName);
    std::string buildNoSuchNickOrChannel(const std::string& nickname, const std::string& target); // For 401
    std::string buildNeedMoreParams(const std::string& nickname, const std::string& command); // For 461

    // Welcome Package
    std::string buildWelcome(const std::string& nickname);
    std::string buildHostInfo(const std::string& nickname);
    std::string buildServerCreation(const std::string& nickname);
    std::string buildServerInfo(const std::string& nickname);

    // Channel related messages
    std::string buildJoinChannel(const std::string& nickname_prefix, const std::string& channelName);
    std::string buildNamesList(const std::string& nickname, const std::string& channelName, const std::string& clientList);
    std::string buildEndNamesList(const std::string& nickname, const std::string& channelName);
    // Note: CHANNEL_TOPIC macro is missing a 'topic' parameter. This will be added correctly.
    std::string buildChannelTopic(const std::string& nickname, const std::string& channelName, const std::string& topic);

    // Client related messages
    std::string buildNickChange(const std::string& oldnick, const std::string & username, const std::string& newnick);
    std::string buildClientQuit(const std::string& nickname_prefix, const std::string& quit_message);

    // Server notices
    std::string buildServerNoticeNickChange(const std::string& oldnick, const std::string& newnick);

    // You can add more specific builders as needed, like:
    // std::string buildPrivMsg(const std::string& sender_prefix, const std::string& target, const std::string& message);
} // namespace MessageBuilder
