#include "domain/GameProfile.hpp"

#include "ui/Text.hpp"
#include <array>

namespace pksm::switchui
{
    namespace
    {
        // FireRed and LeafGreen are a title of their own in each language: Japanese, English,
        // French, German, Italian and Spanish.
        constexpr std::array<ConsoleTitle, 6> FIRE_RED_TITLES{{{0x01006FA0233F8000, "JP"},
            {0x0100554023408000, "EN"}, {0x01004B3023412000, "FR"}, {0x01007F8023416000, "DE"},
            {0x010092302342A000, "IT"}, {0x0100EB702342C000, "ES"}}};
        constexpr std::array<ConsoleTitle, 6> LEAF_GREEN_TITLES{{{0x0100F1E0233FA000, "JP"},
            {0x010034D02340E000, "EN"}, {0x010087C02342E000, "FR"}, {0x0100FD6023430000, "DE"},
            {0x01005C7023432000, "IT"}, {0x01002B5023434000, "ES"}}};
        constexpr std::array<ConsoleTitle, 1> LETS_GO_PIKACHU_TITLES{{{0x010003F003A34000}}};
        constexpr std::array<ConsoleTitle, 1> LETS_GO_EEVEE_TITLES{{{0x0100187003A36000}}};
        constexpr std::array<ConsoleTitle, 1> SWORD_TITLES{{{0x0100ABF008968000}}};
        constexpr std::array<ConsoleTitle, 1> SHIELD_TITLES{{{0x01008DB008C2C000}}};
        constexpr std::array<ConsoleTitle, 1> BRILLIANT_DIAMOND_TITLES{{{0x0100000011D90000}}};
        constexpr std::array<ConsoleTitle, 1> SHINING_PEARL_TITLES{{{0x010018E011D92000}}};
        constexpr std::array<ConsoleTitle, 1> LEGENDS_ARCEUS_TITLES{{{0x01001F5010DFA000}}};
        constexpr std::array<ConsoleTitle, 1> SCARLET_TITLES{{{0x0100A3D008C5C000}}};
        constexpr std::array<ConsoleTitle, 1> VIOLET_TITLES{{{0x01008F6008C5E000}}};
        constexpr std::array<ConsoleTitle, 1> LEGENDS_ZA_TITLES{{{0x0100F43008C44000}}};

        constexpr std::array<GameProfile, 12> PROFILES{{
            {pksm::GameVersion::FR, Text::GameFireRed, Text::GroupGen3Gba, 6, false, 999999, 7, 5,
                0xE23D28, FIRE_RED_TITLES, "/FireRed_s.sav", "FireRed"},
            {pksm::GameVersion::LG, Text::GameLeafGreen, Text::GroupGen3Gba, 3, false, 999999, 7,
                5, 0x4CAF50, LEAF_GREEN_TITLES, "/LeafGreen_s.sav", "LeafGreen"},
            {pksm::GameVersion::GP, Text::GameLetsGoPikachu, Text::GroupGen7LetsGo, 25, false,
                9999999, 12, 12, 0xF2C230, LETS_GO_PIKACHU_TITLES, "/savedata.bin",
                "Let's Go Pikachu"},
            {pksm::GameVersion::GE, Text::GameLetsGoEevee, Text::GroupGen7LetsGo, 133, false,
                9999999, 12, 12, 0xB5835A, LETS_GO_EEVEE_TITLES, "/savedata.bin",
                "Let's Go Eevee"},
            {pksm::GameVersion::SW, Text::GameSword, Text::GroupGen8SwordShield, 888, true,
                9999999, 12, 12, 0x2FA7E0, SWORD_TITLES, "/main", "Sword"},
            {pksm::GameVersion::SH, Text::GameShield, Text::GroupGen8SwordShield, 889, true,
                9999999, 12, 12, 0xD6405A, SHIELD_TITLES, "/main", "Shield"},
            {pksm::GameVersion::BD, Text::GameBrilliantDiamond, Text::GroupGen8DiamondPearl, 483,
                false, 999999, 12, 12, 0x4F86C6, BRILLIANT_DIAMOND_TITLES, "/SaveData.bin",
                "Brilliant Diamond"},
            {pksm::GameVersion::SP, Text::GameShiningPearl, Text::GroupGen8DiamondPearl, 484,
                false, 999999, 12, 12, 0xC77DA3, SHINING_PEARL_TITLES, "/SaveData.bin",
                "Shining Pearl"},
            {pksm::GameVersion::PLA, Text::GameLegendsArceus, Text::GroupGen8Legends, 493, false,
                9999999, 12, 12, 0xC9A94E, LEGENDS_ARCEUS_TITLES, "/main", "Legends Arceus"},
            {pksm::GameVersion::SL, Text::GameScarlet, Text::GroupGen9ScarletViolet, 1007, true,
                9999999, 12, 12, 0xD9382E, SCARLET_TITLES, "/main", "Scarlet",
                Text::TrainerLeaguePoints, 9999999},
            {pksm::GameVersion::VL, Text::GameViolet, Text::GroupGen9ScarletViolet, 1008, true,
                9999999, 12, 12, 0x8A4FBF, VIOLET_TITLES, "/main", "Violet",
                Text::TrainerLeaguePoints, 9999999},
            {pksm::GameVersion::ZA, Text::GameLegendsZA, Text::GroupGen9Legends, 718, false,
                9999999, 12, 12, 0x2E9E5B, LEGENDS_ZA_TITLES, "/main", "Legends ZA"},
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
