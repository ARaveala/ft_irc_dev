#include "Command.h"

/**
 * Extracts additional mode parameters from the command line input starting from the given index.
 *
 * @param mode_arguments Vector to store extracted mode arguments.
 * @param parameters Vector containing all command parameters.
 * @param start_index Index to start extraction from.
 */
static void extractModeArguments(std::vector<std::string> &mode_arguments, const std::vector<std::string> &parameters, size_t start_index)
{
    for (size_t i = start_index; i < parameters.size(); ++i)
        mode_arguments.push_back(parameters[i]);
}

/**
 * Retrieves the current modes set on a channel.
 *
 * @param channel_ptr Shared pointer to the channel object.
 * @param user_on_channel Determines whether user sending the command is on said channel
 * @return A string representing the modes set on the channel.
 */
static std::string getChannelModes(std::shared_ptr<Channel> channel_ptr, bool user_on_channel)
{
    std::string channel_modes = "+";
	std::string mode_params = "";
    if (channel_ptr->getModeN()) channel_modes += "n";
    if (channel_ptr->getModeI()) channel_modes += "i";
    if (channel_ptr->getModeT()) channel_modes += "t";
    if (channel_ptr->getModeK())
	{
		channel_modes += "k";
		if (user_on_channel) // only show the key if user is on channel
			mode_params += " " + channel_ptr->getChannelKey();
	}
    if (channel_ptr->getModeL())
	{
		channel_modes += "l";
		mode_params += " " + std::to_string(channel_ptr->getChannelLimit());
	}
    return channel_modes + mode_params;
}

/**
 * Handles the MODE command, determining whether to apply modes to a user or a channel.
 *
 * @param msg The message containing the command details.
 */
void Command::handleMode(const Message &msg)
{
	std::shared_ptr<Client> client_ptr = msg.getClientPtr();
	if (!client_ptr)
	{
		std::cerr << ("null ptr in handleMode") << std::endl;
		return;
	}
    int client_fd = client_ptr->getFd();
    const auto &parameters = msg.getParameters();

    if (parameters.empty())
    {
        server_ptr_->sendResponse(client_fd, ERR_NEEDMOREPARAMS(client_ptr->getClientPrefix(), "MODE"));
        return;
    }

    const std::string &target = parameters[0];
    std::string mode_string = parameters.size() > 1 ? parameters[1] : "";
    std::vector<std::string> mode_arguments;
    if (parameters.size() > 2)
        extractModeArguments(mode_arguments, parameters, 2);

    if (target.front() != '#' && target.front() != '&')
        handleUserMode(client_ptr, target, mode_string);
    else
        handleChannelMode(client_ptr, target, mode_string, mode_arguments);
}

/**
 * Handles mode changes for a user.
 *
 * @param client Shared pointer to the client object.
 * @param target The target user for the mode change.
 * @param mode_string The string representing the modes to be applied.
 */
void Command::handleUserMode(std::shared_ptr<Client> client_ptr, const std::string &target, const std::string &mode_string)
{
    int client_fd = client_ptr->getFd();

    if (target == client_ptr->getNickname())
        applyUserMode(client_ptr, mode_string);
    else if (server_ptr_->findClientUsingNickname(target))
        server_ptr_->sendResponse(client_fd, ERR_USERSDONTMATCH(server_ptr_->getServerHostname(), client_ptr->getNickname()));
    else
        server_ptr_->sendResponse(client_fd, ERR_NOSUCHNICK(server_ptr_->getServerHostname(), client_ptr->getNickname(), target));
}

/**
 * Handles mode changes for a channel.
 *
 * @param client_ptr Shared pointer to the client object.
 * @param target The target channel for the mode change.
 * @param mode_string The string representing the modes to be applied.
 * @param mode_arguments The arguments associated with the modes.
 */
void Command::handleChannelMode(std::shared_ptr<Client> client_ptr, const std::string &target,
								const std::string &mode_string, const std::vector<std::string> &mode_arguments)
{
    auto channel_ptr = server_ptr_->findChannel(target);
    int client_fd = client_ptr->getFd();

    if (!channel_ptr)
        server_ptr_->sendResponse(client_fd, ERR_NOSUCHCHANNEL(server_ptr_->getServerHostname(), client_ptr->getNickname(), target));
    else if (mode_string.empty())
        server_ptr_->sendResponse(client_fd, RPL_CHANNELMODEIS(server_ptr_->getServerHostname(), client_ptr->getNickname(), channel_ptr->getName(), getChannelModes(channel_ptr, channel_ptr->isUserOnChannel(client_ptr->getNickname()))));
	else if (mode_string == "b") // ignore the mode b message irssi sends constantly
		return;
    else if (!channel_ptr->isUserOnChannel(client_ptr->getNickname()))
        server_ptr_->sendResponse(client_fd, ERR_NOTONCHANNEL(server_ptr_->getServerHostname(), client_ptr->getNickname(), channel_ptr->getName()));
    else if (!channel_ptr->isOperator(client_ptr))
        server_ptr_->sendResponse(client_fd, ERR_CHANOPRIVSNEEDED(server_ptr_->getServerHostname(), channel_ptr->getName()));
    else
        applyChannelModes(client_ptr, channel_ptr, mode_string, mode_arguments);
}

/**
 * Applies mode changes to a user.
 *
 * @param client_ptr Shared pointer to the client object.
 * @param mode_string The string representing the modes to be applied.
 */
void Command::applyUserMode(std::shared_ptr<Client> client_ptr, const std::string &mode_string)
{
    int client_fd = client_ptr->getFd();
    std::string changed_modes;

    if (!mode_string.empty())
    {
        bool mode_setting = true;
        char last_sign = '\0';

        for (char mode_char : mode_string)
        {
            if (mode_char == '-')
                mode_setting = false;
            else if (mode_char == '+')
                mode_setting = true;
            else if (mode_char == 'i')
            {
                client_ptr->setModeI(mode_setting);
                appendToChangedModeString(mode_setting, changed_modes, last_sign, mode_char);
            }
            else
                server_ptr_->sendResponse(client_fd, ERR_UMODEUNKNOWNFLAG(server_ptr_->getServerHostname(), client_ptr->getNickname(), mode_char));
        }
        server_ptr_->sendResponse(client_fd, RPL_UMODECHANGE(client_ptr->getNickname(), changed_modes));
    }
    else
        sendCurrentUserModes(client_ptr);
}

/**
 * Sends the current modes set on a user to the client.
 *
 * @param client_ptr Shared pointer to the client object.
 */
void Command::sendCurrentUserModes(std::shared_ptr<Client> client_ptr)
{
    int client_fd = client_ptr->getFd();
    std::string current_user_modes = "+";
    if (client_ptr->getModeI())
        current_user_modes += "i";
    if (client_ptr->getModeLocalOp())
        current_user_modes += "O";
    server_ptr_->sendResponse(client_fd, RPL_UMODEIS(server_ptr_->getServerHostname(), client_ptr->getNickname(), current_user_modes));
}

/**
 * Applies mode changes to a channel.
 *
 * @param client_ptr Shared pointer to the client object.
 * @param channel_ptr Shared pointer to the channel object.
 * @param mode_string The string representing the modes to be applied.
 * @param mode_arguments The arguments associated with the modes.
 */
void Command::applyChannelModes(std::shared_ptr<Client> client_ptr, std::shared_ptr<Channel> channel_ptr, 
								const std::string &mode_string, const std::vector<std::string> &mode_arguments)
{
    int client_fd = client_ptr->getFd();
    bool mode = true;
    std::string changed_modes, used_parameters;
    size_t arg_index = 0;
    std::vector<std::pair<char, bool>> mode_changes;

    parseModeString(mode_string, mode_changes, mode);

    char last_sign = '\0';
    std::string processed_modes;

    for (const auto &[mode_char, mode_setting] : mode_changes)
    {
        if (processed_modes.find(mode_char) != std::string::npos)
            continue;
        handleModeChange(client_ptr, channel_ptr, mode_char, mode_setting, mode_arguments, arg_index, changed_modes, used_parameters, last_sign);
        processed_modes += mode_char;
    }

    if (!changed_modes.empty())
    {
        std::string final_response = RPL_CHANGEMODE(client_ptr->getClientPrefix(), channel_ptr->getName(), changed_modes, used_parameters);
        server_ptr_->sendResponse(client_fd, final_response);
        channel_ptr->broadcastMessage(client_ptr, final_response, server_ptr_);
    }
}

/**
 * Parses a mode string and determines the mode changes.
 *
 * @param mode_string The string representing the modes to be applied.
 * @param mode_changes Vector to store the mode changes.
 * @param mode_setting Boolean indicating if the mode is being set or unset.
 */
void Command::parseModeString(const std::string &mode_string, std::vector<std::pair<char, bool>> &mode_changes, bool &mode_setting)
{
    for (char mode_char : mode_string)
    {
        if (mode_char == '+')
            mode_setting = true;
        else if (mode_char == '-')
            mode_setting = false;
        else
            mode_changes.emplace_back(mode_char, mode_setting);
    }
}

/**
 * Handles a specific mode change for a channel.
 *
 * @param client_ptr Shared pointer to the client object.
 * @param channel_ptr Shared pointer to the channel object.
 * @param mode_char The mode character being processed.
 * @param mode_setting Boolean indicating if the mode is being set or unset.
 * @param mode_arguments The arguments associated with the modes.
 * @param arg_index The current index in the mode arguments.
 * @param changed_modes The string representing the current mode changes.
 * @param used_parameters The string representing the used parameters.
 * @param last_sign The last mode character processed.
 */
void Command::handleModeChange(std::shared_ptr<Client> client_ptr, std::shared_ptr<Channel> channel_ptr, char mode_char, bool mode_setting,
								const std::vector<std::string> &mode_arguments, size_t &arg_index, std::string &changed_modes,
								std::string &used_parameters, char &last_sign)
{
    int client_fd = client_ptr->getFd();
    bool param_required = modeRequiresParameter(mode_char);
    bool param_mandatory = mandatoryModeParameter(mode_char);

    if (param_required && mode_setting && arg_index >= mode_arguments.size())
    {
        if (param_mandatory)
            server_ptr_->sendResponse(client_fd, ERR_INVALIDMODEPARAM(server_ptr_->getServerHostname(), client_ptr->getNickname(), channel_ptr->getName(), mode_char, "", "Mode parameter missing."));
        return;
    }

    switch (mode_char)
    {
    case 'i':
        channel_ptr->setModeI(mode_setting);
        appendToChangedModeString(mode_setting, changed_modes, last_sign, mode_char);
        break;
    case 'k':
        handleModeK(channel_ptr, mode_setting, mode_arguments, arg_index, changed_modes, used_parameters, last_sign);
        break;
    case 'l':
        handleModeL(channel_ptr, mode_setting, mode_arguments, arg_index, changed_modes, used_parameters, last_sign);
        break;
    case 'n':
        channel_ptr->setModeN(mode_setting);
        appendToChangedModeString(mode_setting, changed_modes, last_sign, mode_char);
        break;
    case 't':
        channel_ptr->setModeT(mode_setting);
        appendToChangedModeString(mode_setting, changed_modes, last_sign, mode_char);
        break;
    case 'o':
        handleModeO(client_ptr, channel_ptr, mode_arguments, mode_setting, arg_index, changed_modes, used_parameters, last_sign);
        break;
    default:
        server_ptr_->sendResponse(client_fd, ERR_UNKNOWNMODE(server_ptr_->getServerHostname(), client_ptr->getNickname(), mode_char));
        break;
    }
}

/**
 * Handles the 'k' (key) mode change for a channel.
 *
 * @param channel_ptr Shared pointer to the channel object.
 * @param mode_setting Boolean indicating if the mode is being set or unset.
 * @param mode_arguments The arguments associated with the modes.
 * @param arg_index The current index in the mode arguments.
 * @param changed_modes The string representing the current mode changes.
 * @param used_parameters The string representing the used parameters.
 * @param last_sign The last mode character processed.
 */
void Command::handleModeK(std::shared_ptr<Channel> channel_ptr, bool mode_setting, const std::vector<std::string> &mode_arguments, size_t &arg_index,
							std::string &changed_modes, std::string &used_parameters, char &last_sign)
{
    if (mode_setting)
    {
        if (arg_index < mode_arguments.size())
        {
			std::string channel_key = mode_arguments[arg_index];
			if (channel_key.size() > CHANNEL_KEY_MAX_LENGTH) // if nickname is too long, it gets truncated
				channel_key = channel_key.substr(0, CHANNEL_KEY_MAX_LENGTH);	
            channel_ptr->setModeK(true);
            channel_ptr->setChannelKey(channel_key);
            appendToChangedModeString(mode_setting, changed_modes, last_sign, 'k');
            if (!used_parameters.empty())
                used_parameters += " ";
            used_parameters += channel_key;
            arg_index++;
        }
    }
    else
    {
        channel_ptr->setModeK(false);
        channel_ptr->setChannelKey("");
        appendToChangedModeString(mode_setting, changed_modes, last_sign, 'k');
    }
}

/**
 * Handles the 'l' (limit) mode change for a channel.
 *
 * @param channel_ptr Shared pointer to the channel object.
 * @param mode_setting Boolean indicating if the mode is being set or unset.
 * @param mode_arguments The arguments associated with the modes.
 * @param arg_index The current index in the mode arguments.
 * @param changed_modes The string representing the current mode changes.
 * @param used_parameters The string representing the used parameters.
 * @param last_sign The last mode character processed.
 */
void Command::handleModeL(std::shared_ptr<Channel> channel_ptr, bool mode_setting, const std::vector<std::string> &mode_arguments, size_t &arg_index, std::string &changed_modes, std::string &used_parameters, char &last_sign)
{
    if (mode_setting && arg_index < mode_arguments.size())
    {
        if (mode_arguments[arg_index].find_first_not_of("0123456789") == std::string::npos)
        {
			int limit;
			try
			{
				limit = std::stoi(mode_arguments[arg_index]);
			}
			catch (const std::exception& e) 
			{
				limit = std::numeric_limits<int>::max();
				std::cout << RED << "EXCEPTION: Proposed channel user limit was out of range" << RESET << std::endl;
			}
			channel_ptr->setModeL(true, limit);
			appendToChangedModeString(mode_setting, changed_modes, last_sign, 'l');
			if (!used_parameters.empty())
				used_parameters += " ";
			used_parameters += std::to_string(limit);
       		arg_index++;
		}
    }
    else
    {
        channel_ptr->setModeL(false, 0);
        appendToChangedModeString(mode_setting, changed_modes, last_sign, 'l');
    }
}

/**
 * Handles the 'o' (operator) mode change for a channel.
 *
 * @param client_ptr Shared pointer to the client object.
 * @param channel Shared pointer to the channel object.
 * @param mode_arguments The arguments associated with the modes.
 * @param mode_setting Boolean indicating if the mode is being set or unset.
 * @param arg_index The current index in the mode arguments.
 * @param changed_modes The string representing the current mode changes.
 * @param used_parameters The string representing the used parameters.
 * @param last_sign The last mode character processed.
 */
void Command::handleModeO(std::shared_ptr<Client> client_ptr, std::shared_ptr<Channel> channel_ptr, const std::vector<std::string> &mode_arguments, bool mode_setting, size_t &arg_index, std::string &changed_modes, std::string &used_parameters, char &last_sign)
{
    if (arg_index < mode_arguments.size())
    {
        if (applyModeO(client_ptr, channel_ptr, mode_arguments[arg_index], mode_setting))
        {
            appendToChangedModeString(mode_setting, changed_modes, last_sign, 'o');
            if (!used_parameters.empty())
                used_parameters += " ";
            used_parameters += mode_arguments[arg_index];
            arg_index++;
        }
    }
}

/**
 * Determines if a mode requires a parameter.
 *
 * @param mode The mode character.
 * @return True if the mode requires a parameter, false otherwise.
 */
bool Command::modeRequiresParameter(char mode_char)
{
    return mode_char == 'k' || mode_char == 'l' || mode_char == 'o';
}

/**
 * Determines if a mode parameter is mandatory.
 *
 * @param mode The mode character.
 * @return True if the mode parameter is mandatory, false otherwise.
 */
bool Command::mandatoryModeParameter(char mode_char)
{
    return mode_char == 'l';
}

/**
 * Applies the operator mode change to a channel.
 *
 * @param client_ptr Shared pointer to the client object.
 * @param channel_ptr Shared pointer to the channel object.
 * @param target_nickname The nickname of the target user.
 * @param mode_setting Boolean indicating if the mode is being set or unset.
 * @return True if the mode change was successful, false otherwise.
 */
bool Command::applyModeO(std::shared_ptr<Client> client_ptr, std::shared_ptr<Channel> channel_ptr, const std::string &target_nickname, bool mode_setting)
{
    auto target_ptr = server_ptr_->findClientUsingNickname(target_nickname);
    int client_fd = client_ptr->getFd();

    if (!target_ptr)
    {
        server_ptr_->sendResponse(client_fd, ERR_NOSUCHNICK(server_ptr_->getServerHostname(), client_ptr->getNickname(), target_nickname));
        return false;
    }

    if (!channel_ptr->isUserOnChannel(target_nickname))
    {
        server_ptr_->sendResponse(client_fd, ERR_USERNOTINCHANNEL(server_ptr_->getServerHostname(), client_ptr->getNickname(), target_nickname, channel_ptr->getName()));
        return false;
    }

    return channel_ptr->changeOpStatus(target_ptr, mode_setting);
}

/**
 * Appends the mode change to the changed modes string.
 *
 * @param mode_setting Boolean indicating if the mode is being set or unset.
 * @param changed_modes The string representing the current mode changes.
 * @param last_sign The last mode character processed.
 * @param mode_char The mode character being processed.
 */
void Command::appendToChangedModeString(bool mode_setting, std::string &changed_modes, char &last_sign, char mode_char)
{
    if (mode_setting && last_sign != '+')
        changed_modes += '+';
    if (!mode_setting && last_sign != '-')
        changed_modes += '-';
    changed_modes += mode_char;
    last_sign = mode_setting ? '+' : '-';
}