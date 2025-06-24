#ifndef __BOTMESSAGE_H__
#define __BOTMESSAGE_H__

#include "../headers/headers.h"
#include "../bot/Bot.h"

class BotMessage
{
	private:
	const std::string				raw_message_;
	std::string						prefix_;
	std::string						reply_num_;
	std::vector<std::string>		parameters_;
	std::string						trailer_;
	bool							valid_message_;

	public:
	BotMessage(std::string raw_message);
	~BotMessage();

	bool isValidMessage();
	
	bool analyzeMessage();

	std::string extractPrefix(std::string line);
	bool isValidPrefix(std::string prefix, std::string nickname); // TODO: split analyzemessage() function
	void setPrefix(std::string &prefix);
	void setReplyNumber(std::string raw_message);
	void setParams(std::string raw_message);
	void setTrailer(std::string raw_message);
	
	std::string 				const &getPrefix() const;
	std::string 				const &getReplyNumber() const;
	std::vector<std::string>	const &getParameters() const;
	std::string 				const &getTrailer() const;
	void printMessageContents();
};

#endif