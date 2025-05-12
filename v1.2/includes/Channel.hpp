#pragma once

#include <string>
#include <set>
#include <vector>
#include <iostream>
#include <bitset>
#include <map>

#include "Client.hpp"

class Channel {
	private:
		std::string _name;
		std::string _topic;
		std::unordered_map<std::string, std::bitset<4>> _userModes;  // Nickname -> Bitset of modes
		std::map<int, std::shared_ptr<Client>>& _clients;
	public:
		Channel(const std::string& channelName, std::map<int, std::shared_ptr<Client>>& clients);
		~Channel();
		const std::string& getName() const;
		const std::string& getTopic() const;
		void setTopic(const std::string& newTopid);
		bool addUser(User* user);
		bool removeUser(User* user);
		bool isUserInChannel(User* user) const;
		const std::set<User*>& getUser() const;
		bool addOperator(User* user);
		bool removeOperator(User* user);
		bool isOperator(User* user) const;
		void broadcastMessage(const std::string& message, User* sender = nullptr) const;
		void setMode(const std::sting& mode, User* user);
		void removeMode(const std::string& mode, User* user);

		const int MODE_OPERATOR = 0;
		const int MODE_VOICE = 1;
		const int MODE_INVITE_ONLY = 2;
		const int MODE_MODERATED = 3;
};

