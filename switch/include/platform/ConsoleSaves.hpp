#pragma once

#include "platform/ConsoleUsers.hpp"
#include "utils/coretypes.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

// The saves the games keep on the console, one per game and user. Errors are returned as
// user-facing messages.
namespace pksm::switchui::console
{
    // Where one game keeps a user's save.
    struct SaveLocation
    {
        uint64_t titleId = 0;
        // The save file inside the game's save data, such as "/main".
        std::string file;
        ConsoleUser user;
        // The folder under the SD save folder that gets a backup before each write, such as
        // "Sword".
        std::string backupFolder;
    };

    // For each title, whether `user` has a save of it on the console. Reading the list does
    // not open the saves, so it works while a game is running.
    std::vector<bool> savesPresent(std::span<const uint64_t> titleIds, const ConsoleUser& user);

    bool readSave(const SaveLocation& location, std::shared_ptr<u8[]>& data, size_t& size,
        std::string& error);

    // Copies the save as it is now to "<SD save folder>/<backupFolder>/<user> - <date>/" and
    // then replaces it with `data`. Nothing is written to the console when the copy fails.
    bool writeSave(const SaveLocation& location, const u8* data, size_t size,
        std::string& backupPath, std::string& error);
}
