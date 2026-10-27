#pragma once

#include "enums/GameVersion.hpp"
#include "enums/Language.hpp"
#include "ui/Text.hpp"
#include <cstddef>
#include <cstdint>

namespace pksm::switchui
{
    // Everything the Switch app needs to know about one supported game. Adding a game
    // means adding one row to the table in GameProfile.cpp.
    struct GameProfile
    {
        pksm::GameVersion version;
        Text name;
        // Species shown next to the game's saves.
        size_t mascotSpecies;
        bool hasBattlePoints;
        uint32_t maxMoney;
        size_t trainerNameLength;
        size_t trainerNameLengthJapanese;

        size_t trainerNameLengthFor(pksm::Language language) const
        {
            return language == pksm::Language::JPN ? trainerNameLengthJapanese
                                                   : trainerNameLength;
        }
    };

    // Returns nullptr when the Switch app does not support the game.
    const GameProfile* findGameProfile(pksm::GameVersion version);
}
