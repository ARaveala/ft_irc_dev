#pragma once

#include <vector>
#include <memory>
#include <map>
#include <deque>
#include <set>
#include <algorithm>
#include <cctype>
#include <bitset>
#include <functional>

#include "config.h"
#include "IrcResources.hpp"

class Client;
class Server;

class IrcMessage {
	private:
		size_t _bytesSentForCurrentMessage = 0;

		std::deque<std::string> _messageQue;

		std::string _prefix;
		std::string _command;

		// Changed to_lowercase to public, as it's a utility function used externally.
		// If you intend for it to be an internal helper, it should be part of MessageBuilder or Server.
		// For now, making it public to resolve compilation issues.
		// static const std::set<std::string> _illegal_nicknames; // Removed static initialization

		std::vector<std::string> _paramsList;
		std::vector<std::string> _params;
		std::bitset<config::MSG_TYPE_NUM> _msgState;

		MsgType _activeMsg = MsgType::NONE;


	public:
		// Made public to allow external access (e.g., from Server.cpp)
		static std::string to_lowercase(const std::string& s) {
			std::string lower_s = s;
			std::transform(lower_s.begin(), lower_s.end(), lower_s.begin(),
			[](unsigned char c){ return std::tolower(c); });
			return lower_s;
		}

    	IrcMessage();
    	~IrcMessage();

		bool parseMessage(std::string& raw_data);
		void queueMessage(const std::string& message);
		std::string& getQueueMessage();
		void removeQueueMessage();
		void advanceCurrentMessageOffset(size_t bytes_sent);
		size_t getRemainingBytesInCurrentMessage() const;
		size_t getBytesSentForCurrentMessage() const; // Added previously
		void resetCurrentMessageOffset();

		const std::string& getPrefix() const;
		const std::string& getCommand() const;
		const std::vector<std::string>& getParams() const;
		std::string toRawString() const; // For debugging/reconstruction

		void setPrefix(const std::string& prefix);
		void setCommand(const std::string& command);

		const std::string getParam(unsigned long index) const; // Getter for specific param
		void queueMessageFront(const std::string& msg);
		const std::deque<std::string>& getQue() const; // Getter for message queue
		void clearQue(); // Clear the message queue

		const std::string getMsgParam(int index) const;
		void changeTokenParam(int index, const std::string& newValue);
		const std::vector<std::string>& getMsgParams(); // Returns a reference to the parameters vector

		void setType(MsgType msg, std::vector<std::string> sendParams);
		void clearAllMsg(); // Clears all message-related state
		static void printMessage(const IrcMessage& msg); // Static print for debugging

		bool isActive(MsgType type); // Checks if a specific message type is active
		MsgType getActiveMessageType() const; // Returns the currently active message type
};
