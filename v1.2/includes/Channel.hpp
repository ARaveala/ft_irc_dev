#pragma once

#include <string>
#include <set>
#include <vector>
#include <iostream>
#include <bitset>
#include <map>

#include "Client.hpp"

struct WeakPtrCompare {
    bool operator()(const std::weak_ptr<Client>& lhs, const std::weak_ptr<Client>& rhs) const {
        auto l = lhs.lock();  // Convert weak_ptr to shared_ptr safely
        auto r = rhs.lock();  

        if (!l || !r) return false;  // Prevent comparisons if either pointer is expired

        return l->getNickname() < r->getNickname();
    }
};

class Channel {
	private:
		std::string _name;
		std::string _topic;
		std::map<std::weak_ptr<Client>, std::bitset<4>, WeakPtrCompare> _ClientModes;  // Nicknames -> Bitset of modes
		// this does need its own map your right, only of the joined clienst focourse
		// duh
		///std::map<int, std::shared_ptr<Client>>& _clients;
	public:
		//Channel(const std::string& channelName, std::map<int, std::shared_ptr<Client>>& clients);
		Channel(const std::string& channelName);
		~Channel();
		const std::string& getName() const;
		const std::string& getTopic() const;
		void setTopic(const std::string& newTopid);
		bool addClient(std::shared_ptr <Client> Client);
		bool removeClient(Client* Client);
		bool isClientInChannel(Client* Client) const;
		const std::set<Client*>& getClient() const;
		bool addOperator(Client* Client);
		bool removeOperator(Client* Client);
		bool isOperator(Client* Client) const;
		void broadcastMessage(const std::string& message, Client* sender = nullptr) const;
		void setMode(const std::string& mode, Client* Client);
		void removeMode(const std::string& mode, Client* Client);
// these could go into config.h as namespace?? these are just index values
// with fancy name
		const int MODE_OPERATOR = 0;
		const int MODE_VOICE = 1;
		const int MODE_INVITE_ONLY = 2;
		const int MODE_MODERATED = 3;
		// void changeNickname();
};

