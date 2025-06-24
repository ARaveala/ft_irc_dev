#pragma once

class Msg
{
	public:
		std::string					command;
		std::vector<std::string>	parameters;
		std::string					trailing_msg;
};
