#include "Channel.hpp"
#include <iostream>
#include <bitset>
#include <config.h>
#include "IrcResources.hpp"

#include <algorithm>
#include <cctype>

Channel::Channel(const std::string& channelName)  : _name(channelName), _topic("not set"){
	std::cout << "Channel '" << _name << "' created." << std::endl;
	_ChannelModes.reset();  //set all modes to off (0)
    _ChannelModes.set(Modes::ChannelMode::TOPIC);  // enable topic protection by default, why to show we can
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
		if (auto clientPtr = entry.first.lock()) {  // Convert weak_ptr to shared_ptr safely, anny expired pointers will be ignored , oohlalal
			list += clientPtr->getNickname() + "!" + clientPtr->getNickname() + "@localhost ";
		}
	}
	return list;
}

/*const std::string Channel::getAllNicknames() {
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
}*/


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

std::bitset<config::CLIENT_NUM_MODES>& Channel::getClientModes(const std::string nickname)
{
	std::cout << "Total Clients: " << _ClientModes.size() << std::endl;
	for (auto& entry : _ClientModes) {
		std::cout<<"whats thje name we looking at now = ["<<entry.first.lock()->getNickname()<<"]\n";
		if (auto clientPtr = entry.first.lock(); clientPtr && clientPtr->getNickname() == nickname) {

            return entry.second.first;  // return the matching weak_ptr
        }
    }
	// we could substitute with our own  throw here
	throw std::runtime_error("Client not found get client modes!");
	//return ;  // return empty weak_ptr if no match is found
}
// will add this back in a little later once my code is cleaner, incase debugging required
/*std::bitset<config::CLIENT_NUM_MODES> Channel::getClientModes(const std::string nickname) const {
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
}*/

// hold on to this, i dont think we need it, worth keeping incase of subtle issue
/*void Channel::buildWhoReplyFor(std::shared_ptr<Client> requestingClient) {
    if (!requestingClient)
        return;

    const std::string& channelName = this->getName();
    const std::string& targetNick = requestingClient->getNickname();

    for (std::map<std::weak_ptr<Client>, std::pair<std::bitset<config::CLIENT_NUM_MODES>, int>, WeakPtrCompare>::const_iterator it = _ClientModes.begin(); it != _ClientModes.end(); ++it) {
        std::shared_ptr<Client> member = it->first.lock();
        if (!member)
            continue;

        const std::bitset<config::CLIENT_NUM_MODES>& clientModes = it->second.first;

        std::string nickname   = member->getNickname();
        std::string username   = member->getClientUname();
        std::string host       = "localhost";//member->getHost(); // or "localhost"
        std::string realname   = member->getfullName();
        std::string status     = "H"; // Assume the user is "here"

        // Append "@" for operator if applicable
        if (clientModes[Modes::OPERATOR])
            status += "@";

        // Build the 352 WHO reply line
        std::string whoLine = ":localhost 352 " + targetNick + " " + channelName + " " +
                              username + " " + host + " localhost " + nickname + " " +
                              status + " :0 " + realname + "\r\n";

        requestingClient->getMsg().queueMessage(whoLine);
    }

    // Final 315: End of /WHO list
    std::string endLine = ":localhost 315 " + targetNick + " " + channelName + " :End of /WHO list\r\n";
    requestingClient->getMsg().queueMessage(endLine);
}*/


std::string Channel::getCurrentModes() const {

	std::string activeModes = "+";
    for (size_t i = 0; i < Modes::channelModeChars.size(); ++i) {
        if (_ChannelModes.test(i)) {
            activeModes += Modes::channelModeChars[i];
        }
    }
    return activeModes;
}



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


std::vector<std::string> Channel::applymodes(std::vector<std::string> params)
{
	std::string modes;
	std::string targets;

	size_t paramIndex = 1;
	size_t modeIndex = 0;
	std::string modeFlags; 
	char modeChar;
	char sign;
	std::vector<std::string> messageData;
	std::cout<<"starting to change the mode----param at 0"<<params[paramIndex]<<"\n";
	while (paramIndex < params.size())
	{
		if (modeIndex == 0){
				sign = params[paramIndex][modeIndex]; // we take the + or -
				modeFlags = params[paramIndex].substr(1);
				modes += sign; 
			}
		while (modeIndex < modeFlags.size())
		{
			std::cout<<"steping into the damn setmodechannel checkl moder flags "<<modeFlags<<"---\n";
			
			modeChar = modeFlags[modeIndex];//params[paramIndex][modeIndex];
			if (channelModeRequiresParameter(modeChar))
			{
				paramIndex++;
			}
			std::cout<<"steping into the damn setmodechannel checkl moder char "<<modeChar<<"---\n";

			messageData = setChannelMode(modeChar , setModeBool(sign), params[paramIndex]);
			//std::cout<<"whats in message data "<<messageData[0]<<"----\n";
			
			if (!messageData.empty())
			{
				modes += messageData[0] + " ";
				if (messageData.size() > 1)
					targets += messageData[1] + " ";
				messageData.clear();
			}
			modeIndex++; 
			std::cout<<"innerloop ----\n";

		}
		std::cout<<"outerloop----\n";

		paramIndex++;

	}
	std::cout<<"ending to change the mode----\n";
	if (modes.size() > 1) // not only sign but also mode flag
	{
		messageData.push_back(modes); 
		messageData.push_back(targets); 
	}
	return messageData;
}

std::vector<std::string> Channel::setChannelMode(char modeChar , bool enableMode, const std::string& target) {
	std::cout<<"SET CHANNEL MODE ACTIVATED -------------------------mode char = "<<modeChar<<"\n";

	std::vector<std::string> response;
	bool modeChange = false;
	Modes::ChannelMode cmodeType = Modes::NONE;
	Modes::ClientMode modeType = Modes::CLIENT_NONE;
    if (isValidChannelMode(modeChar)){
		cmodeType = charToChannelMode(modeChar);
		std::cout<<"is mode not changing here "<< static_cast<int>(cmodeType) << std::endl;
	}
    if (isValidClientMode(modeChar))
		modeType = charToClientMode(modeChar);
    
	if (cmodeType == Modes::NONE && modeType == Modes::CLIENT_NONE) {
        return {};
    }
	if(_ChannelModes.test(cmodeType) == true )
		std::cout<<"mode already set----\n";
	else
		std::cout<<"mode is false----\n";

	if (cmodeType!= Modes::NONE) {
		if(_ChannelModes.test(cmodeType) != enableMode) {
			_ChannelModes.set(cmodeType, enableMode);
			std::cout<<"mode changed in apply mode----\n";
			modeChange = true;
		}
	}
	//bool shouldReport = !(modeChange && !enableMode) && (cmodeType != Modes::NONE); // || modeType != Modes::CLIENT_NONE);
	bool shouldReport = modeChange || enableMode;
	if (shouldReport) {
			std::cout<<"should report activated ----\n";

		response.push_back(std::string(1, modeChar)); // Mode char
	}
 	switch (cmodeType) {
            case Modes::USER_LIMIT:
                if (enableMode) {
					_ulimit = std::stoul(target);
				} else {
					_ulimit = 0;
				} if (shouldReport) { // shouldReport check ensures we only add if we're actually reporting
                    response.push_back(target);
				}
				break;
            
            case Modes::PASSWORD:
                if (enableMode)
				{

					_password = target;
				}
				else
				{
					_password.clear();
				}
				if (shouldReport) { // shouldReport check ensures we only add if we're actually reporting
                    response.push_back(target);
                }
				break;

            case Modes::TOPIC:
				break;

            case Modes::INVITE_ONLY:
				break;

            default:
                break;
    }
	 if (modeType != Modes::CLIENT_NONE) {
		if(getClientModes(target).test(modeType) != enableMode)
		{
			getClientModes(target).set(modeType, enableMode);
		}
		response.push_back(std::string(1, modeChar)); // Mode char
        response.push_back(target);  // The user being affected
    }

    return response;
}
	/*if (currentClientName != "" && getClientModes(currentClientName).test(Modes::OPERATOR) == false)
	{
		return {MsgType::NOT_OPERATOR, {nickname, channelname}};
	}
	char modechar = mode[1];
	bool onoff = setModeBool(mode[0]);
	MsgType ret;
	Modes::ChannelMode cmodeType = charToChannelMode(modechar);
	if (cmodeType != Modes::NONE) {
		if (cmodeType == Modes::PASSWORD) {
			if (pass.empty())
				return {MsgType::INVALID_PASSWORD, {}};
				// do we want a character limit here? we could just have it at 8 for simplicities sake
			this->_password = pass;
			ret = MsgType::PASSWORD_APPLIED;
				  //  -!- mode/#bbq [+k hell] by user channel broadcast
		}
		if (cmodeType != Modes::INVITE_ONLY && (nickname.empty() || nickname == ""))
		{
			return {MsgType::INVALID_INVITE, {}};
		}
		_ChannelModes.set(cmodeType, onoff);
		return {ret, {}};
	}
	return {MsgType::UNKNOWN, {}};*/
//}


// this confilct is maybe unresolved 
    // This part should also be case-insensitive for currentClientName lookup, why?
    /*if (currentClientName != "") {
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
}*/


bool Channel::canClientJoin(const std::string& nickname, const std::string& password ) {
    std::cout<<"handling can client join to client named = "<<nickname<<"\n";

    //std::string lower_nickname_param = nickname;
    //std::transform(lower_nickname_param.begin(), lower_nickname_param.end(), lower_nickname_param.begin(),
    //               [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

    if(_ChannelModes.test(Modes::INVITE_ONLY)) {
        // Check if the lowercase nickname is in the invites list
        auto it = std::find(_invites.begin(), _invites.end(), nickname);//, lower_nickname_param);
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
    }
    if (nickname found in ban list ) // This would also need case-insensitivity
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
			setChannelMode('o', setModeBool('+'), client->getNickname());
			//setClientMode("+q", client->getNickname(), "");
			// if we add any other here we must remeber to set to 0 or 1
			client->setChannelCreator(false);
		}
		else 
		{
			setChannelMode('o', setModeBool('-'), client->getNickname());

			// make sure these modes are set to 0
			//setClientMode("-o", client->getNickname(), "");
			//setClientMode("-q", client->getNickname(), "");
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


std::pair<MsgType, std::vector<std::string>> Channel::initialModeValidation(
        const std::string& ClientNickname,
        size_t paramsSize)  {
        // The target (params[0]) is the channel name itself, which is implicitly 'this->getName()'.
        if (paramsSize == 1) {
            // Client must be in the channel to list its modes.
            if (!isClientInChannel(ClientNickname)) {
                return {MsgType::NOT_IN_CHANNEL, {ClientNickname, getName()}};
            }
            // Return RPL_CHANNELMODEIS with necessary parameters for MessageBuilder
            return {MsgType::RPL_CHANNELMODEIS, {ClientNickname, getName(), getCurrentModes(), ""}};
        }
        if (!isClientInChannel(ClientNickname)) {
            return {MsgType::NOT_IN_CHANNEL, {ClientNickname, getName()}};
        }

        // Client must be an operator in the channel to set its modes.
        if (!getClientModes(ClientNickname).test(Modes::OPERATOR)) {
            return {MsgType::NOT_OPERATOR, {ClientNickname, getName()}};
        }
        // If all channel-specific initial checks pass, return success.
        return {MsgType::NONE, {}};
}
/**
 * @brief +o-o user1 user2 is not standard protocol, you can not make me try to imitate libera chat at this 
 * stage of my education at this pace . 
 * 
 * @param requestingClientNickname 
 * @param params 
 * @return std::pair<MsgType, std::vector<std::string>> 
 */
std::pair<MsgType, std::vector<std::string>> Channel::modeSyntaxValidator(
        const std::string& requestingClientNickname,
        const std::vector<std::string>& params) const {

        size_t currentIndex = 1;

        char currentSign = ' '; // current mode operation ('+' or '-')

        // Loop through all tokens in `params` starting from the first mode/parameter token.
        while (currentIndex < params.size()) {
            const std::string& currentToken = params[currentIndex];

            // --- 1. Determine if currentToken is a mode group or a parameter ---
            bool isModeGroupToken = (currentToken.length() > 0 && (currentToken[0] == '+' || currentToken[0] == '-'));
			// this will be false on first run if not flags
            if (isModeGroupToken) {
                // If it's a mode group token, set the sign and iterate through its characters
                currentSign = currentToken[0];

                // Iterate through mode characters within this token (skip the sign)
                for (size_t i = 1; i < currentToken.length(); ++i) {
                    char modeChar = currentToken[i];

                    // --- 1a. Validate modeChar is known for this channel ---
                    bool modeCharValid = isValidChannelMode(modeChar) || isValidClientMode(modeChar);

                    if (!modeCharValid) {
                        std::cout << "DEBUG: Syntax Error: Unknown mode char '" << modeChar << "' for channel " << getName() << "." << std::endl;
                        // Parameters for UNKNOWN_MODE: {modeChar, clientNickname, channelName}
                        return {MsgType::UNKNOWN_MODE, {std::string(1, modeChar), requestingClientNickname, getName()}};
                    }

                    // --- 1b. Check if modeChar requires a parameter and validate parameter existence ---
                    bool paramExpected = channelModeRequiresParameter(modeChar); // Using a Channel method

                    if (paramExpected) {
                        // Check for missing parameter *before* trying to access it
                        if (currentIndex + 1 >= params.size()) {
                            std::cout << "DEBUG: Syntax Error: Mode '" << modeChar << "' requires a parameter but none found." << std::endl;
                            // Parameters for NEED_MORE_PARAMS: {clientNickname, "MODE", modeChar}
                            return {MsgType::NEED_MORE_PARAMS, {requestingClientNickname, "MODE", std::string(1, modeChar)}};
                        }

                        // If parameter exists, get it for content validation
                        const std::string& modeParam = params[currentIndex + 1];

                        // --- 1c. Validate parameter content specific to channel modes ---
                        if (modeChar == 'o' ) { // || modeChar == 'v'Channel client modes (+o, +v)
                            // The user specified: "we dont care if it exists merly if they exist in the channel"
                            if (!isClientInChannel(modeParam)) { // Check if the target client for +o/+v is in *this* channel
                                std::cout << "DEBUG: Syntax Error: User '" << modeParam << "' not in channel for mode '" << modeChar << "'." << std::endl;
                                // Parameters for NO_SUCH_NICK: {clientNickname, modeParam, channelName}
                                return {MsgType::NO_SUCH_NICK, {requestingClientNickname, modeParam, getName()}};
                            }
                            // Also, check if username starts with lowercase for IRC standard compliance
                            if (modeParam.empty()) { 
                                std::cout << "DEBUG: Syntax Error: Invalid nickname format '" << modeParam << "' for mode '" << modeChar << "'." << std::endl;
                                return {MsgType::NO_SUCH_NICK, {requestingClientNickname, modeParam, getName()}};
                            }
                        } else if (modeChar == 'l' && currentSign == '+') { // Set user limit (+l)
                            try {
                                if (modeParam.empty()) throw std::invalid_argument("empty");
                                
								try
								{
									 long limit = std::stoul(modeParam);
									 if (limit > 100)
									 	std::cout<<"something is not right limit too big boys \n";
									
								}
								catch(const std::exception& e)
								{
									std::cerr << e.what() << '\n';
								}
                                // Add a limit max check here, e.g., if (limit > MAX_USERS_ALLOWED)
                                // if (limit > SOME_MAX_CHANNEL_LIMIT) {
                                //     return {MsgType::INVALID_MODEPARAM, {requestingClientNickname, std::string(1, modeChar), "limit too high"}};
                                // }
                            } catch (const std::exception& e) {
                                std::cout << "DEBUG: Syntax Error: Invalid numeric parameter '" << modeParam << "' for mode '" << modeChar << "'." << std::endl;
                                // Custom message for invalid limit, using NEED_MORE_PARAMS for structure
                                return {MsgType::NEED_MORE_PARAMS, {requestingClientNickname, "MODE", std::string(1, modeChar), "Invalid limit (not a number or too high)"}};
                            }
                        } else if (modeChar == 'k' && currentSign == '+') { // Set channel key/password (+k)
                            if (modeParam.empty()) {
                                std::cout << "DEBUG: Syntax Error: Empty password parameter for mode '" << modeChar << "'." << std::endl;
                                // Custom message for empty password, using NEED_MORE_PARAMS for structure
                                return {MsgType::NEED_MORE_PARAMS, {requestingClientNickname, "MODE", std::string(1, modeChar), "Empty password"}};
                            }
                            // Add password restrictions (e.g., length, allowed characters) here
                            // if (!isValidPassword(modeParam)) { ... return error ... }
                        }

                        // If a parameter was expected and consumed, advance currentIndex past it.
                        currentIndex++;
                    }
                } // End inner loop over characters in currentToken
                
                // After processing all characters in this mode group token,
                // advance currentIndex past the mode group token itself.
                currentIndex++;

            } else { // Current token is NOT a mode group (doesn't start with '+' or '-')
                // This means it's an unexpected parameter or malformed command.
                std::cout << "DEBUG: Syntax Error: Unexpected token '" << currentToken << "' for channel " << getName() << ". Expected mode group or end of command." << std::endl;
                // Using NEED_MORE_PARAMS for general structural errors, or UNKNOWN_COMMAND if more specific.
                return {MsgType::NEED_MORE_PARAMS, {requestingClientNickname, "MODE", currentToken}};
            }
        } // End while loop over params

        // If the loop completes without any errors, the syntax is valid.
        return {MsgType::NONE, {}};
    }


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
            /*else if (modes.test(Modes::VOICE)) { // Assuming Modes::VOICE is defined in config.h
                prefix = "+";
            }*/
            return prefix;
        }
    }
    return ""; // Client not found in this channel
}
