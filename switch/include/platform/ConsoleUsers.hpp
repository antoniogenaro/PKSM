#pragma once

#include <switch.h>
#include <cstddef>
#include <string>
#include <vector>

// The users of the console. Each has their own console saves.
namespace pksm::switchui
{
    struct ConsoleUser
    {
        AccountUid uid{};
        std::string name;
    };

    // Every user of the console, in the system's order. Empty when the console could not say.
    std::vector<ConsoleUser> listConsoleUsers();

    // The index in `users` of the user the app was started for, or of the last one who
    // played, or 0.
    size_t startingUserIndex(const std::vector<ConsoleUser>& users);
}
