#include "ChannelManager.hpp"
#include <iostream>

ChannelManager::ChannelManager() {
    std::cout << "ChannelManager created." << std::endl;
}

ChannelManager::~ChannelManager() {
    std::cout << "ChannelManager destroying channels..." << std::endl;
    for (auto const& [name, channel] : channels) {
        delete channel; // Delete the Channel object allocated with new
    }
    channels.clear();
    std::cout << "ChannelManager destroyed." << std::endl;
}

Channel* ChannelManager::createChannel(const std::string& channelName) {
    if (channels.count(channelName) == 0) {
        Channel* newChannel = new Channel(channelName);
        channels[channelName] = newChannel;
        std::cout << "Channel '" << channelName << "' created." << std::endl;
        return newChannel;
    }
    std::cerr << "Error: Channel '" << channelName << "' already exists." << std::endl;
    return nullptr; // Channel already exists
}

Channel* ChannelManager::getChannel(const std::string& channelName) {
    auto it = channels.find(channelName);
    if (it != channels.end()) {
        return it->second;
    }
    return nullptr; // Channel not found
}

bool ChannelManager::deleteChannel(const std::string& channelName) {
    auto it = channels.find(channelName);
    if (it != channels.end()) {
        delete it->second; // Delete the Channel object
        channels.erase(it); // Remove from the map
        std::cout << "Channel '" << channelName << "' deleted." << std::endl;
        return true;
    }
    std::cerr << "Error: Channel '" << channelName << "' not found for deletion." << std::endl;
    return false; // Channel not found
}

std::vector<std::string> ChannelManager::listChannels() const {
    std::vector<std::string> channelNames;
    for (auto const& [name, channel] : channels) {
        channelNames.push_back(name);
    }
    return channelNames;
}

bool ChannelManager::channelExists(const std::string& channelName) const {
    return channels.count(channelName) > 0;
}
