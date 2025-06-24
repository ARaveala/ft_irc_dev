#pragma once
#include <string>
#include <map> 
#include <vector>
#include <iostream>

#include "Channel.hpp"
#include "User.hpp"

class ChannelManager {
    private:
        std::map<std::string, Channel*> channels;
    public:
        ChannelManager();
        ~ChannelManager();
        Channel* createChannel(const std::string& channelName);
        Channel* getChannel(const std::string& channelName);
        bool deleteChannel(const std::string& channelName);
        std::vector<std::string> listChannels() const;
        bool channelExists(const std::string& channelName) const;
};