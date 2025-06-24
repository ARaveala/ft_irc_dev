#pragma once

#include <string>
#include <set> 
#include <vector>
#include <iostream>

#include "User.hpp"

class Channel {
    private:
        std::string name;
        std::string topic;
        std::set<User*> users;     // Set of users in the channel (using pointers to User objects)
        std::set<User*> operators; // Set of channel operators
        // Add other channel properties like modes (key, invite-only, moderated, etc.)
        // std::string key; // Example: channel key for +k mode
        // bool invite_only; // Example: flag for +i mode

    public:
        Channel(const std::string& channelName);
        ~Channel();
        const std::string& getName() const;
        const std::string& getTopic() const;
        void setTopic(const std::string& newTopic);
        bool addUser(User* user);
        bool removeUser(User* user);
        bool isUserInChannel(User* user) const;
        const std::set<User*>& getUsers() const;
        bool addOperator(User* user);
        bool removeOperator(User* user);
        bool isOperator(User* user) const;
        void broadcastMessage(const std::string& message, User* sender = nullptr) const;
        void setMode(const std::string& mode, User* user);
        void removeMode(const std::string& mode, User* user);
};

