
#include "Server.hpp"

std::string		getCommand(std::vector<std::string> array)
{
	return (array[0]);	//First word is always the command.
}

std::vector<std::string>	getParameters(std::vector<std::string> array)
{
	std::vector<std::string> parameters;

	for (size_t i = 1; i < array.size(); i++)
	{
		if (array[i][0] != ':')					//Everything is a parameter, that doesn't start with ':'
		{
			parameters.push_back(array[i]);
		}
    }
	return (parameters);
}

std::string		getTrailingMessage(std::vector<std::string> array)
{
	size_t last_string = array.size() - 1;

	if (array[last_string][0] == ':')			//Checks if trailing string
	{
		return (&(array[last_string][1]));		//the '1' parses out the ':' character
	}
	else
	{
		return ("");
	}
}
/*
	Initializes Msg Object.
	-Command
	-Parameter[s]
	-Trailing message [if exists]
*/
void	initializeMsg(Msg &msg, std::vector<std::string> array)
{	
	msg.command = getCommand(array);
	msg.parameters = getParameters(array);
	msg.trailing_msg = getTrailingMessage(array);
}
