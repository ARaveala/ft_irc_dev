#include "Channel.hpp"
#include "User.hpp"
#include <iostream>

Channel::Channel(const std::string& channelName) : name(channelName), topic("") {
    std::cout << "Channel '" << name << "' created." << std::endl;
}

Channel::~Channel() {
    std::cout << "Channel '" << name << "' destroyed." << std::endl;
}

const std::string& Channel::getName() const {
    return name;
}

const std::string& Channel::getTopic() const {
    return topic;
}

void Channel::setTopic(const std::string& newTopic) {
    topic = newTopic;
}

bool Channel::addUser(User* user) {
    // std::set::insert returns a pair: iterator to the element and a boolean indicating insertion
    auto result = users.insert(user);
    if (result.second) {
        if (user) std::cout << user->getNickname() << " joined channel " << name << std::endl;
    }
    return result.second; // Return true if insertion happened (user was not already there)
}

bool Channel::removeUser(User* user) {
    // std::set::erase returns the number of elements removed (0 or 1 for a set)
    size_t removed_count = users.erase(user);

    if (removed_count > 0) {
        // Also remove from operators if they were an operator
        operators.erase(user);
        if (user) std::cout << user->getNickname() << " left channel " << name << std::endl;
    }

    return removed_count > 0;
}

bool Channel::isUserInChannel(User* user) const {
    return users.count(user) > 0;
}

const std::set<User*>& Channel::getUsers() const {
    return users;
}

bool Channel::addOperator(User* user) {
    if (isUserInChannel(user)) {
        auto result = operators.insert(user);
        if (result.second) {
            if (user) std::cout << user->getNickname() << " is now an operator in " << name << std::endl;
        }
        return result.second;
    }
    return false; // User must be in the channel to be an operator
}

bool Channel::removeOperator(User* user) {
     size_t removed_count = operators.erase(user);
     if (removed_count > 0) {
        if (user) std::cout << user->getNickname() << " is no longer an operator in " << name << std::endl;
     }
     return removed_count > 0;
}

bool Channel::isOperator(User* user) const {
    return operators.count(user) > 0;
}

void Channel::broadcastMessage(const std::string& message, User* sender) const {
    std::cout << "Channel [" << name << "] Broadcast: " << message << std::endl;
    // In a real server:
    // for (User* user : users) {
    //     if (user && user != sender) { // Added null check for user pointer
    //         // Send message to user->socket
    //         // Example: send(user->socket_fd, message.c_str(), message.length(), 0);
    //     }
    // }
}

// Set mode definition (basic example)
void Channel::setMode(const std::string& mode, User* user) {
    // Added null check for user pointer before accessing nickname
    if (user) {
      std::cout << name << " mode " << mode << " set by " << user->getNickname() << "." << std::endl;
    } else {
       std::cout << name << " mode " << mode << " set." << std::endl;
    }
    // Add actual mode logic here (e.g., if mode == "+t", set a flag)
}

// Remove mode definition (basic example)
void Channel::removeMode(const std::string& mode, User* user) {
    // Added null check for user pointer before accessing nickname
    if (user) {
      std::cout << name << " mode " << mode << " removed by " << user->getNickname() << "." << std::endl;
    } else {
      std::cout << name << " mode " << mode << " removed." << std::endl;
    }
    // Add actual mode logic here
}