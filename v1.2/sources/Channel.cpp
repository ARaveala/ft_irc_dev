#include "Channel.hpp"
#include <iostream>
#include <bitset>
#include "IrcResources.hpp"
#include <algorithm>
#include <cctype>

/*Channel::Channel(const std::string& channelName, std::map<int, std::shared_ptr<Client>>& clients) : _name(channelName), _topic(""), _clients(clients) {
    std::cout << "Channel '" << _name << "' created." << std::endl;
}*/

Channel::Channel(const std::string& channelName)  : _name(channelName), _topic("not set"){
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

// rename to refer t getting a user list for channel gets all nickname sinto a lits, rename
// const std::string Channel::getAllNicknames() {
// 	std::string list;
// 	for (const auto& entry : _ClientModes) {
// 		if (auto clientPtr = entry.first.lock()) {  // Convert weak_ptr to shared_ptr safely, anny expired pointers will be ignored , oohlalal
// 			list += clientPtr->getNickname() + "!" + clientPtr->getNickname() + "@localhost ";
// 		}
// 	}
// 	return list;
// }

const std::string Channel::getAllNicknames() {
    std::string list;
    for (const auto& entry : _ClientModes) {
        if (auto clientPtr = entry.first.lock()) {
            // Use getClientModePrefix here!
            list += getClientModePrefix(clientPtr) + clientPtr->getNickname() + " ";
            // You might also need the full hostmask like "!" + clientPtr->getUsername() + "@" + clientPtr->getHostname()
            // depending on the exact IRC reply format you're building, but for a simple list, just the prefix+nickname is common.
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

//make this fucntion clean up any wekptr that no longer refrences
std::weak_ptr<Client> Channel::getWeakPtrByNickname(const std::string& nickname) {
    std::string lower_nickname_param = nickname;
    std::transform(lower_nickname_param.begin(), lower_nickname_param.end(), lower_nickname_param.begin(),
                   [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

    for (const auto& entry : _ClientModes) {
        if (auto clientPtr = entry.first.lock()) {
            std::string stored_lower_nickname = clientPtr->getNickname();
            std::transform(stored_lower_nickname.begin(), stored_lower_nickname.end(), stored_lower_nickname.begin(),
                           [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

            if (stored_lower_nickname == lower_nickname_param) {
                return entry.first; // return the matching weak_ptr
            }
        }
    }
    return {}; // return empty weak_ptr if no match is found
}

std::bitset<config::CLIENT_NUM_MODES> Channel::getClientModes(const std::string nickname) const {
    std::string lower_nickname_param = nickname;
    std::transform(lower_nickname_param.begin(), lower_nickname_param.end(), lower_nickname_param.begin(),
                   [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

    for (const auto& entry : _ClientModes) { // Note: const auto& is fine here
        if (auto clientPtr = entry.first.lock()) {
            std::string stored_lower_nickname = clientPtr->getNickname();
            std::transform(stored_lower_nickname.begin(), stored_lower_nickname.end(), stored_lower_nickname.begin(),
                           [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

            if (stored_lower_nickname == lower_nickname_param) {
                // This will now return a copy of the bitset, which is correct
                return entry.second.first;
            }
        }
    }
    // This will return a new, default-constructed bitset by value, which is correct
    return std::bitset<config::CLIENT_NUM_MODES>();
}

/*std::bitset<config::CHANNEL_NUM_MODES>& Channel::getChannelModes()
{
	//std::cout << "Total Clients: " << _ClientModes.size() << std::endl;
	for (auto& entry : _ClientModes) {
		std::cout<<"whats thje name we looking at now = ["<<entry.first.lock()->getNickname()<<"]\n";
		if (auto clientPtr = entry.first.lock(); clientPtr && clientPtr->getNickname() == nickname) {

            return entry.second.first;  // return the matching weak_ptr
        }
    }
	// we could substitute with our own  throw here
	throw std::runtime_error("Client not found get client modes!");
	//return ;  // return empty weak_ptr if no match is found
}*/

void Channel::setTopic(const std::string& newTopic) {
    _topic = newTopic;
}

Modes::ClientMode Channel::charToClientMode(const char& modeChar) {
	switch (modeChar) {
		case 'o': return Modes::OPERATOR;
		case 'q': return Modes::FOUNDER;
		//case 'i': return Modes::INVITE_ONLY;
		default : return Modes::CLIENT_NONE;
	}

}
Modes::ChannelMode Channel::charToChannelMode(const char& modeChar) {
	switch (modeChar) {
		case 'i': return Modes::INVITE_ONLY;
		case 'k': return Modes::PASSWORD;
		case 'l': return Modes::USER_LIMIT;
		case 't': return Modes::TOPIC;
		default : return Modes::NONE;
	}

}
bool Channel::setModeBool(char onoff) {
	return onoff == '+'; 
}
//16:31 -!- mode/#bbq [-oo pleb1 pleb2] by anonikins
/*void Channel::setChannelMode(std::string mode)
{
//	char operation = mode[0];
	char modechar = mode[1];
	bool onoff = setModeBool(mode[0]);
	//if (operation == '+')
	//	onoff = true;

	Modes::ChannelMode modeType = charToChannelMode(modechar);
	if (modeType != Modes::NONE) {
		_ChannelModes.set(modeType, onoff);
	}

}*/

int Channel::setChannelMode(std::string mode, std::string nickname, std::string currentClientName, std::string channelname, const std::string pass, std::map<std::string, int>& listOfClients,  std::function<void(MsgType, std::vector<std::string>&)> setMsgType) {
    // Case-insensitivity for currentClientName check
    if (currentClientName != "") {
        std::string lower_current_client_name = currentClientName;
        std::transform(lower_current_client_name.begin(), lower_current_client_name.end(), lower_current_client_name.begin(),
                       [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

        // Find the actual client shared_ptr for currentClientName
        std::shared_ptr<Client> currentClientPtr;
        for (const auto& entry : _ClientModes) {
            if (auto clientPtr = entry.first.lock()) {
                std::string stored_lower_nickname = clientPtr->getNickname();
                std::transform(stored_lower_nickname.begin(), stored_lower_nickname.end(), stored_lower_nickname.begin(),
                               [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });
                if (stored_lower_nickname == lower_current_client_name) {
                    currentClientPtr = clientPtr;
                    break;
                }
            }
        }

        // Now check if currentClientPtr is an operator
        if (!currentClientPtr || !getClientModes(currentClientPtr->getNickname()).test(Modes::OPERATOR)) {
            std::vector<std::string> params = {nickname, channelname}; // `nickname` here is the target of the mode, not the current client
            setMsgType(MsgType::NOT_OPERATOR, params);
            return 2;
        }
    }

    char modechar = mode[1];
    bool onoff = setModeBool(mode[0]);
    Modes::ChannelMode cmodeType = charToChannelMode(modechar);

    if (cmodeType != Modes::NONE) {
        // For password mode (+k/-k)
        if (cmodeType == Modes::PASSWORD) { // Changed `!=` to `==` for clarity
            if (onoff) { // Setting password
                if (pass.empty()) {
                    // ERROR: Password not provided for +k
                    // setMsgType(MsgType::NEED_MORE_PARAMS_K, {channelname}); // Example error message
                    return 0; // Or appropriate error code
                }
                this->_password = pass;
            } else { // Unsetting password
                this->_password.clear(); // Clear the password
            }
        }
        // For invite-only mode (+i/-i), and potentially adding/removing invites
// For invite-only mode (+i/-i), and potentially adding/removing invites
else if (cmodeType == Modes::INVITE_ONLY) {
    _ChannelModes.set(cmodeType, onoff); // Set or unset the invite-only flag for the channel

    // If setting +i (invite-only)
    if (onoff) {
        // If a nickname is provided (meaning an invite is being issued to a specific user)
        if (!nickname.empty()) {
            std::string lower_invite_nickname = nickname;
            std::transform(lower_invite_nickname.begin(), lower_invite_nickname.end(), lower_invite_nickname.begin(),
                            [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

            // IMPORTANT: Check if the invited nickname actually exists on the server
            // This uses the 'listOfClients' parameter to verify against all connected clients.
            // **TO BE TESTED**: Ensure listOfClients is correctly populated with all connected client nicknames (lowercase)
            // and their FDs from the Server class.
            auto it_client_exists = listOfClients.find(lower_invite_nickname);

            if (it_client_exists != listOfClients.end()) { // Client exists on the server
                // Check if nickname is already in invites before adding
                auto it_invite = std::find(_invites.begin(), _invites.end(), lower_invite_nickname);
                if (it_invite == _invites.end()) {
                    _invites.push_back(lower_invite_nickname); // Add to invite list

                    // **TO BE TESTED**: Send RPL_INVITING (341) to the currentClientName (the inviter)
                    // and potentially a NOTICE to the invited user.
                    // Example: ":<server> 341 <inviter_nick> <invited_nick> <channel_name>"
                    // You'll need to use setMsgType for this.
                }
            } else { // Client does NOT exist on the server
                // **TO BE TESTED**: Send ERR_NOSUCHNICK (401) to the currentClientName (the inviter)
                // Example: ":<server> 401 <inviter_nick> <non_existent_nick> :No such nick/channel"
                // You'll need to use setMsgType for this.
                std::vector<std::string> params;
                params.push_back(currentClientName); // Inviter
                params.push_back(nickname); // The non-existent nick
				setMsgType(static_cast<MsgType>(IRCerr::ERR_NOSUCHNICK), params);            }
        }
    }
    // If unsetting -i (invite-only), and a nickname is provided, remove from invite list
    else if (!onoff && !nickname.empty()) {
        std::string lower_remove_nickname = nickname;
        std::transform(lower_remove_nickname.begin(), lower_remove_nickname.end(), lower_remove_nickname.begin(), // Changed end() to begin() for consistency
                       [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });
        _invites.erase(std::remove(_invites.begin(), _invites.end(), lower_remove_nickname), _invites.end());
        // **TO BE TESTED**: Potentially send a message indicating the invite was removed.
    }
}
        // For other channel modes (like +l, +t)
        else {
             _ChannelModes.set(cmodeType, onoff);
        }
    }
    return 0; // Success
}

int Channel::setClientMode(std::string mode, std::string nickname, std::string currentClientName) {

    // This part should also be case-insensitive for currentClientName lookup
    if (currentClientName != "") {
        std::string lower_current_client_name = currentClientName;
        std::transform(lower_current_client_name.begin(), lower_current_client_name.end(), lower_current_client_name.begin(),
                       [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });
        
        // Find the actual client shared_ptr for currentClientName
        std::shared_ptr<Client> currentClientPtr;
        for (const auto& entry : _ClientModes) {
            if (auto clientPtr = entry.first.lock()) {
                std::string stored_lower_nickname = clientPtr->getNickname();
                std::transform(stored_lower_nickname.begin(), stored_lower_nickname.end(), stored_lower_nickname.begin(),
                               [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });
                if (stored_lower_nickname == lower_current_client_name) {
                    currentClientPtr = clientPtr;
                    break;
                }
            }
        }

        // Now check if currentClientPtr is an operator
        if (!currentClientPtr || !getClientModes(currentClientPtr->getNickname()).test(Modes::OPERATOR)) {
            // build message here ?
            // Need a way to send error message back to currentClientName
            std::cerr << "Error: " << currentClientName << " is not an operator on channel " << _name << std::endl;
            return 2; // Indicate permission denied
        }
    }


    char modechar = mode[1];
    bool onoff = setModeBool(mode[0]);
    Modes::ClientMode modeType = charToClientMode(modechar);

    if (modeType != Modes::CLIENT_NONE) {
        // Apply case-insensitivity for the target 'nickname'
        std::string lower_target_nickname = nickname;
        std::transform(lower_target_nickname.begin(), lower_target_nickname.end(), lower_target_nickname.begin(),
                       [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

        // Find the target client's weak_ptr in _ClientModes
        std::weak_ptr<Client> targetWeakClientPtr;
        for (auto& entry : _ClientModes) { // Need non-const auto& here to modify the bitset
            if (auto clientPtr = entry.first.lock()) {
                std::string stored_lower_nickname = clientPtr->getNickname();
                std::transform(stored_lower_nickname.begin(), stored_lower_nickname.end(), stored_lower_nickname.begin(),
                               [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });
                if (stored_lower_nickname == lower_target_nickname) {
                    targetWeakClientPtr = entry.first;
                    break;
                }
            }
        }

        if (auto targetClient = targetWeakClientPtr.lock()) {
             // Now that we have the actual client's bitset, set the mode
            // This is how you access and modify the bitset directly from the map:
            _ClientModes[targetWeakClientPtr].first.set(modeType, onoff);
            return 1; // Success
        } else {
            // Target client not found in channel (or weak_ptr expired)
            std::cerr << "Error: Target client " << nickname << " not found in channel " << _name << std::endl;
            // You might want to send an error message like ERR_NOSUCHNICK back to the currentClientName
            return 0; // Failure
        }
    }
    return 0; // Invalid mode
}

bool Channel::canClientJoin(const std::string& nickname, const std::string& password ) {
    std::cout<<"handling can client join to client named = "<<nickname<<"\n";

    std::string lower_nickname_param = nickname;
    std::transform(lower_nickname_param.begin(), lower_nickname_param.end(), lower_nickname_param.begin(),
                   [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

    if(_ChannelModes.test(Modes::INVITE_ONLY)) {
        // Check if the lowercase nickname is in the invites list
        auto it = std::find(_invites.begin(), _invites.end(), lower_nickname_param);
        if (it != _invites.end()) {
            _invites.erase(it); // Remove name from invite list after successful join
            return true;
        }
        // setmsg invite only (You'll need to pass a way to send messages back to the client here)
        return false;
    }
    
    if (_ChannelModes.test(Modes::PASSWORD)) {
        if (password == "") {
            // set msg no password provided
            return false;
        }
        if (password != _password) {
            //set msg password mismatch
            return false;
        }
    }

    /*if (_clientCount >= _ClientLimit)
    {
        // setmsg client limit reached
        return false;
    }*/
    /*if (nickname found in ban list ) // This would also need case-insensitivity
    {
        //setmsg banned
        return false;
    }*/
    return true;
}

bool Channel::addClient(std::shared_ptr <Client> client) {
    //std::set::insert returns a pair: iterator to the element and a boolean indicating insertion
	if (!client)
		return false; // no poopoo pointers
	std::weak_ptr<Client> weakclient = client;
	//_ClientModes.emplace(weakclient, std::make_pair(std::bitset<4>(), client->getFd()));
	//_ClientModes[weakclient].first.set(MODE_OPERATOR);

	if (_ClientModes.find(weakclient) == _ClientModes.end()) {
		_ClientModes.insert({weakclient, std::make_pair(std::bitset<config::CLIENT_NUM_MODES>(), client->getFd())});  //Insert new client
		if (client->getChannelCreator() == true)
		{
			setClientMode("+o", client->getNickname(), "");
			setClientMode("+q", client->getNickname(), "");
			client->setChannelCreator(false);
		}
		
		_clientCount += 1;
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
    // We already use getWeakPtrByNickname which handles case-insensitivity
    size_t removed_count = _ClientModes.erase(getWeakPtrByNickname(nickname));
    if (removed_count > 0) {
        // Also remove from operators if they were an operator
        //operators.erase(Client); // This comment refers to an old structure

        std::cout << nickname << " left channel " << _name << std::endl;
        _clientCount -= 1; // Decrement client count
    }
    return removed_count > 0;
}

// bool Channel::isClientInChannel(Client* client) const {
//     return client.count(client) > 0;
// }

bool Channel::isClientInChannel(const std::string& nickname) const {
    std::string lower_nickname_param = nickname;
    std::transform(lower_nickname_param.begin(), lower_nickname_param.end(), lower_nickname_param.begin(),
                   [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

    for (const auto& entry : _ClientModes) {
        if (auto clientPtr = entry.first.lock()) {
            std::string stored_lower_nickname = clientPtr->getNickname();
            std::transform(stored_lower_nickname.begin(), stored_lower_nickname.end(), stored_lower_nickname.begin(),
                           [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

            if (stored_lower_nickname == lower_nickname_param) {
                return true;
            }
        }
    }
    return false;
}


// const std::set<Client*>& Channel::getClients() const {
//     return Clients;
// }

// bool Channel::addOperator(Client* Client) {
//     if (isClientInChannel(Client)) {
//         auto result = operators.insert(Client);
//         if (result.second) {
//             if (Client) std::cout << Client->getNickname() << " is now an operator in " << _name << std::endl;
//         }
//         return result.second;
//     }
//     return false; // Client must be in the channel to be an operator
// }

// todo it's generally cleaner and safer to stick to std::shared_ptr<Client> consistently if you're using shared pointers for client management.
// bool Channel::removeOperator(Client* Client) {
// 	size_t removed_count = operators.erase(Client);
//      if (removed_count > 0) {
//         if (Client) std::cout << Client->getNickname() << " is no longer an operator in " << _name << std::endl;
//      }
//      return removed_count > 0;
// }

// todo it's generally cleaner and safer to stick to std::shared_ptr<Client> consistently if you're using shared pointers for client management.
// bool Channel::isOperator(Client* Client) const {
//     return operators.count(Client) > 0;
// }

// todo it's generally cleaner and safer to stick to std::shared_ptr<Client> consistently if you're using shared pointers for client management.
// void Channel::broadcastMessage(const std::string& message, Client* sender) const {
//     std::cout << "Channel [" << _name << "] Broadcast: " << message << std::endl;
//     // In a real server:
//     // for (Client* Client : Clients) {
//     //     if (Client && Client != sender) { // Added null check for Client pointer
//     //         // Send message to Client->socket
//     //         // Example: send(Client->socket_fd, message.c_str(), message.length(), 0);
//     //     }
//     // }
// }

// Set mode definition (basic example)
void Channel::setMode(const std::string& mode, std::shared_ptr<Client> client) { // Changed Client* to std::shared_ptr<Client>
    if (client) { // Now client is a shared_ptr, it will be non-nullptr if valid
        std::cout << _name << " mode " << mode << " set by " << client->getNickname() << "." << std::endl;
    } else {
        std::cout << _name << " mode " << mode << " set." << std::endl;
    }
    // Add actual mode logic here (e.g., if mode == "+t", set a flag)
}

// Remove mode definition (basic example)
// todo it's generally cleaner and safer to stick to std::shared_ptr<Client> consistently if you're using shared pointers for client management.
// void Channel::removeMode(const std::string& mode, Client* Client) {
//     // Added null check for Client pointer before accessing nickname
//     if (Client) {
//       std::cout << _name << " mode " << mode << " removed by " << Client->getNickname() << "." << std::endl;
//     } else {
//       std::cout << _name << " mode " << mode << " removed." << std::endl;
//     }
//     // Add actual mode logic here
// }

/*
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

 std::string Channel::getClientModePrefix(std::shared_ptr<Client> client) const {
    if (!client) {
        return ""; // Or handle as an error if a null shared_ptr is passed
    }

    // Iterate through _ClientModes to find the matching client
    for (const auto& entry : _ClientModes) {
        // Safely lock the weak_ptr and compare with the provided shared_ptr
        if (auto weakClientPtr = entry.first.lock(); weakClientPtr && weakClientPtr == client) {
            const std::bitset<config::CLIENT_NUM_MODES>& modes = entry.second.first;
            std::string prefix = "";

            // Check for operator mode ('@') first, as it takes precedence over voice ('+')
            if (modes.test(Modes::OPERATOR)) { // Assuming Modes::OPERATOR is defined in config.h
                prefix = "@";
            }
            // Else, check for voice mode ('+')
            else if (modes.test(Modes::VOICE)) { // Assuming Modes::VOICE is defined in config.h
                prefix = "+";
            }
            return prefix;
        }
    }
    return ""; // Client not found in this channel
}
