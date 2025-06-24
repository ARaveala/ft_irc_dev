#pragma once

#include <string>
#include <iostream>

// Example user class.
class User {
public:
    std::string nickname;
    ~User();
    User(const std::string& nick);
    bool operator<(const User& other) const;
    bool operator==(const User& other) const;
    const std::string& getNickname() const;
};