#include "domain/GameProfile.hpp"

#include "ui/Text.hpp"
#include <array>

namespace pksm::switchui
{
    namespace
    {
        constexpr std::array<GameProfile, 4> PROFILES{{
            {pksm::GameVersion::FR, Text::GameFireRed, Text::GroupGen3Gba, 6, false, 999999, 7, 5,
                0xE23D28},
            {pksm::GameVersion::LG, Text::GameLeafGreen, Text::GroupGen3Gba, 3, false, 999999, 7,
                5, 0x4CAF50},
            {pksm::GameVersion::SW, Text::GameSword, Text::GroupGen8SwordShield, 888, true,
                9999999, 12, 12, 0x2FA7E0},
            {pksm::GameVersion::SH, Text::GameShield, Text::GroupGen8SwordShield, 889, true,
                9999999, 12, 12, 0xD6405A},
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
