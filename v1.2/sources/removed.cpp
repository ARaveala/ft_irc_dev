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