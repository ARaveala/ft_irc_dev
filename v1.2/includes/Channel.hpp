#pragma once

#include <string>
#include <set>
#include <vector>
#include <iostream>
#include <bitset>
#include <map>
#include <algorithm>
#include <optional>
#include <ctime> // For std::time_t and std::time(NULL)

#include "config.h"
#include "Client.hpp" // For Client class definition

/**
 * @brief a custom comparator, since we use weak_ptrs as keys and they do not
 * have comparison operators defined, a map can not order the keys, this comparator
 * allows for ordering as it utilizes lock() to switch them to shared_ptrs for that.
 *
 * If either left or right weak_ptr is expired, it prevents us from comparing them.
 */
struct WeakPtrCompare {
    bool operator()(const std::weak_ptr<Client>& lhs, const std::weak_ptr<Client>& rhs) const {
        // This provides a strict weak ordering for weak_ptrs (and shared_ptrs),
        // based on the address of their internal control block.
        // It correctly handles expired and null weak_ptrs consistently.
        return lhs.owner_before(rhs);
    }
};

class Channel {
	private:
		int _ClientLimit = 0;
		unsigned long _clientCount = 0;

		// unsigned long _ulimit; // This seems unused, consider removing if not needed.

		std::string _name;
		std::string _topic;
		std::string _password; // Used as the channel key if +k mode is set
		std::string _topicSetter; // Stores the nickname of the user who last set the topic
		std::time_t _topicSetTime; // Stores the Unix timestamp (from <ctime>) when the topic was last set


		// ClientModes map: maps weak_ptr to Client to their modes (bitset) and maybe other info
		std::map<std::weak_ptr<Client>, std::pair<std::bitset<config::CLIENT_NUM_MODES>, int>, WeakPtrCompare> _ClientModes;

		std::bitset<config::CHANNEL_NUM_MODES> _ChannelModes;

		std::deque<std::string> _invites; // List of nicknames invited to an invite-only channel

	public:
		Channel(const std::string& channelName);
		~Channel();

		// Getters for channel properties
		const std::string& getName() const;
		const std::string& getTopic() const;
		const unsigned long& getClientCount() const; // Added const
		std::vector<int> getAllfds() const; // Added const
		const std::string getAllNicknames() const; // Added const
		std::weak_ptr<Client> getWeakPtrByNickname(const std::string& nickname);

		// Corrected: Renamed getClients() to getAllClients() for consistency with existing declaration
		// And added const to return type for safety
		const std::map<std::weak_ptr<Client>, std::pair<std::bitset<config::CLIENT_NUM_MODES>, int>, WeakPtrCompare>& getAllClients() const;


		std::bitset<config::CLIENT_NUM_MODES>& getClientModes(const std::string nickname); // Use reference for modification
		bool CheckChannelMode(Modes::ChannelMode comp) const; // Added const

		std::string getChannelModeString() const; // Added for MODE command response
		const std::string& getTopicSetter() const; // Getter for topic setter
		std::time_t getTopicTime() const; // Getter for topic set time


		// Channel mode management
		void setMode(Modes::ChannelMode mode, bool enable); // Sets/unsets a channel mode
		void setClientMode(const std::string& nickname, Modes::ClientMode mode, bool enable); // Sets/unsets a client mode within the channel

		// For channel key (+k)
		const std::string& getKey() const; // ADDED: Getter for the channel key (password)
		void setKey(const std::string& key); // ADDED: Setter for the channel key

		// Utility methods
		std::string getCurrentModes() const; // Returns string representation of channel modes
		// std::string getNicknameFromWeakPtr(const std::weak_ptr<Client>& weakClient); // Removed: This is a helper, not a Channel member
		// std::vector<std::shared_ptr<Client>> getActiveClients() const; // Consider if really needed, getAllClients() is similar

		// Client management in channel
		std::optional<std::pair<MsgType, std::vector<std::string>>> canClientJoin(const std::string& nickname, const std::string& password);
		int addClient(std::shared_ptr <Client> Client);
		void setTopic(const std::string& newTopic, const std::string& setter_nickname); // Corrected spelling and added setter
		bool removeClient(const std::string& nickname); // Removes client and returns true if channel empty
		void removeClientByNickname(const std::string& nickname); // Simpler client removal without empty check (if needed)

		bool isClientInChannel(const std::string& nickname) const;
		bool isClientOperator(const std::string& nickname) const; // Added const
		bool isValidChannelMode(char modeChar) const;
		bool isValidClientMode(char modeChar) const;
		bool channelModeRequiresParameter(char modeChar) const;

		// Mode application/validation methods (might be better integrated into setMode or a helper)
		std::pair<MsgType, std::vector<std::string>> initialModeValidation( const std::string& ClientNickname, size_t paramsSize);
		std::pair<MsgType, std::vector<std::string>> modeSyntaxValidator( const std::string& requestingClientNickname, const std::vector<std::string>& params ) const;
		// std::vector<std::string> applymodes(std::vector<std::string> params); // Consider if this is still needed or integrated

	    bool isEmpty() const;

		// Invite list management
		void addInvited(const std::string& nickname); // ADDED: To add a nickname to the invite list
		bool isInvited(const std::string& nickname) const; // To check if a client is on the invite list
		void removeInvited(const std::string& nickname); // ADDED: To remove client from invite list after they join (or are kicked)
};
