#include "User.hpp"

User::User(const std::string& nick) : nickname(nick) {
    std::cout << "User '" << nickname << "' created." << std::endl; // Optional: for debugging
}

bool User::operator<(const User& other) const {
    return nickname < other.nickname;
}

bool User::operator==(const User& other) const {
    return nickname == other.nickname;
}

const std::string& User::getNickname() const {
    return nickname;
}

User::~User() {
    std::cout << "User '" << nickname << "' destroyed." << std::endl;
}