#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace pksm::switchui
{
    // The supported saves on the SD card, grouped by game.
    struct SaveLibrary
    {
        // One list per entry of gameProfiles(), in the same order. Each holds the files of
        // that game, relative to the save folder and sorted.
        std::vector<std::vector<std::string>> filesByGame;
        // Set when the save folder could not be read.
        std::string error;

        // Reads the save folder. The identification cache spares reading files that did not
        // change since the last scan.
        static SaveLibrary scan();
    };
}
