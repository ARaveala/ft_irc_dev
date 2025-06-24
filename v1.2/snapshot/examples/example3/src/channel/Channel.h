#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include <iostream>
#include <string>
#include <map>
#include <utility>
#include <memory>
#include <cctype>
#include <set>
#include <chrono>

#include "../client/Client.h"
#include "../common/MagicNumbers.h"
#include "../server/Server.h"

class Client;
class Server;
class Channel :public std::enable_shared_from_this<Channel>
{
	private:
		std::string 										name_; // Channel name
		std::string 										channel_key_; // Channel key
		bool												topic_is_set_; // topic is set
		bool 												mode_t_; // Topic lock mode
		bool												mode_i_; // Invite-only mode
		bool												mode_k_; // Key-protected mode
		bool 												mode_l_; // User limit mode
		bool												mode_n_; // no external messages
		std::string 										mode_; // Channel modes
		unsigned int 										limit_;
		std::set<std::string> 								invited_users_;
		std::pair<std::string, std::string> 				topic_; // Channel topic (author, topic)
		std::chrono::time_point<std::chrono::system_clock>	channel_creation_timestamps_;
		std::chrono::time_point<std::chrono::system_clock>	topic_timestamp_;
		std::map<std::weak_ptr<Client>, bool, std::owner_less<std::weak_ptr<Client>>>	 			users_; // Users in the channel and their operator status (true if op)
	public:
		Channel(const std::string &name);
		~Channel();

		// Accessor methods
		std::string const 								&getName() const;
		std::string const 								&getChannelKey() const;
		unsigned int const 								&getChannelLimit() const;
		std::pair<std::string, std::string> const 		&getTopic() const;
		bool const 										&getModeT() const;
		bool const 										&getModeI() const;
		bool const 										&getModeK() const;
		bool const 										&getModeL() const;
		bool const 										&getModeN() const;
		std::string const 								&getMode() const;
		bool const 										&hasTopic() const;
		bool const 										&isInviteOnly() const;
		std::chrono::time_point<std::chrono::system_clock> const &getChannelCreationTimestamps() const;
		std::map<std::weak_ptr<Client>, bool, std::owner_less<std::weak_ptr<Client>>> const 	&getUsers() const;


		// Mutator methods
		void setName(const std::string &name);
		void setUserCount(unsigned int usercount);
		void setChannelKey(const std::string &channel_key);
		void setTopic(const std::pair<std::string, std::string> &topic);
		void setModeT(bool mode_t);
		void setModeI(bool mode_i);
		void setModeK(bool mode_k);
		void setModeL(bool mode_l, unsigned int limit = DEFAULT_MAX_CHAN_CLIENTS);
		void setModeN(bool mode_n);
		void setChannelCreationTimestamps();



		// Functional methods
    bool isFull() const;
    bool isPasswordProtected() const;
    void addUser(const std::weak_ptr<Client> &client_ptr, bool is_channel_op);
    void removeUser(const std::weak_ptr<Client> &client_ptr);
    bool isUserOnChannel(const std::string &nickname);
    bool userIsOperator(const std::string &nickname);
    bool isValidChannelName(const std::string &channelName) const;
    void broadcastMessage(const std::weak_ptr<Client> &sender_ptr, const std::string &message, Server *server_ptr);
    void broadcastMessageToAll(const std::string &message, Server *server_ptr);
    bool changeOpStatus(const std::weak_ptr<Client> &client_ptr, bool status);
    bool isOperator(const std::weak_ptr<Client> &client_ptr);
    bool canChangeTopic(const std::weak_ptr<Client> &client_ptr);
    bool isCorrectPassword(const std::string &given_password);
    bool isUserInvited(const std::string &nickname) const;
    void addUserToInvitedList(const std::string &nickname);
    void removeUserFromInvitedList(const std::string &nickname);
    void sendTopicToClient(const std::weak_ptr<Client> &client_ptr, Server *server_ptr);

    void clearTopic(const std::string &nickname);
    bool isEmpty();
};

#endif// CHANNEL_H
