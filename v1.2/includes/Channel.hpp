#pragma once

#include <string>
#include <set>
#include <vector>
#include <iostream>

#include "Client.hpp"

class Channel {
	private:
		std::string name;
		std::string topic;
		std::set<Client*> clients; // should this be something else?
		std::set<Client*> operators;
		// modes? single char flags to indicate roles of clients.
	public:
		Channel(const std::string& channelName);
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
};

