#pragma once

#include "enums/GameVersion.hpp"
#include "enums/Language.hpp"
#include "ui/Text.hpp"
#include <cstddef>
#include <cstdint>
#include <span>

namespace pksm::switchui
{
    // Everything the Switch app needs to know about one supported game. Adding a game
    // means adding one row to the table in GameProfile.cpp.
    struct GameProfile
    {
        pksm::GameVersion version;
        Text name;
        // The card the game shares with its pair in the game list, such as "GEN III · GBA".
        Text group;
        // Species shown next to the game's saves.
        size_t mascotSpecies;
        bool hasBattlePoints;
        uint32_t maxMoney;
        size_t trainerNameLength;
        size_t trainerNameLengthJapanese;
        // The background of the game's art on its card, as 0xRRGGBB.
        unsigned artColor;
        // The game's title on the Switch, whose saves are kept on the console, and the save
        // file inside them. 0 for games that are not Switch games.
        uint64_t consoleTitleId     = 0;
        const char* consoleSaveFile = "";
        // The SD folder that gets a backup of a console save before it is replaced.
        const char* backupFolder = "";

        bool hasConsoleSaves() const { return consoleTitleId != 0; }

        size_t trainerNameLengthFor(pksm::Language language) const
        {
            return language == pksm::Language::JPN ? trainerNameLengthJapanese
                                                   : trainerNameLength;
        }
    };

    // Every supported game, in the order the game list shows them. Games of one group are
    // next to each other.
    std::span<const GameProfile> gameProfiles();

    // Returns nullptr when the Switch app does not support the game.
    const GameProfile* findGameProfile(pksm::GameVersion version);
}
