#pragma once

#define MAX_CLIENTS = 10;
#define TIMEOUT_CLIENT = 100;
//#define FAILURE = 1;
//#define SUCCESS = 0;

namespace errVal {
	constexpr int FAILURE = 1;
	constexpr int SUCCESS = 0;
}
/*
This is like global variables but its encapsulated in the Config, so its much harder to 
mix variables of the same name. This is however using namespace , 2 questions: 
1. do you want to learn them now when they are forbidden in the modules
2. are they forbidden in this project .
#ifndef CONFIG_H
#define CONFIG_H

#include <string>

namespace Config {
    constexpr int MAX_NUM = 0;
    constexpr const char* PRIV = "!???";

    // Example of a more complex configuration (bitmask permissions)
    enum Permissions {
        READ    = 1 << 0,
        WRITE   = 1 << 1,
        EXECUTE = 1 << 2
    };
}

#endif // CONFIG_H
this is used in code like this:
Config::MAX_NUM
*/
