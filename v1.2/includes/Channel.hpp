#pragma once

#include <string>
#include <set>
#include <vector>
#include <iostream>
#include <bitset>
#include <map>
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
/*struct WeakPtrCompare {
    bool operator()(const std::weak_ptr<Client>& lhs, const std::weak_ptr<Client>& rhs) const {
        auto l = lhs.lock();  // Convert weak_ptr to shared_ptr safely
        auto r = rhs.lock();  

        if (!l || !r) return false;  // Prevent comparisons if either pointer is expired

        return l->getNickname() < r->getNickname();
    }
};*/

struct WeakPtrCompare {
    bool operator()(const std::weak_ptr<Client>& lhs, const std::weak_ptr<Client>& rhs) const {
        // This provides a strict weak ordering for weak_ptrs (and shared_ptrs),
        // based on the address of their internal control block.
        // It correctly handles expired and null weak_ptrs consistently.
        return lhs.owner_before(rhs);
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
		int _ClientLimit = 0;
		int _clientCount = 0;
		std::string _password;
		std::map<std::weak_ptr<Client>, std::pair<std::bitset<config::CLIENT_NUM_MODES>, int>, WeakPtrCompare> _ClientModes;  // Nicknames -> Bitset of modes
		std::bitset<config::CHANNEL_NUM_MODES> _ChannelModes;
		std::deque<std::string> _invites;
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
		const std::string getAllNicknames();
		std::weak_ptr<Client> getWeakPtrByNickname(const std::string& nickname);
		std::map<std::weak_ptr<Client>, std::pair<std::bitset<config::CLIENT_NUM_MODES>, int>, WeakPtrCompare> getAllClients() {return _ClientModes;};
		std::bitset<config::CLIENT_NUM_MODES>& getClientModes(const std::string nickname);
		//std::bitset<config::CHANNEL_NUM_MODES>& getChannelModes();
		std::string getCurrentModes() const;
		//std::weak_ptr<Client> getElementByFd(const int fd);
		std::string getNicknameFromWeakPtr(const std::weak_ptr<Client>& weakClient);
		//, const std::string pass, std::map<std::string, int> listOfClients,  FuncType::setMsgRef setMsgType
		int setClientMode(std::string mode, std::string nickname, std::string currentClientName); // const 
		int setChannelMode(std::string mode, std::string nickname, std::string currentClientName, std::string channelname, const std::string pass, std::map<std::string, int>& listOfClients,  std::function<void(MsgType, std::vector<std::string>&)> setMsgType);
		Modes::ClientMode charToClientMode(const char& modeChar);
		Modes::ChannelMode charToChannelMode(const char& modeChar); 
		//void setChannelMode(std::string mode);
		bool setModeBool(char onoff);
		bool canClientJoin(const std::string& nickname, const std::string& password );


		void setTopic(const std::string& newTopid);
		bool addClient(std::shared_ptr <Client> Client);
		bool removeClient(std::string nickname);

		bool isClientInChannel(const std::string& nickname) const {
			for (const auto& entry : _ClientModes) {
        		if (auto clientPtr = entry.first.lock(); clientPtr && clientPtr->getNickname() == nickname) {
            		return true;
        	}
			return false;
    }
	// we could substitute with a throw here
    return {};  // return empty weak_ptr if no match is found
		};

//		bool isClientInChannel(Client* Client) const;

		//const std::set<Client*>& getClient() const;
		//bool addOperator(Client* Client);
		//bool removeOperator(Client* Client);
		bool isOperator(Client* Client) const;
		//void broadcastMessage(const std::string& message, Client* sender = nullptr) const;
		//void setMode(const std::string& mode, Client* Client);
		//void removeMode(const std::string& mode, Client* Client);
		void clearAllChannel() {
			_ClientModes.clear();
		};

};

