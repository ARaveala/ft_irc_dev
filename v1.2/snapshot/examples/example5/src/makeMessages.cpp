
#include "Server.hpp"


void	printMsg(Msg msg)									//Debug function
{
	std::cout << "Command: " << msg.command << std::endl;
	for (size_t i = 0;  i < msg.parameters.size();  i++)
	{
		std::cout  << "Parameter[" << i << "]: " << msg.parameters[i] << std::endl;
	}
	if (msg.trailing_msg != "")
	{
		std::cout << "Trailing_msg: " << msg.trailing_msg << std::endl;
	}
}

void	printArray(std::vector<std::string>message_array)		//Debug function
{
	for (size_t i = 0; i < message_array.size(); i++)
	{
		std::cout << "Array: " << message_array[i] << std::endl;
	}
}

size_t	skipSpaces(std::string string, size_t index)
{
	while (string[index] == ' ')
	{
		index++;
	}
	return (index);
}

/*
	Splits a String separated by Spaces to an Array. 
	(However trailing message spaces are not removed.)
	std::string ---> std::vector<std::string>
*/
void	stringToArray(std::string string, std::vector<std::string> &array)
{
    std::string	word;
	size_t		start = 0;
	size_t		end = 0;
	size_t		length = 0;

	start = skipSpaces(string, start);
	while (true)
	{
		end = string.find(' ', start);							//Find index of next space
		if (end == std::string::npos || string[start] == ':')	//i. Extract last word, if a. space character IS NOT found or b. if trailing message IS found
		{
			word = string.substr(start);
			array.push_back(word);
			break ;
		}
		else													//ii. otherwise calculate length of word and extract
		{
			length = end - start;

			word = string.substr(start, length);
			array.push_back(word);

			end = skipSpaces(string, end);
			start = end;
		}
	}
}


void	makeMsgfromString(Msg &msg, std::string message)
{
    std::vector<std::string>	message_array;

	
	stringToArray(message, message_array);

    initializeMsg(msg, message_array);
	return ;
}

/*
	Makes Msg Object received from Irssi
	(Can make multiple Msg objects)
*/
void	Server::makeMessages(std::vector<Msg> &msgs, std::string buffer)
{
	Msg				msg;

	std::string		single_msg;
    size_t			start = 0;
	size_t			end = 0;
	size_t			length = 0;

 	if (buffer.find("\r") != std::string::npos)		//the usual case (i.e. Irssi)
	{
		while ((end = buffer.find("\r", start)) != std::string::npos)
		{
			length = end - start;
			single_msg = buffer.substr(start, length);
			makeMsgfromString(msg, single_msg);
			msgs.push_back(msg);
			start = end + 2;
		}
	}
	else		//handles case when there is no '\r' return but there is '\n' (i.e. NetCat)
	{
		buffer += "\n"; // because the new line is missing
		while ((end = buffer.find("\n", start)) != std::string::npos)
		{
			length = end - start;
			single_msg = buffer.substr(start, length);
			makeMsgfromString(msg, single_msg);
			msgs.push_back(msg);
			start = end + 1;
		}
	}
}
