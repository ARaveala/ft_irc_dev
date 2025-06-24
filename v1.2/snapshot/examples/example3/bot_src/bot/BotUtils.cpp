#include "Bot.h"

/**
 * @brief	checks whether user's desired nickname fits within RFC2812 standard
 *			allowed chars: a-z, A-Z, 0-9, "[", "]", "\", "_", "-", "^", "|", "{", "}"
 *			however, first character is not allowed to be a digit or "-"
 *
 * @param nickname
 * @return true
 * @return false
 */
bool Bot::isValidNickname(std::string nickname)
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

void Bot::reConnection()
{
	int counter = 10;
	while (counter && !Bot::signal_)
	{
		std::cout << '\r' << std::setw(2) << std::setfill('0') << counter-- << std::flush;
		sleep(1);
	}
	std::cout << "\nRetry again!!!\n"
			  << std::flush;
	close(server_fd_);
	setRegisterStatus(false);
	init_bot();
}

std::map<std::string, void (BotCommand::*)(const BotMessage &msg)> const &Bot::getSupportedCommands() const
{
	return supported_commands_;
}

int const &Bot::getServerfd() const
{
	return server_fd_;
}

void Bot::send_response(int fd, const std::string &response)
{
	std::cout << YELLOW << "Response: "
			  << response << RESET;
	if (send(fd, response.c_str(), response.length(), 0) < 0)
		std::cerr << "Response send() faild" << std::endl;
}

void	Bot::readConfigFile()
{
	std::ifstream config_file(CONFIG_FILE);
	if (config_file.is_open())
	{
	    std::stringstream ss;
        ss << config_file.rdbuf();
        std::string line;
        std::cout << "Reading the file" << std::endl;

        bool reading_operators = false;
        bool reading_forbidden_words = false;

        while (std::getline(ss, line))
        {
            // Trim leading and trailing whitespace
            line.erase(0, line.find_first_not_of(" \t\n\r"));
            line.erase(line.find_last_not_of(" \t\n\r") + 1);

            if (line.empty())
                continue;

            if (line == "OPERATORS:")
            {
                reading_operators = true;
                reading_forbidden_words = false;
                continue;
            }
            else if (line == "FORBIDDEN WORDS:")
            {
                reading_operators = false;
                reading_forbidden_words = true;
                continue;
            }

            if (reading_operators)
            {
                std::istringstream iss(line);
                std::string nickname, username;
                if (iss >> nickname >> username)
                {
                    operators_[nickname] = username;
                    std::cout << "Operator: " << nickname << " " << username << std::endl;
                }
            }
            else if (reading_forbidden_words)
            {
                if (line.find(' ') == std::string::npos)
                {
                    forbidden_words_.push_back(line);
                    std::cout << "Forbidden word: " << line << std::endl;
                }
            }
		}
	}
}



std::vector<std::string> const &Bot::getForbiddenWords() const
{
	return forbidden_words_;
}

std::vector<std::string> const &Bot::getViolatedUsers() const
{
	return violated_users_;
}

void Bot::insertInViolatedUsers(std::string const &username)
{
	violated_users_.push_back(username);
}

std::unordered_map<std::string, std::string> const &Bot::getOperators() const
{
	return operators_;
}