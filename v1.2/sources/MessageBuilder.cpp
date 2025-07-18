#include <tuple>
#include <functional>
#include <string>
#include <iostream> // debugging only 

#include "MessageBuilder.hpp"

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
	std::cout << "MsgType numeric value: " << static_cast<int>(type) << std::endl;
		switch (type) {
	        case MsgType::NICKNAME_IN_USE:
	            return callBuilder(std::function<std::string(const std::string&, const std::string&)>(MessageBuilder::buildNicknameInUse), params);
	        case MsgType::RPL_NICK_CHANGE:
	            return callBuilder(std::function<std::string(const std::string&, const std::string&, const std::string&)>(MessageBuilder::buildNickChange), params);
			case MsgType::RPL_INVITING: 
				return callBuilder(std::function<std::string(const std::string&, const std::string&, const std::string&)>(MessageBuilder::buildInviting), params);
			case MsgType::GET_INVITE:
				return callBuilder(std::function<std::string(const std::string&, const std::string&, const std::string&, const std::string&)>(MessageBuilder::buildGetInvite), params);
			case MsgType::ERR_USERONCHANNEL:
				return callBuilder(std::function<std::string(const std::string&, const std::string&, const std::string&)>(MessageBuilder::buildUserOnChannel), params);
	        case MsgType::WELCOME:
	            return callBuilder(std::function<std::string(const std::string&)>(MessageBuilder::generatewelcome), params);
			case MsgType::JOIN:
				return callBuilder(std::function<std::string(const std::string&, const std::string&, const std::string&, const std::string&, const std::string&)>(MessageBuilder::buildJoinChannel), params);
	        case MsgType::NEED_MORE_PARAMS:
	            return callBuilder(std::function<std::string(const std::string&, const std::string&)>(MessageBuilder::buildNeedMoreParams), params);
	        case MsgType::NO_SUCH_CHANNEL:
	            return callBuilder(std::function<std::string(const std::string&, const std::string&)>(MessageBuilder::buildNoSuchChannel), params);
	        case MsgType::ERR_NOSUCHNICK:
	            return callBuilder(std::function<std::string(const std::string&, const std::string&)>(MessageBuilder::buildNoSuchNick), params);
			case MsgType::ERR_ERRONEUSNICKNAME:
	            return callBuilder(std::function<std::string(const std::string&, const std::string&)>(MessageBuilder::buildErroneousNickname), params);	        
			case MsgType::NOT_ON_CHANNEL:
	            return callBuilder(std::function<std::string(const std::string&, const std::string&)>(MessageBuilder::buildNotInChannel), params);
	        case MsgType::NOT_OPERATOR:
	            return callBuilder(std::function<std::string(const std::string&, const std::string&)>(MessageBuilder::buildNotOperator), params);
	        case MsgType::INVALID_TARGET:
	            return callBuilder(std::function<std::string(const std::string&, const std::string&)>(MessageBuilder::buildInvalidTarget), params);
	        case MsgType::INVALID_CHANNEL_NAME:
	            return callBuilder(std::function<std::string(const std::string&, const std::string&, const std::string&)>(MessageBuilder::buildInvalidChannelName), params);
	        case MsgType::RPL_CHANNELMODEIS:
	            return callBuilder(std::function<std::string(const std::string&, const std::string&, const std::string&, const std::string&)>(MessageBuilder::buildChannelModeIs), params);
	        case MsgType::RPL_UMODEIS:
	            return callBuilder(std::function<std::string(const std::string&, const std::string&)>(MessageBuilder::buildUModeIs), params);
	        case MsgType::CHANNEL_MODE_CHANGED:
	            return callBuilder(std::function<std::string(const std::string&, const std::string&, const std::string&, const std::string&, const std::string&)>(MessageBuilder::buildChannelModeChange), params);
	        case MsgType::UPDATE_CHAN:
	            return callBuilder(std::function<std::string(const std::string&, const std::string&, const std::string&)>(MessageBuilder::buildChanUpdate), params);
			case MsgType::USER_LIMIT_CHANGED:
	            return callBuilder(std::function<std::string(const std::string&, const std::string&)>(MessageBuilder::buildClientModeChange), params);
	        case MsgType::CLIENT_QUIT:
	            return callBuilder(std::function<std::string(const std::string&, const std::string&)>(MessageBuilder::buildClientQuit), params);
	        case MsgType::RPL_TOPIC:
	            return callBuilder(std::function<std::string(const std::string&, const std::string&, const std::string&, const std::string&)>(MessageBuilder::buildTopicChange), params);
			case MsgType::INVITE_ONLY:
			    return callBuilder(std::function<std::string(const std::string&, const std::string&)>(MessageBuilder::buildInviteOnlyChannel), params);					
			case MsgType::INVALID_PASSWORD:
			    return callBuilder(std::function<std::string(const std::string&, const std::string&)>(MessageBuilder::buildIncorrectPasswordMessage), params);
	        case MsgType::PART:
	            return callBuilder(std::function<std::string(const std::string&, const std::string&, const std::string&, const std::string&)>(MessageBuilder::buildPart), params);
			case MsgType::KICK:
				return callBuilder(std::function<std::string(const std::string&, const std::string&, const std::string&, const std::string&, const std::string&)>(MessageBuilder::buildKick), params);
			case MsgType::PRIV_MSG:
			return callBuilder(std::function<std::string(const std::string&, const std::string&, const std::string&, const std::string&)>(MessageBuilder::buildPrivMessage), params);
			// Add more cases here...
			default:
		        return "Error: Unknown message type";
		}
	}
	
	std::string generatewelcome(const std::string& nickname) {
		return buildWelcome(nickname) + buildHostInfo(nickname) +  buildServerCreation(nickname) + buildServerInfo(nickname) + buildRegistartionEnd(nickname);
	}

    // Helper for common server prefix (you can make this a constant or pass it)
    const std::string SERVER = "localhost";
	const std::string SERVER_PREFIX = ":" + SERVER; // Or ":localhost" as used in some of your macros
	const std::string SERVER_AT = "@localhost";
	const std::string QUIT_MSG = "Client disconnected";
	
	std::string generateInitMsg() {
		return SERVER_PREFIX + " NOTICE * :initilization has begun.......\r\n";// +  SERVER_PREFIX + " CAP * LS :multi-prefix\r\n";
	}
	inline std::string prefix(const std::string& nick, const std::string& user) {
	    return ":" + nick + "!" + user + SERVER_AT;
	}
	std::string buildRegistartionEnd(const std::string& nickname) {
		std::string fullhello = SERVER_PREFIX + " 375 " + nickname + " :- ~ Meowdy, traveler! ~\r\n"
    	 + SERVER_PREFIX + " 372 " + nickname + " :- Here's your good-luck cat butt:\r\n"
    	 + SERVER_PREFIX + " 372 " + nickname + " :-     /\\_/\\\r\n"
    	 + SERVER_PREFIX + " 372 " + nickname + " :-    ( o.o )\r\n"
    	 + SERVER_PREFIX + " 372 " + nickname + " :-     > ^ <    🍑\r\n"
    	 + SERVER_PREFIX + " 372 " + nickname + " :-     Cat butt initialized.\r\n"
    	 + SERVER_PREFIX + " 376 " + nickname + " :End of /MOTD\r\n";
		return fullhello;
	}
    
    std::string buildNicknameInUse(const std::string& nick, const std::string& attempted) {
        return SERVER_PREFIX + " 433 "  + nick + " " + attempted + "\r\n";
    }

    // A more generic error builder, useful for 401, 461 etc.
    std::string buildErrorReply(const std::string& sender_prefix, const std::string& reply_code,
                                const std::string& recipient_nickname, const std::string& message_text) {
        return sender_prefix + " " + reply_code + " " + recipient_nickname + " :" + message_text + "\r\n";
    }

    std::string buildNoSuchNick(const std::string& nickname, const std::string& target) {
        return SERVER_PREFIX +  " " + std::to_string(static_cast<int>(MsgType::ERR_NOSUCHNICK)) + " " + nickname + " " + target + " :No such nick/channel\r\n";
    }

	std::string buildErroneousNickname(const std::string& clientCurrentNick, const std::string& badNick) {
		(void)clientCurrentNick;
    	return SERVER_PREFIX + " " + std::to_string(static_cast<int>(MsgType::ERR_ERRONEUSNICKNAME))+ " * " + badNick + " :Erroneous nickname\r\n";
	}

    // Welcome Package
    std::string buildWelcome(const std::string& nickname) {
        return SERVER_PREFIX + " 001 " + nickname + " :Welcome to the IRC server " + nickname + "!\r\n"; // Added nick to end as per RFC
    }

    std::string buildHostInfo(const std::string& nickname) {
        return SERVER_PREFIX + " 002 " + nickname + " :Your host is "+ SERVER +", running version 1.0\r\n";
    }

    std::string buildServerCreation(const std::string& nickname) {
        return SERVER_PREFIX + " 003 " + nickname + " :This server was created today\r\n";
    }

    std::string buildServerInfo(const std::string& nickname) {
        return SERVER_PREFIX + " 004 " + nickname + " " + SERVER + " 1.0 o o\r\n";
    }

	// cap response 
	std::string buildCapResponse() {
	    return SERVER_PREFIX + " CAP  * ACK :" +  "multi-prefix" + "\r\n" + SERVER_PREFIX + " CAP * ACK :END\r\n";
	}

	std::string bildPing() {
		return "PING :" + SERVER + "\r\n";
	}

	std::string bildPong() {
		return "PONG :" + SERVER + "\r\n";
	}

	//return SERVER_PREFIX + " CAP * LS :multi-prefix sasl\r\nconst std::string&ender_prefix is "nick!user@host".
	std::string buildJoinChannel(const std::string& nickname, const std::string& username, const std::string& channelName, const std::string& clientList, const std::string& topic) {
		(void) topic;
		std::string name =  prefix(nickname, username) + " JOIN " + channelName + "\r\n";
    	std::string namesReply = SERVER_PREFIX + " 353 " + nickname + " = " + channelName + " :" + clientList + "\r\n";
    	std::string endOfNames = SERVER_PREFIX + " 366 " + nickname + " " + channelName + " :End of /NAMES list\r\n";
    	std::string topicReply = SERVER_PREFIX + " 332 " + nickname + " " + channelName + " :Welcome to " + channelName + "!\r\n";;
	    return name + topicReply + namesReply + endOfNames ;
	}

    std::string buildNamesList(const std::string& nickname, const std::string& channelName, const std::string& clientList) {
        return ":" + SERVER + " 353 " + nickname + " = " + channelName + " :" + clientList + "\r\n";
    }

    std::string buildEndNamesList(const std::string& nickname, const std::string& channelName) {
        return ":" + SERVER + " 366 " + nickname + " " + channelName + " :End of /NAMES list\r\n";
    }

    // Added 'topic' parameter that was missing in your macro
    std::string buildChannelTopic(const std::string& nickname, const std::string& channelName, const std::string& topic) {
        return ":" + SERVER + " 332 " + nickname + " " + channelName + " :" + topic + "\r\n";
    }

    std::string buildNickChange(const std::string& oldnick, const std::string & username, const std::string& newnick) {
        return prefix(oldnick, username) + " NICK " +  newnick + "\r\n";
    }
    std::string buildClientQuit(const std::string& clientNickname, const std::string& username) {
        return prefix(clientNickname, username) + " QUIT :" + QUIT_MSG + "\r\n";
    } 

    // Server notices not sure if needed 
    std::string buildServerNoticeNickChange(const std::string& oldnick, const std::string& newnick) {
        return ":server NOTICE * :User " + oldnick + " changed nickname to " + newnick + "\r\n";
    }

	// MODE MESSAGES
   std::string buildNeedMoreParams(const std::string& clientNickname, const std::string& command) {
        return  SERVER_PREFIX +  " " + std::to_string(static_cast<int>(MsgType::NEED_MORE_PARAMS)) + " " + clientNickname + " " + command + " :Not enough parameters\r\n";
    }

    std::string buildNoSuchChannel(const std::string& clientNickname, const std::string& channelName) {
        return  SERVER_PREFIX +  " " + std::to_string(static_cast<int>(MsgType::NO_SUCH_CHANNEL)) + " " + clientNickname + " " + channelName + " :No such channel\r\n";
    }

    std::string buildNotInChannel(const std::string& clientNickname, const std::string& channelName) {
        return  SERVER_PREFIX +  " " +  std::to_string(static_cast<int>(MsgType::NOT_ON_CHANNEL)) + " " + channelName + " " + clientNickname + " :not on that channel\r\n";
    } // changed from NOT_IN_CHANNEL
    std::string buildInvalidChannelName(const std::string& clientNickname, const std::string& channelName, const std::string& msg) {
        return  SERVER_PREFIX +  " " +  std::to_string(static_cast<int>(MsgType::INVALID_CHANNEL_NAME)) + " " + clientNickname + " " + channelName + " " + msg + "\r\n";
    }
    std::string buildNotOperator(const std::string& clientNickname, const std::string& channelName) {
        return  SERVER_PREFIX + " " + std::to_string(static_cast<int>(MsgType::NOT_OPERATOR)) + " " + clientNickname + " " + channelName + " :You're not channel operator\r\n";
    }

    std::string buildInvalidTarget(const std::string& clientNickname, const std::string& target) {
        // This message more specific if needed
        return  SERVER_PREFIX + std::to_string(static_cast<int>(MsgType::INVALID_TARGET)) + " " + clientNickname + " :Cant change mode for other "+target+"users\r\n";
    }

    std::string buildChannelModeIs(const std::string& clientNickname, const std::string& channelName, const std::string& modeString, const std::string& modeParams) {
        return  SERVER_PREFIX + " " + std::to_string(static_cast<int>(MsgType::RPL_CHANNELMODEIS)) + " " + clientNickname + " " + channelName + " " + modeString + (modeParams.empty() ? "" : " " + modeParams) + "\r\n";
    }
   	std::string buildChanUpdate(const std::string& clientNickname, const std::string& username, const std::string& channelName) {
        return prefix(clientNickname, username) + " JOIN " +  channelName + "\r\n";
    }

	std::string buildUModeIs(const std::string& clientNickname, const std::string& modeString) {
        return  SERVER_PREFIX + std::to_string(static_cast<int>(MsgType::RPL_UMODEIS)) + " " + clientNickname + " " + modeString + "\r\n";
    }

	std::string buildChannelModeChange(const std::string& clientNickname, const std::string& username, const std::string channelname, const std::string& modes, const std::string& targets) {
        return  prefix(clientNickname, username) +" MODE " + channelname + " " + modes + " " + targets + "\r\n";
    }

	std::string buildClientModeChange(const std::string channelname, const std::string& modes) {
        return  ":**:" + channelname +" MODE " + channelname  + " :" + modes + "\r\n**";
    } 

	std::string buildInviteOnlyChannel(const std::string& clientNickname, const std::string& channelName) {
	    return SERVER_PREFIX + " " + std::to_string(static_cast<int>(MsgType::INVITE_ONLY)) +
	           " " + clientNickname + " " + channelName + " :Cannot join channel (+i)\r\n";
	}

	std::string buildIncorrectPasswordMessage(const std::string& clientNickname, const std::string& channelName) {
	    return SERVER_PREFIX + " " + std::to_string(static_cast<int>(MsgType::INVALID_PASSWORD)) +
	           " " + clientNickname + " " + channelName + " :Cannot join channel (+k)\r\n";
	}

	std::string buildInviting(const std::string& sender_nickname, const std::string& target_nickname, const std::string& channel_name) {
    	return SERVER_PREFIX + " 341 " + sender_nickname + " " + target_nickname + " :" + channel_name + "\r\n";
	}

	std::string buildGetInvite(const std::string& nickname, const std::string& username, const std::string& target, const std::string& channel_name) {
		return prefix(nickname, username) +  " INVITE " + target + " :" + channel_name + "\r\n";
	}

	std::string buildUserOnChannel(const std::string& inviter_nickname, const std::string& target_nickname, const std::string& channel_name) {
		return SERVER_PREFIX + " 443 " + inviter_nickname + " " + target_nickname + " " + channel_name + " :is already on channel\r\n";
	}
	
	std::string buildIvalidChannelName(const std::string& clientNickname, const std::string& channelName) {
		return SERVER_PREFIX + " " + std::to_string(static_cast<int>(MsgType::INVALID_CHANNEL_NAME)) +
			   " " + clientNickname + " " + channelName + " :illegal channel name\r\n";
	}
	// add to above
	std::string buildChannelIsFull(const std::string& clientNickname, const std::string& channelName) {
	    return SERVER_PREFIX + " " + std::to_string(static_cast<int>(MsgType::CHANNEL_FULL)) +
	           " " + clientNickname + " " + channelName + " :Cannot join channel (+l)\r\n";
	}

	std::string buildTopicChange(const std::string& clientNickname, const std::string& username, const std::string& channelName, const std::string& topic) {
		return prefix(clientNickname, username) +  " TOPIC " + channelName + " :" + topic + "\r\n";
	}

	std::string buildPart(const std::string& clientNickname, const std::string& username, const std::string& channelName, const std::string& partReason) {
	    return prefix(clientNickname, username) + " PART " + channelName + " :" + partReason + "\r\n";
	}

	std::string buildKick(const std::string& clientNickname, const std::string& username, const std::string& channelName, const std::string& target, const std::string& kickReason) {
	    return prefix(clientNickname, username) + " KICK " + channelName + " " + target + " :" + kickReason + "\r\n";
	}
	std::string buildPrivMessage(const std::string& clientNickname, const std::string& username, const std::string& where, const std::string& msg){
		return ":" + prefix(clientNickname, username)+ " PRIVMSG " + where + " " + msg +"\r\n";  
	}

	std::string buildWhois(const std::vector<std::string>& p) {
    // Expected indices:
    // [0] = requester nickname
    // [1] = target nickname
    // [2] = target username
    // [3] = target full name
    // [4] = idle seconds (as string)
    // [5] = signon time (as string)
    // [6] = channels string (may be empty)

    std::string msg;

    // WHOISUSER (311)
    msg += SERVER_PREFIX + " 311 " + p[0] + " " + p[1] + " " + p[2] + " " + SERVER_AT +
           " * :" + p[3] + "\r\n";

    // WHOISSERVER (312)
    msg += SERVER_PREFIX + " 312 " + p[0] + " " + p[1] + " " + SERVER +
           " :A & J IRC server\r\n";

    // WHOISIDLE (317)
    msg += SERVER_PREFIX + " 317 " + p[0] + " " + p[1] + " " + p[4] + " " + p[5] +
           " :seconds idle, signon time\r\n";

    // WHOISCHANNELS (319) — optional if [6] is not empty
    if (p.size() > 6 && !p[6].empty()) {
        msg += SERVER_PREFIX + " 319 " + p[0] + " " + p[1] + " :" + p[6] + "\r\n";
    }

    // ENDOFWHOIS (318)
    msg += SERVER_PREFIX + " 318 " + p[0] + " " + p[1] + " :End of WHOIS list\r\n";

    return msg;
}

}
 // namespace MessageBuilder