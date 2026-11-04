#pragma once

#include "enums/GameVersion.hpp"

// The names of the places where Pokémon are met or hatched come from core's
// i18n::rawLocations(language, version).
namespace pksm::switchui
{
    // Whether a Pokémon from the game `origin` gets its met and egg locations picked by name.
    // Generation III Pokémon do only while still in generation III's format; elsewhere their
    // numbers name no place in any list.
    bool hasLocationNames(pksm::GameVersion origin, bool generationThree);
}
