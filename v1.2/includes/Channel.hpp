// #pragma once

// // Standard Library Includes (alphabetical for consistency)
// #include <algorithm>
// #include <bitset>
// #include <ctime>      // For std::time_t
// #include <deque>
// #include <iostream>
// #include <map>
// #include <memory>     // For shared_ptr and weak_ptr
// #include <optional>
// #include <set>
// #include <string>
// #include <vector>

// // Project-Specific Includes
// #include "config.h"
// #include "Client.hpp" // Assuming Client.hpp defines the Client class

// // Global Helper Structs/Functions (must be defined before classes that use them)

// /**
//  * @brief A custom comparator, since we use weak_ptrs as keys and they do not
//  * have comparison operators defined, a map can not order the keys, this comparitor
//  * allows for ordering as it utlaizes lock() to switch them to shared_ptrs for that.abs
//  *
//  * if either left or right weak_ptr is expired it prevents us from comapring them .
//  *
//  */
// struct WeakPtrCompare {
//     bool operator()(const std::weak_ptr<Client>& lhs, const std::weak_ptr<Client>& rhs) const {
//         // This provides a strict weak ordering for weak_ptrs (and shared_ptrs),
//         // based on the address of their internal control block.
//         // It correctly handles expired and null weak_ptrs consistently.
//         return lhs.owner_before(rhs);
//     }
// };

// // Removed the global definition of 'getNicknameFromWeakPtr' from here,
// // as the compiler error indicates it's intended to be a member function
// // and its definition is in Channel.cpp as 'Channel::getNicknameFromWeakPtr'.

// class Channel {
//     public:
//         // Constructors and Destructor
//         Channel(const std::string& channelName);
//         ~Channel();

//         // Getters (all const methods first, then non-const, grouped by type/purpose)

//         // -- Basic Channel Information --
//         const std::string& getName() const;
//         const std::string& getTopic() const;
//         int getOperatorCount() {return _operatorCount;};
//         const unsigned long& getClientCount() {return _clientCount;}; //const
//         std::string getCurrentModes() const;
//         bool isEmpty() const;
//         bool getwasOpRemoved() {return _wasOpRemoved;};
//         bool CheckChannelMode(Modes::ChannelMode comp) const {return _ChannelModes.test(static_cast<std::size_t>(comp));}; //ref


//         // -- Client/Member Information --
//         std::map<std::weak_ptr<Client>, std::pair<std::bitset<config::CLIENT_NUM_MODES>, int>, WeakPtrCompare> getAllClients() {return _ClientModes;};
//         std::vector<int> getAllfds();
//         const std::string getAllNicknames();
//         std::string getClientModePrefix(std::shared_ptr<Client> client) const ;
//         // This function is now correctly declared as a member function
//         std::string getNicknameFromWeakPtr(const std::weak_ptr<Client>& weakClient);


//         // Setters / Modifiers
//         void setwasOpRemoved() {_wasOpRemoved = false;};
//         void setOperatorCount(int value) {
//             if (_operatorCount == 0){
//                 return;
//             }
//             _operatorCount += value;
//             if (_operatorCount < 0) _operatorCount = 0; // extra safeguard
//         };
//         void setTopic(const std::string& newTopic);


//         // Client Lifecycle Management (Join/Part/Presence)
//         std::optional<std::pair<MsgType, std::vector<std::string>>> canClientJoin(const std::string& nickname, const std::string& password);
//         int addClient(std::shared_ptr <Client> Client);
//         bool removeClientByNickname(const std::string& nickname);
//         bool isClientInChannel(const std::string& nickname) const;
//         void clearAllChannel() {
//             _ClientModes.clear();
//         };
//         std::pair<MsgType, std::vector<std::string>> promoteFallbackOperator(const std::shared_ptr<Client>& removingClient, bool isLeaving);


//         // Channel Mode Operations
//         std::bitset<config::CLIENT_NUM_MODES>& getClientModes(const std::string nickname); //ref
//         std::vector<std::string> setChannelMode(char modeChar , bool enableMode, const std::string& target);
//         Modes::ClientMode charToClientMode(const char& modeChar);
//         Modes::ChannelMode charToChannelMode(const char& modeChar);
//         bool setModeBool(char onoff);
//         bool isValidChannelMode(char modeChar) const;
//         bool isValidClientMode(char modeChar) const;
//         bool channelModeRequiresParameter(char modeChar) const;
//         std::pair<MsgType, std::vector<std::string>> initialModeValidation( const std::string& ClientNickname, size_t paramsSize);
//         std::pair<MsgType, std::vector<std::string>> modeSyntaxValidator( const std::string& requestingClientNickname, const std::vector<std::string>& params ) const;
//         std::vector<std::string> applymodes(std::vector<std::string> params);
//         MsgType checkModeParameter(const std::string& nick, char mode, const std::string& param, char sign) const;


//         // Invite List Management
//         void addInvite(const std::string& nickname);
//         bool isInvited(const std::string& nickname) const;
//         void removeInvite(const std::string& nickname);


//     private:
//         // Member Variables (grouped for clarity)

//         // -- Channel Identification and State --
//         std::string _name;
//         std::string _topic;
//         std::string _password;
//         std::string _topicSetter;     // Stores the nickname of the user who last set the topic
//         std::time_t _topicSetTime;    // Stores the Unix timestamp (from <ctime>) when the topic was last set
//         int _ClientLimit = 0;
//         unsigned long _ulimit;
//         unsigned long _clientCount = 0;
//         int _operatorCount = 0;
//         bool _wasOpRemoved = false;

//         // -- Data Structures --
//         std::map<std::weak_ptr<Client>, std::pair<std::bitset<config::CLIENT_NUM_MODES>, int>, WeakPtrCompare> _ClientModes;  // Nicknames -> Bitset of modes
//         std::bitset<config::CHANNEL_NUM_MODES> _ChannelModes;
//         std::deque<std::string> _invites;

//         // Private Helper Methods (if any)
// };









#pragma once

#include <string>
#include <set>
#include <vector>
#include <iostream>
#include <bitset>
#include <map>
#include <algorithm>
#include <optional>
#include <ctime>

#include "config.h"
#include "Client.hpp"

/**
 * @brief a custome comparator, since we use weak_ptrs as keys and they do not 
 * have comparison operators defined, a map can not order the keys, this comparitor
 * allows for ordering as it utlaizes lock() to switch them to shared_ptrs for that.abs
 * 
 * if either left or right weak_ptr is expired it prevents us from comapring them . 
 * 
 */

struct WeakPtrCompare {
    bool operator()(const std::weak_ptr<Client>& lhs, const std::weak_ptr<Client>& rhs) const {
        // This provides a strict weak ordering for weak_ptrs (and shared_ptrs),
        // based on the address of their internal control block.
        // It correctly handles expired and null weak_ptrs consistently.
        return lhs.owner_before(rhs);
    }
};

/**
 * @brief std::string getNicknameFromWeakPtr(const std::weak_ptr<Client>& weakClient) {
    if (auto clientPtr = weakClient.lock()) {  // ✅ Convert weak_ptr to shared_ptr safely
        return clientPtr->getNickname();  // ✅ Get the current nickname live
    }
    return "";  // 🔥 Return empty string if the Client no longer exists
}
 */

 class Channel {
	private:
		int _ClientLimit = 0;
		unsigned long _clientCount = 0;
		int _operatorCount = 0;
		unsigned long _ulimit;
		bool _wasOpRemoved = false;
		std::string _name;
		std::string _topic;
		std::string _password;
		std::string _topicSetter; // Stores the nickname of the user who last set the topic
		std::time_t _topicSetTime; // Stores the Unix timestamp (from <ctime>) when the topic was last set


		std::map<std::weak_ptr<Client>, std::pair<std::bitset<config::CLIENT_NUM_MODES>, int>, WeakPtrCompare> _ClientModes;  // Nicknames -> Bitset of modes

		std::bitset<config::CHANNEL_NUM_MODES> _ChannelModes;

		std::deque<std::string> _invites;

	public:
		Channel(const std::string& channelName);
		~Channel();

		const std::string& getName() const;
		const std::string& getTopic() const;
		int getOperatorCount() {return _operatorCount;};
		bool getwasOpRemoved() {return _wasOpRemoved;};
		void setwasOpRemoved() {_wasOpRemoved = false;};
		void setOperatorCount(int value) {
			if (_operatorCount == 0){
				return;
			}
		 	_operatorCount += value;
    		if (_operatorCount < 0) _operatorCount = 0; // extra safeguard
		};
		const unsigned long& getClientCount() {return _clientCount;}; //const
		std::vector<int> getAllfds();
		const std::string getAllNicknames();
		//std::weak_ptr<Client> getWeakPtrByNickname(const std::string& nickname);
		std::map<std::weak_ptr<Client>, std::pair<std::bitset<config::CLIENT_NUM_MODES>, int>, WeakPtrCompare> getAllClients() {return _ClientModes;};

		std::bitset<config::CLIENT_NUM_MODES>& getClientModes(const std::string nickname); //ref
		bool CheckChannelMode(Modes::ChannelMode comp) const {return _ChannelModes.test(static_cast<std::size_t>(comp));}; //ref
		
		std::string getCurrentModes() const;
		std::string getNicknameFromWeakPtr(const std::weak_ptr<Client>& weakClient);
		//std::vector<std::shared_ptr<Client>> getActiveClients() const;

		std::vector<std::string> setChannelMode(char modeChar , bool enableMode, const std::string& target);
		
		Modes::ClientMode charToClientMode(const char& modeChar);
		Modes::ChannelMode charToChannelMode(const char& modeChar); 
		bool setModeBool(char onoff);
		//bool canClientJoin(const std::string& nickname, const std::string& password );
		std::optional<std::pair<MsgType, std::vector<std::string>>> canClientJoin(const std::string& nickname, const std::string& password);
		int addClient(std::shared_ptr <Client> Client);
		// void setTopic(const std::string& newTopid); wwrong spelling - Topid/Topic
		void setTopic(const std::string& newTopic);
		//bool removeClient(const std::string& nickname);
	    //NEWNEW void removeClientByNickname(const std::string& nickname);
		bool removeClientByNickname(const std::string& nickname);

		bool isClientInChannel(const std::string& nickname) const;
		//bool isClientOperator(const std::string& nickname);
		bool isValidChannelMode(char modeChar) const;
		bool isValidClientMode(char modeChar) const;
		bool channelModeRequiresParameter(char modeChar) const;

		std::pair<MsgType, std::vector<std::string>> initialModeValidation( const std::string& ClientNickname, size_t paramsSize);
		std::pair<MsgType, std::vector<std::string>> modeSyntaxValidator( const std::string& requestingClientNickname, const std::vector<std::string>& params ) const;
		std::vector<std::string> applymodes(std::vector<std::string> params);

	    bool isEmpty() const;

		//void broadcast(const std::string& message, std::shared_ptr<Client> exclude_client = nullptr);
// T O P I C 
		//const std::string& getTopicSetter() const;
		//std::time_t getTopicSetTime() const;
		//void setTopicSetter(const std::string& setter_nickname);
		//void setTopicSetTime(std::time_t set_time);

// I N V I T E
		void addInvite(const std::string& nickname);
		bool isInvited(const std::string& nickname) const; // To check if a client is on the invite list
		void removeInvite(const std::string& nickname); // To remove client from invite list after they join

		
		std::string getClientModePrefix(std::shared_ptr<Client> client) const ;
		MsgType checkModeParameter(const std::string& nick, char mode, const std::string& param, char sign) const;
		std::pair<MsgType, std::vector<std::string>> promoteFallbackOperator(const std::shared_ptr<Client>& removingClient, bool isLeaving);
		void clearAllChannel() {
			_ClientModes.clear();
		};
};

