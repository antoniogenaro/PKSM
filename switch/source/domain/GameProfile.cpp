#include "domain/GameProfile.hpp"

#include "ui/Text.hpp"
#include <array>

namespace pksm::switchui
{
    namespace
    {
        constexpr std::array<GameProfile, 9> PROFILES{{
            {pksm::GameVersion::FR, Text::GameFireRed, Text::GroupGen3Gba, 6, false, 999999, 7, 5,
                0xE23D28},
            {pksm::GameVersion::LG, Text::GameLeafGreen, Text::GroupGen3Gba, 3, false, 999999, 7,
                5, 0x4CAF50},
            {pksm::GameVersion::GP, Text::GameLetsGoPikachu, Text::GroupGen7LetsGo, 25, false,
                9999999, 12, 12, 0xF2C230, 0x010003F003A34000, "/savedata.bin",
                "Let's Go Pikachu"},
            {pksm::GameVersion::GE, Text::GameLetsGoEevee, Text::GroupGen7LetsGo, 133, false,
                9999999, 12, 12, 0xB5835A, 0x0100187003A36000, "/savedata.bin",
                "Let's Go Eevee"},
            {pksm::GameVersion::SW, Text::GameSword, Text::GroupGen8SwordShield, 888, true,
                9999999, 12, 12, 0x2FA7E0, 0x0100ABF008968000, "/main", "Sword"},
            {pksm::GameVersion::SH, Text::GameShield, Text::GroupGen8SwordShield, 889, true,
                9999999, 12, 12, 0xD6405A, 0x01008DB008C2C000, "/main", "Shield"},
            {pksm::GameVersion::PLA, Text::GameLegendsArceus, Text::GroupGen8Legends, 493, false,
                9999999, 12, 12, 0xC9A94E, 0x01001F5010DFA000, "/main", "Legends Arceus"},
            {pksm::GameVersion::SL, Text::GameScarlet, Text::GroupGen9ScarletViolet, 1007, true,
                9999999, 12, 12, 0xD9382E, 0x0100A3D008C5C000, "/main", "Scarlet",
                Text::TrainerLeaguePoints, 9999999},
            {pksm::GameVersion::VL, Text::GameViolet, Text::GroupGen9ScarletViolet, 1008, true,
                9999999, 12, 12, 0x8A4FBF, 0x01008F6008C5E000, "/main", "Violet",
                Text::TrainerLeaguePoints, 9999999},
        }};
    }

    std::span<const GameProfile> gameProfiles()
    {
        return PROFILES;
    }

    const GameProfile* findGameProfile(pksm::GameVersion version)
    {
        for (const auto& profile : PROFILES)
        {
            if (profile.version == version)
            {
                return &profile;
            }
        }
        return nullptr;
    }
}
