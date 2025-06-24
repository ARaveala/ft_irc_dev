#pragma once

#include "Client.hpp"
#include <string>
#include <iostream>
#include <memory>
#include <ctime>

class Channel
{
	public:
		Channel(const std::string &name);
		~Channel();

		// Setters
		void setInviteOnly(bool value) { _inviteOnly = value; }
		void setTopicRestrictions(bool value) { _topicRestrictions = value; }
		void setIsChannelPassword(bool value) { _isChannelPassword = value; }
		void setUserLimit(int value) { _userLimit = value; }
		void setChannelKey(bool plussign, std::string modeParam);

		// Getters
		bool getInviteOnly() const { return _inviteOnly; }
		bool getTopicRestrictions() const { return _topicRestrictions; }
		bool getIsChannelPassword() const { return _isChannelPassword; }
		int getUserLimit() const { return _userLimit; }
		int getUserCount() const { return _userCount; }

		std::string getName() const { return _name; }
		std::string getTopic() const { return _topic; }
		std::string getPassword() const { return _password; }
		std::string getCreationTime() const { return _creationTime; }

		// Channel actions
		void addClient(const std::shared_ptr<Client>& client);
		void removeClient(const std::shared_ptr<Client>& client);
		int isClient(const std::shared_ptr<Client>& client) const;
		std::vector<std::shared_ptr<Client>> getClients() const { return _clients; }
		std::shared_ptr<Client> getClient(int i) const { return _clients[i]; }
		void incrementUserCount() { _userCount++; }
		void decrementUserCount() { _userCount--; }

		// Admin actions
		int isAdmin(const std::shared_ptr<Client>& client) const;
		void addAdmin(const std::shared_ptr<Client>& admin);
		void removeAdmin(const std::shared_ptr<Client>& admin);
		std::vector<std::shared_ptr<Client>> getAdmins() const { return _admins; }

		// Invite actions
		void addInvited(const std::shared_ptr<Client>& client);
		void removeInvited(const std::shared_ptr<Client>& client);
		int isInvited(const std::shared_ptr<Client>& client) const;
		std::vector<std::shared_ptr<Client>> getInvited() const { return _invited; }

		// broadcast
		void broadcast(const std::string &msg, const std::shared_ptr<Client>& client, int flag);
		void broadcastTopic(const std::string &chName, const std::string &topic);
		void broadcastAdmins(const std::string &msg);

		// mode and topic
		void setTopic(const std::string &topic, const std::shared_ptr<Client>& admin);
		std::string getChannelModes();
	
	private:
		bool _inviteOnly;
		bool _topicRestrictions;
		bool _isChannelPassword;
		int _userLimit;
		int _userCount;
	
		std::string _creationTime;
		std::string _name;
		std::string _topic;
		std::string	_password;
		std::vector<std::shared_ptr<Client>> _admins;
		std::vector<std::shared_ptr<Client>> _clients;
		std::vector<std::shared_ptr<Client>> _invited;
};

std::ostream& operator<<(std::ostream& os, const Channel& channel);

