#include "BotCommand.hpp"

BotCommand::BotCommand(Bot *bot_ptr)
{
	bot_ = bot_ptr;
}

BotCommand::~BotCommand()
{
}

static std::string getNicknameFromPrefix(std::string prefix)
{
	std::string nickname = "";
	if (prefix[0] == ':')
		prefix.erase(0, 1);
	size_t end_pos = prefix.find('!');
	if (end_pos != std::string::npos)
		nickname = prefix.substr(0, end_pos);
	return nickname;
}

static std::string getUsernameFromPrefix(std::string prefix)
{
	std::string username = "";
	size_t start_pos = prefix.find('~');
	size_t end_pos = prefix.find('@');
	if (start_pos != std::string::npos && end_pos != std::string::npos)
		username = prefix.substr(start_pos, end_pos - start_pos);
	return username;
}

void BotCommand::handleJoin(const BotMessage &msg)
{
	std::string reply_number = msg.getReplyNumber();
	int fd = bot_->getServerfd();
	if (reply_number == "451")
	{
		bot_->send_response(fd, RPL_PASS(bot_->getServerPassword()));
		bot_->send_response(fd, RPL_NICK(bot_->getNickname()));
		bot_->send_response(fd, RPL_USER(bot_->getUsername()));
		return;
	}
	std::string prefix = msg.getPrefix();
	std::string nickname = getNicknameFromPrefix(prefix);
	std::string username = getUsernameFromPrefix(prefix);
	std::string channel_name = msg.getParameters().front();
	if (nickname != bot_->getNickname())
	{
		bot_->send_response(fd, RPL_PRIVMSG(channel_name, "Hello " + nickname + ", good to see you!"));
		auto operators = bot_->getOperators();
		if (!operators.empty())
		{
			auto iter = operators.find(nickname);
			if (iter != operators.end())
			{
				if (iter->second == username)
					bot_->send_response(fd, "MODE " + channel_name + " +o " + nickname + CRLF);
			}
		}
	}
}

void BotCommand::handleNick(const BotMessage &msg)
{
	std::string reply_number = msg.getReplyNumber();
	int fd = bot_->getServerfd();
	std::string nick = bot_->getNickname();
	if (reply_number == "433")
	{
		size_t pos = nick.find_last_of("0123456789");
		if (pos != std::string::npos)
		{
			std::string num_in_nick = nick.substr(pos);
			int number;
			std::stringstream(num_in_nick) >> number;
			++number;
			nick.replace(pos, num_in_nick.length(), std::to_string(number));
			bot_->setNickname(nick);
		}
		else
			nick += "_";
		bot_->send_response(fd, RPL_NICK(bot_->getNickname()));
	}
}

void BotCommand::handlePrivmsg(const BotMessage &msg)
{
	std::string prefix = msg.getPrefix();
	std::string channel_name = msg.getParameters()[0];
	std::string nickname = getNicknameFromPrefix(prefix);
	std::string username = getUsernameFromPrefix(prefix);
	std::string line(msg.getTrailer());
	int fd = bot_->getServerfd();
	std::string word;
	std::vector<std::string> forbidden_words = bot_->getForbiddenWords();
	std::vector<std::string> violated_users = bot_->getViolatedUsers();
	if (!forbidden_words.empty())
	{
		for (auto word_it: forbidden_words)
		{
			if (line.find(word_it) != std::string::npos)
			{
				if (std::find(violated_users.begin(), violated_users.end(), nickname) != violated_users.end())
				{
					bot_->send_response(fd, KICK_REQUEST(channel_name, nickname + " :You have violated the chat room rules."));
					return;
				}
				else
				{
					bot_->send_response(fd, RPL_PRIVMSG(channel_name, nickname + " :This is the last warning, You have violated the chat room rules."));
					bot_->insertInViolatedUsers(nickname);
					return;
				}
			}
		}
	}
	if (line.find("lol") != std::string::npos)
	{
		bot_->send_response(fd, RPL_PRIVMSG(channel_name, "   /\\O"));
		bot_->send_response(fd, RPL_PRIVMSG(channel_name, "    /\\/"));  
    	bot_->send_response(fd, RPL_PRIVMSG(channel_name, "   /\\"));    
   		bot_->send_response(fd, RPL_PRIVMSG(channel_name, "  /  \\"));
 		bot_->send_response(fd, RPL_PRIVMSG(channel_name, "LOL  LOL"));
		return;
	}
	if (line.find("haha") != std::string::npos){
		bot_->send_response(fd, RPL_PRIVMSG(channel_name, "THAT WAS HILARIOUS!"));
		return;}
}

void BotCommand::handleInvite(const BotMessage &msg)
{
	std::string reply_number = msg.getReplyNumber();
	int fd = bot_->getServerfd();
	if (reply_number == "INVITE" && msg.getParameters()[0] == bot_->getNickname())
		bot_->send_response(fd, "JOIN " + msg.getTrailer() + CRLF); 
}
