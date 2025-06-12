#include "Channel.hpp"
#include <iostream>
#include <bitset>
#include <config.h>
#include "IrcResources.hpp"
#include <string>
/*Channel::Channel(const std::string& channelName, std::map<int, std::shared_ptr<Client>>& clients) : _name(channelName), _topic(""), _clients(clients) {
    std::cout << "Channel '" << _name << "' created." << std::endl;
}*/

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

//make this fucntion clean up any wekptr that no longer refrences
std::weak_ptr<Client> Channel::getWeakPtrByNickname(const std::string& nickname) {
    for (const auto& entry : _ClientModes) {
        if (auto clientPtr = entry.first.lock(); clientPtr && clientPtr->getNickname() == nickname) {
            return entry.first;  // return the matching weak_ptr
        }
    }
	// we could substitute with a throw here
    return {};  // return empty weak_ptr if no match is found
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
std::string Channel::getCurrentModes() const {

	std::string activeModes = "+";
    for (size_t i = 0; i < Modes::channelModeChars.size(); ++i) {
        if (_ChannelModes.test(i)) {
            activeModes += Modes::channelModeChars[i];
        }
    }
    return activeModes;
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

/* // we will try this 1
	std::pair<bool, std::string> Channel::setChannelMode(char modeChar, bool enableMode, const std::string& modeParam) {
    // Note: The operator check should primarily happen in CommandDispatcher::initialModeValidation.
    // This function assumes the caller (CommandDispatcher) has already verified operator status.
    // If you keep it here as a safety net, ensure getClientModes and Modes::OPERATOR are accessible.


    Modes::ChannelMode cmodeType = Modes::charToChannelMode(modeChar);
    
    // The initialModeValidation should have caught UNKNOWN_MODE, so this might be redundant
    // but good for internal consistency if this function can be called directly.
    if (cmodeType == Modes::NONE) {
        // This case should ideally not be reached if pre-validation is robust.
        // If it is, consider logging an internal error.
        return {false, ""}; // Indicate failure, no mode fragment
    }

    // Build the mode fragment string
    std::string modeFragment = std::string(1, enableMode ? '+' : '-');
    modeFragment += modeChar;

    bool success = false;
    switch (cmodeType) {
        case Modes::USER_LIMIT: // +l <limit>
            if (enableMode) {
                if (modeParam.empty()) return {false, ""}; // Missing parameter for +l
                try {
                    unsigned int limit = std::stoul(modeParam); // Convert string to unsigned long
                    _userLimit = limit;
                    _ChannelModes.set(Modes::USER_LIMIT, true); // Ensure limit mode is ON
                    modeFragment += " " + modeParam; // Add parameter to fragment
                    success = true;
                } catch (const std::exception& e) {
                    // Invalid number format for limit
                    return {false, ""};
                }
            } else { // -l (remove limit)
                _userLimit = 0; // Or some default
                _ChannelModes.set(Modes::USER_LIMIT, false);
                success = true;
            }
            break;

        case Modes::INVITE_ONLY: // +i
            _ChannelModes.set(Modes::INVITE_ONLY, enableMode);
            success = true;
            break;

        case Modes::PASSWORD: // +k <password>
            if (enableMode) {
                if (modeParam.empty()) return {false, ""}; // Missing parameter for +k
                _password = modeParam;
                _ChannelModes.set(Modes::PASSWORD, true); // Ensure password mode is ON
                modeFragment += " " + modeParam; // Add parameter to fragment
                success = true;
            } else { // -k (remove password)
                _password.clear(); // Clear the password
                _ChannelModes.set(Modes::PASSWORD, false);
                success = true;
            }
            break;

        case Modes::TOPIC_SET_BY_OP: // +t
            _ChannelModes.set(Modes::TOPIC_SET_BY_OP, enableMode);
            success = true;
            break;
			    return {success, modeFragment};

 */
std::vector<std::string> Channel::applymodes(std::vector<std::string> params)
{
	std::string modes;
	std::string targets;

	size_t paramIndex = 1;
	size_t modeIndex = 0;
	//int targetIndex = 0;
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
				//modeIndex++; 
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


  //  char modeChar = mode[1];
    //bool enableMode = setModeBool(mode[0]);
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
		//response.push_back(target);
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

/*int Channel::setChannelMode(std::string mode, std::string nickname, std::string currentClientName, std::string channelname, const std::string pass, std::map<std::string, int>& listOfClients,  std::function<void(MsgType, std::vector<std::string>&)> setMsgType) {
	if (currentClientName != "" && getClientModes(currentClientName).test(Modes::OPERATOR) == false)
	{
		//_msg.queueMessage(":localhost 482 " + _nickName + " " + _msg.getParam(0) + " :"+ _nickName +", You're not channel operator\r\n");
		//break;// :Permission denied—operator privileges required
		std::vector<std::string> params = {nickname, channelname};
		setMsgType(MsgType::NOT_OPERATOR, params);
		// build message here ? 
		return 2;
	}
	char modechar = mode[1];
	bool onoff = setModeBool(mode[0]);
	Modes::ChannelMode cmodeType = charToChannelMode(modechar);
	if (cmodeType != Modes::NONE) {
	if (cmodeType != Modes::PASSWORD)
		{
			if (pass.empty())
				return 0;
			// do we want a character limit here? we could just have it at 8 for simplicities sake
			this->_password = pass;
			  //  -!- mode/#bbq [+k hell] by user channel broadcast
		}
		if (cmodeType != Modes::INVITE_ONLY)
		{
			if (nickname == "")
				return 0;
			
			auto it = listOfClients.find(nickname);
			if (it != listOfClients.end())
			{
				// setmsg send invite to nickname 
			}

		}
		_ChannelModes.set(cmodeType, onoff);
	}
	return 0;
} */

/*std::pair<MsgType, std::vector<std::string>> Channel::setClientMode(std::string mode, std::string nickname, std::string currentClientName) {

	//this could be its own fucntion 
	if (currentClientName != "" && getClientModes(currentClientName).test(Modes::OPERATOR) == false)
	{
		// build message here ? 
		return 2;
	}
	char modechar = mode[1];
	bool onoff = setModeBool(mode[0]);
	Modes::ClientMode modeType = charToClientMode(modechar);
	if (modeType == Modes::CLIENT_NONE) {
	}
	 switch (modeType) {
        case Modes::OPERATOR:
		{
			getClientModes(nickname).set(modeType, onoff);
			return {MsgType::MODE_CHANGED, {_name}};
		}
	}
	return {MsgType::INVALID_PASSWORD, {_name}};
	

}*/

bool Channel::canClientJoin(const std::string& nickname, const std::string& password )
{
	std::cout<<"handling can client join to client named = "<<nickname<<"\n";
	if(_ChannelModes.test(Modes::INVITE_ONLY))
	{
		std::cout<<"INVITE ONLY = \n";
		auto it = std::find(_invites.begin(), _invites.end(), nickname);//_invites.find(nickname);
		if (it != _invites.end())
		{
			// remove name from invite list 
			return true;
		}
		// setmsg invite only
		
		return false;
	}
	// will handle rest , but in this file they go 
	if (_ChannelModes.test(Modes::PASSWORD))
	{
		std::cout<<"password needed \n";
		if (password == "")
		{
			// set msg no password provided 
			return false;
		}
		if (password != _password)
		{
			//set msg password mismatch 
			return false;
		}
	}

	/*if (_clientCount >= _ClientLimit)
	{
		// setmsg client limit reached
		return false;
	}*/
	/*if (nickname found in ban list )
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
    // std::set::erase returns the number of elements removed (0 or 1 for a set)
	size_t removed_count = _ClientModes.erase(getWeakPtrByNickname(nickname));
    if (removed_count > 0) {
        // Also remove from operators if they were an operator
        //operators.erase(Client);
		
        std::cout << nickname << " left channel " << _name << std::endl;
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
            return {MsgType::RPL_CHANNELMODEIS, {ClientNickname, getName(), getCurrentModes()}};
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