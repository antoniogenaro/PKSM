#include "domain/GameProfile.hpp"

#include "ui/Text.hpp"
#include <array>

namespace pksm::switchui
{
    namespace
    {
        constexpr std::array<GameProfile, 4> PROFILES{{
            {pksm::GameVersion::SW, Text::GameSword, 888, true, 9999999, 12, 12},
            {pksm::GameVersion::SH, Text::GameShield, 889, true, 9999999, 12, 12},
            {pksm::GameVersion::FR, Text::GameFireRed, 6, false, 999999, 7, 5},
            {pksm::GameVersion::LG, Text::GameLeafGreen, 3, false, 999999, 7, 5},
        }};
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
