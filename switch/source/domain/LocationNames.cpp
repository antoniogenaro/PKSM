#include "domain/LocationNames.hpp"

namespace pksm::switchui
{
    bool hasLocationNames(pksm::GameVersion origin, bool generationThree)
    {
        if (origin == pksm::GameVersion::S || origin == pksm::GameVersion::R ||
            origin == pksm::GameVersion::E || origin == pksm::GameVersion::FR ||
            origin == pksm::GameVersion::LG)
        {
            return generationThree;
        }
        return origin == pksm::GameVersion::GP || origin == pksm::GameVersion::GE ||
               origin == pksm::GameVersion::SW || origin == pksm::GameVersion::SH ||
               origin == pksm::GameVersion::PLA || origin == pksm::GameVersion::SL ||
               origin == pksm::GameVersion::VL || origin == pksm::GameVersion::ZA;
    }
}
