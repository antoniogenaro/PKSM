#include "domain/GameProfile.hpp"

#include <array>

namespace pksm::switchui
{
    namespace
    {
        constexpr std::array<GameProfile, 4> PROFILES{{
            {pksm::GameVersion::SW, "Pokemon Espada", 888, true, 9999999, 12, 12},
            {pksm::GameVersion::SH, "Pokemon Escudo", 889, true, 9999999, 12, 12},
            {pksm::GameVersion::FR, "Pokemon Rojo Fuego", 6, false, 999999, 7, 5},
            {pksm::GameVersion::LG, "Pokemon Verde Hoja", 3, false, 999999, 7, 5},
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
