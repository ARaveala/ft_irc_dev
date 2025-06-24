#pragma once

#include <iostream>
#include <vector>			//std::vector
#include <algorithm>		//std::find()
#include <arpa/inet.h>		//inet_ntop()
#include <netdb.h>			//getaddrinfo()
#include <unistd.h>			//gethostname()

class Client
{
	private:
		std::string _nickname;
		std::string _username;
		std::string _host;
		std::string _hostname;
		std::string _realname;
		std::string _password;
		std::string _prefix;

		std::vector<std::string> _channelsNames;

		bool        _passwordChecked;
		bool        _welcomeSent;
		int         _socket;

	public:
		Client(int socket, const std::string& password);

		//Getters
		std::string     getNickname();
		std::string     getUsername();
		std::string     getHost();
		std::string     getHostname();
		std::string     getRealname();
		std::string     getPassword();
		std::string     getPrefix();
		std::vector<std::string>& getChannelsNames();
		bool            getPasswordChecked();
		bool            getWelcomeSent();
		int             getSocket();

		//Setters
		void        setNickname(std::string str);
		void        setUsername(std::string str);
		void        setPrefix(std::string str);
		void        setHost(std::string str);
		void        setRealname(std::string str);
		void        setHostname(std::string str);
		void        setPasswordChecked(bool value);
		void        setWelcomeSent(bool value);

		//Join / Leave Channel
		void        joinChannel(std::string channelName);
		void        leaveChannel(std::string channelName);  
};
