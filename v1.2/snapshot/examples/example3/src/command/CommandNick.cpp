#include "Command.h"

/**
 * @brief handles the NICK command which sets/changes user's nickname
 * @param msg
 */

void Command::handleNick(const Message &msg)
{
	std::shared_ptr<Client> client_ptr = msg.getClientPtr();
	if (!client_ptr)
	{
		std::cerr << ("null ptr in handleNick") << std::endl;
		return;
	}
	int client_fd = client_ptr->getFd();
	if (!server_ptr_->hasClientSentPass(client_ptr))
		return;
	std::vector<std::string> parameters = msg.getParameters();
	if (parameters.empty())
	{
		server_ptr_->sendResponse(client_fd, ERR_NONICKNAMEGIVEN(client_ptr->getClientPrefix()));
		return;
	}
	std::string new_nickname = parameters[0]; // desired nickname is the first parameter, we'll just ignore the rest for now
	std::string current_nickname = client_ptr->getNickname();

	if (new_nickname == current_nickname)
		return;
	std::string new_nick_lowercase = new_nickname;
	std::string current_nick_lowercase = current_nickname;

	std::transform(new_nick_lowercase.begin(), new_nick_lowercase .end(), new_nick_lowercase.begin(), ::tolower); // Convert the nickname to lowercase
	std::transform(current_nick_lowercase.begin(), current_nick_lowercase .end(), current_nick_lowercase.begin(), ::tolower); // Convert the nickname to lowercase

	if (new_nick_lowercase != current_nick_lowercase)
	{
		if (isValidNickname(new_nickname) == false)
		{
			server_ptr_->sendResponse(client_fd, ERR_ERRONEUSNICK(server_ptr_->getServerHostname(), client_ptr->getNickname(), new_nickname));
			return;
		}
		if (isNicknameInUse(new_nickname) == true)
		{
			server_ptr_->sendResponse(client_fd, ERR_NICKINUSE(server_ptr_->getServerHostname(), new_nickname));
			return;
		}
	}

	std::string old_prefix = client_ptr->getClientPrefix();
	server_ptr_->sendResponse(client_fd, RPL_NICKCHANGE(old_prefix, new_nickname));
	client_ptr->setNickname(new_nickname);
	client_ptr->setClientPrefix();
	if (!client_ptr->getRegisterStatus() && !client_ptr->getUsername().empty())
	{
		client_ptr->registerClient();
		server_ptr_->welcomeAndMOTD(client_fd, server_ptr_->getServerHostname(), client_ptr->getNickname(), client_ptr->getClientPrefix());
	}
	std::vector<std::weak_ptr<Channel>> channel_list = client_ptr->getChannels();//else
	if (!channel_list.empty())
	{
		for (auto channel_ptr: channel_list)
			channel_ptr.lock()->broadcastMessage(client_ptr, RPL_NICKCHANGECHANNEL(old_prefix, new_nickname), server_ptr_);
	}
}

bool Command::isNicknameInUse(std::string const &nickname)
{
	return server_ptr_->findClientUsingNickname(nickname) != nullptr;
}

/**
 * @brief	checks whether user's desired nickname fits within RFC2812 standard
 *			allowed chars: a-z, A-Z, 0-9, "[", "]", "\", "_", "-", "^", "|", "{", "}"
 *			however, first character is not allowed to be a digit or "-"
 *
 * @param nickname
 * @return true
 * @return false
 */
bool Command::isValidNickname(std::string &nickname)
{
	if (isdigit(nickname.front()) || nickname.front() == '-')
		return false;
	if (nickname.size() > NICK_MAX_LENGTH) // if nickname is too long, it gets truncated
		nickname = nickname.substr(0, NICK_MAX_LENGTH);
	std::regex pattern("([A-Za-z0-9\\[\\]\\\\_\\-\\^|{}])\\w*");
	if (std::regex_match(nickname, pattern))
		return true;
	else
		return false;
}