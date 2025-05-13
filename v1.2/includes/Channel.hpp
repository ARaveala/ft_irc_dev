#pragma once

#include <string>
#include <set>
#include <vector>
#include <iostream>
#include <bitset>
#include <map>

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
        auto l = lhs.lock();  // Convert weak_ptr to shared_ptr safely
        auto r = rhs.lock();  

        if (!l || !r) return false;  // Prevent comparisons if either pointer is expired

        return l->getNickname() < r->getNickname();
    }
};

/*std::string buildNicknameListFromWeakPtr(const std::weak_ptr<Client>& weakClient) {
	std::string nickNameList = nullptr;
	while (auto clientPtr = weakClient.lock()) {  // ✅ Convert weak_ptr to shared_ptr safely
        nickNameList += clientPtr->getNickname();
		return clientPtr->getNickname();  // ✅ Get the current nickname live
    }

    return nickNameList;  // 🔥 Return empty string if the Client no longer exists
}*/
/**
 * @brief std::string getNicknameFromWeakPtr(const std::weak_ptr<Client>& weakClient) {
    if (auto clientPtr = weakClient.lock()) {  // ✅ Convert weak_ptr to shared_ptr safely
        return clientPtr->getNickname();  // ✅ Get the current nickname live
    }
    return "";  // 🔥 Return empty string if the Client no longer exists
}


 * 
 */
class Channel {
	private:
		std::string _name;
		std::string _topic;
		std::map<std::weak_ptr<Client>, std::pair<std::bitset<4>, int>, WeakPtrCompare> _ClientModes;  // Nicknames -> Bitset of modes
		// this does need its own map your right, only of the joined clienst focourse
		// duh
		///std::map<int, std::shared_ptr<Client>>& _clients;
	public:
		//Channel(const std::string& channelName, std::map<int, std::shared_ptr<Client>>& clients);
		Channel(const std::string& channelName);
		~Channel();
		const std::string& getName() const;
		const std::string& getTopic() const;
		std::vector<int> getAllfds();
		std::weak_ptr<Client> getWeakPtrByNickname(const std::string& nickname);
		std::string getNicknameFromWeakPtr(const std::weak_ptr<Client>& weakClient);

		void setTopic(const std::string& newTopid);
		bool addClient(std::shared_ptr <Client> Client);
		bool removeClient(std::string nickname);
		bool isClientInChannel(Client* Client) const;
		const std::set<Client*>& getClient() const;
		bool addOperator(Client* Client);
		bool removeOperator(Client* Client);
		bool isOperator(Client* Client) const;
		void broadcastMessage(const std::string& message, Client* sender = nullptr) const;
		//void setMode(const std::string& mode, Client* Client);
		void removeMode(const std::string& mode, Client* Client);
// these could go into config.h as namespace?? these are just index values
// with fancy name
		const std::size_t MODE_OPERATOR = 0;
		const std::size_t MODE_VOICE = 1;
		const std::size_t MODE_INVITE_ONLY = 2;
		const std::size_t MODE_MODERATED = 3;
		// void changeNickname();
		//clean up for terminal proplems
		void clearAllChannel() {
			_ClientModes.clear();
		};
};

