
// RFC 2812
// [<prefix> <SPACE>] <command> <params> <CRLF>

#include <iostream>
#include <string>
#include <vector>
#include "IrcMessage.hpp"

// Print the contents of an IrcMessage object
void printMessage(const IrcMessage& msg)
{
    std::cout << "  Prefix: '" << msg.getPrefix() << "'" << std::endl;
    std::cout << "  Command: '" << msg.getCommand() << "'" << std::endl;
    std::cout << "  Parameters:" << std::endl;
    const std::vector<std::string>& params = msg.getParams();
    if (params.empty()) {
        std::cout << "    (No parameters)" << std::endl;
    } else {
        for (size_t i = 0; i < params.size(); ++i) {
            std::cout << "    [" << i << "]: '" << params[i] << "'" << std::endl;
        }
    }
    std::cout << "---" << std::endl; // Separator for messages
}

int main()
{
    // Define a list of example raw IRC messages to test
    std::vector<std::string> raw_messages = {
        "PING :irc.example.com\r\n",                      // Basic PING with trailing param
        ":server.name PONG server.name :server.name\r\n", // PONG with prefix and two params (one trailing)
        "NICK new_nick\r\n",                              // NICK with one param, no prefix
        ":nick!user@host JOIN #channel\r\n",              // JOIN with prefix and one param
        ":nick!user@host PRIVMSG #channel :Hello team!\r\n", // PRIVMSG with prefix, target param, and trailing text param
        "MODE #channel +o Alice\r\n",                     // MODE with three params, no prefix
        "QUIT :Leaving\r\n",                              // QUIT with trailing param
        "JUSTCOMMAND\r\n",                                // Command only, no params, no prefix
        ":only.prefix COMMAND\r\n",                       // Prefix and command only
        "INVITE user #channel\r\n",                       // Two params, no trailing
        "TOPIC #channel :New topic text\r\n",             // Channel and trailing topic
        ":service.name 331 user #channel :No topic is set\r\n", // Numeric reply with prefix and trailing
        "MISSING_CRLF",                                   // Invalid message - missing terminator
        ":prefix COMMAND missing trailing space\r\n"     // Non-trailing params split correctly
        // Add more test cases as you think of them!
    };

    std::cout << "--- Testing IrcMessage Parsing and Serialisation---" << std::endl;

    // Loop through each raw message and attempt to parse it and then serialise it
    for (const std::string& raw_msg : raw_messages)
    {
        std::cout << "Parsing raw message: \"" << raw_msg << "\"" << std::endl;

        IrcMessage message; // Create an IrcMessage object

        // Attempt to parse the raw string
        bool success = message.parse(raw_msg);

        if (success) {
            std::cout << "  Parsing successful!" << std::endl;
            printMessage(message); // Print the parsed contents
			// test serialisation with toRawString
			std::string reserialised_msg = message.toRawString();
			std::cout << "  Serialised raw message: \"" << reserialised_msg << "\"" << std::endl;
        } else {
            std::cout << "  Parsing failed!" << std::endl;
            std::cout << "---" << std::endl;
        }
    }

    std::cout << "--- End of Testing ---" << std::endl;

    return 0;
}

