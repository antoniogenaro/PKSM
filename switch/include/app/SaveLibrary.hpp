#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace pksm::switchui
{
    struct ConsoleUser;

    // The supported saves on the SD card, grouped by game, and which games the selected
    // console user has a save of.
    struct SaveLibrary
    {
        // One list per entry of gameProfiles(), in the same order. Each holds the files of
        // that game: those in the save folder relative to it and sorted, then the backups of
        // JKSV and Checkpoint as full paths (see storage::savePath).
        std::vector<std::vector<std::string>> filesByGame;
        // One flag per entry of gameProfiles(): whether the user has a console save of it.
        std::vector<bool> consoleSaveByGame;
        // Set when the save folder could not be read.
        std::string error;

        // Reads the save folder, and the console saves of `user` when there is one. The
        // identification cache spares reading files that did not change since the last scan.
        static SaveLibrary scan(const ConsoleUser* user);
    };
}
