#include "Server.hpp"


time_t stringToUnixTimeStamp(std::string time)
{
	struct tm tm;
	if (strptime(time.c_str(), "%a %b %d %H:%M:%S %Y", &tm) == NULL)
	{
		std::cout << "Error: strptime" << std::endl;
		return (-1);
	}
	return (mktime(&tm));
}

std::string getCurrentTime()
{
	time_t now = time(0);
	char timeStr[100];
    struct tm *localTime = localtime(&now);
    strftime(timeStr, sizeof(timeStr), "%a %b %d %H:%M:%S %Y", localTime);
    return std::string(timeStr);
}

std::string getCurrentEpochTime()
{
    time_t now = time(0);
    return std::to_string(now);
}
