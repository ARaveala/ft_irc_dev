#pragma once

#include <string>
#include <map>
#include <vector>
#include <iostream>

#include "Channel.hpp"
#include "Client.hpp"

class ChannelManager {
    private:
        std::map<std::string, Channel*> channels;
    public:
        ChannelManager();
        ~ChannelManager();
        Channel* createChannel(const std::string& channelName);
        Channel* getChannel(const std::string& channelName);
        bool deleteChannel(const std::string& ChannelName);
        std::vector<std::string> listChannels() const;
        bool channelExists(const std::string& channelName) const;
};