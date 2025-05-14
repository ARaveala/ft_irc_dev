#include "Channel.hpp"
#include <iostream>

/*Channel::Channel(const std::string& channelName, std::map<int, std::shared_ptr<Client>>& clients) : _name(channelName), _topic(""), _clients(clients) {
    std::cout << "Channel '" << _name << "' created." << std::endl;
}*/

Channel::Channel(const std::string& channelName)  : _name(channelName){
	std::cout << "Channel '" << _name << "' created." << std::endl;
}
Channel::~Channel() {
    std::cout << "Channel '" << _name << "' destroyed." << std::endl;
}

const std::string& Channel::getName() const {
    return _name;
}

const std::string& Channel::getTopic() const {
    return _topic;
}

std::vector<int> Channel::getAllfds() {
	std::vector<int> fds;
	for (const auto& entry : _ClientModes) {
		if (auto clientPtr = entry.first.lock()) {  // Convert weak_ptr to shared_ptr safely, anny expired pointers will be ignored , oohlalal
			fds.push_back(entry.second.second);  //Retrieve FD from stored pair (bitset, FD)
		}
	}
	return fds;
}

// rename to refer t getting a user list for channel
const std::string Channel::getAllNicknames() {
	std::string list;
	for (const auto& entry : _ClientModes) {
		if (auto clientPtr = entry.first.lock()) {  // Convert weak_ptr to shared_ptr safely, anny expired pointers will be ignored , oohlalal
			list += clientPtr->getNickname() + "!" + clientPtr->getNickname() + "@localhost ";
		}
	}
	return list;
}

std::string Channel::getNicknameFromWeakPtr(const std::weak_ptr<Client>& weakClient) {
    if (auto clientPtr = weakClient.lock()) {  //  Convert weak_ptr to shared_ptr safely
        return clientPtr->getNickname();  //  Get the current nickname live
    }
    return "";  //eturn empty string if the Client no longe
}

std::weak_ptr<Client> Channel::getWeakPtrByNickname(const std::string& nickname) {
    for (const auto& entry : _ClientModes) {
        if (auto clientPtr = entry.first.lock(); clientPtr && clientPtr->getNickname() == nickname) {
            return entry.first;  // return the matching weak_ptr
        }
    }
	// we could substitute with a throw here
    return {};  // return empty weak_ptr if no match is found
}

void Channel::setTopic(const std::string& newTopic) {
    _topic = newTopic;
}

bool Channel::addClient(std::shared_ptr <Client> client) {
    //std::set::insert returns a pair: iterator to the element and a boolean indicating insertion
	
	if (!client)
		return false; // no poopoo pointers
	std::weak_ptr<Client> weakclient = client;
	//_ClientModes.emplace(weakclient, std::make_pair(std::bitset<4>(), client->getFd()));
	//_ClientModes[weakclient].first.set(MODE_OPERATOR);

	if (_ClientModes.find(weakclient) == _ClientModes.end()) {
		_ClientModes.insert({weakclient, std::make_pair(std::bitset<4>(), client->getFd())});  // ✅ Insert new client
	} else {
		std::cout << "Client already exists in channel!" << std::endl;
	}
	for (auto it = _ClientModes.begin(); it != _ClientModes.end(); it++)
	{
		std::cout<<"show me the fds in the clientmodes map = "<<it->second.second<<"\n";
	}
	/*if (result.second) {
        if (Client) std::cout << Client->getNickname() << " joined channel " << _name << std::endl;
    }*/
    return true; // Return true if insertion happened (Client was not already there)
}

/*
 *
 * @param Client
 * @return true
 * @return false
 */
bool Channel::removeClient(std::string nickname) {
    // std::set::erase returns the number of elements removed (0 or 1 for a set)
	//std::weak_ptr<Client> weakClient = getWeakPtrByNickname(nickname);
	//size_t removed_count = _ClientModes.erase(weakClient);
	size_t removed_count = _ClientModes.erase(getWeakPtrByNickname(nickname));
    if (removed_count > 0) {
        // Also remove from operators if they were an operator
        //operators.erase(Client);
        std::cout << nickname << " left channel " << _name << std::endl;
	}
    return removed_count > 0;
}

/*bool Channel::isClientInChannel(Client* client) const {
    return client.count(client) > 0;
}

const std::set<Client*>& Channel::getClients() const {
    return Clients;
}

bool Channel::addOperator(Client* Client) {
    if (isClientInChannel(Client)) {
        auto result = operators.insert(Client);
        if (result.second) {
            if (Client) std::cout << Client->getNickname() << " is now an operator in " << _name << std::endl;
        }
        return result.second;
    }
    return false; // Client must be in the channel to be an operator
}

bool Channel::removeOperator(Client* Client) {
     size_t removed_count = operators.erase(Client);
     if (removed_count > 0) {
        if (Client) std::cout << Client->getNickname() << " is no longer an operator in " << _name << std::endl;
     }
     return removed_count > 0;
}

bool Channel::isOperator(Client* Client) const {
    return operators.count(Client) > 0;
}

void Channel::broadcastMessage(const std::string& message, Client* sender) const {
    std::cout << "Channel [" << _name << "] Broadcast: " << message << std::endl;
    // In a real server:
    // for (Client* Client : Clients) {
    //     if (Client && Client != sender) { // Added null check for Client pointer
    //         // Send message to Client->socket
    //         // Example: send(Client->socket_fd, message.c_str(), message.length(), 0);
    //     }
    // }
}

// Set mode definition (basic example)
void Channel::setMode(const std::string& mode, Client* Client) {
    // Added null check for Client pointer before accessing nickname
    if (Client) {
      std::cout << _name << " mode " << mode << " set by " << Client->getNickname() << "." << std::endl;
    } else {
       std::cout << _name << " mode " << mode << " set." << std::endl;
    }
    // Add actual mode logic here (e.g., if mode == "+t", set a flag)
}

// Remove mode definition (basic example)
void Channel::removeMode(const std::string& mode, Client* Client) {
    // Added null check for Client pointer before accessing nickname
    if (Client) {
      std::cout << _name << " mode " << mode << " removed by " << Client->getNickname() << "." << std::endl;
    } else {
      std::cout << _name << " mode " << mode << " removed." << std::endl;
    }
    // Add actual mode logic here
}


 * @brief void processClients() {
    for (auto it = _ClientModes.begin(); it != _ClientModes.end(); ) {
        if (auto clientPtr = it->first.lock()) {
            //client is still valid, do something
        } else {
            it = _ClientModes.erase(it);
        }
    }
}
Would this struc
 *
 */