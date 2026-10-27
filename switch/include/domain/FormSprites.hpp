#pragma once

#include <cstddef>

namespace pksm
{
    class PKX;
}

// Which sprite of the types sheet shows a Pokémon's alternate form, as the 3DS app picks it.
namespace pksm::switchui
{
    // The types-sheet index of `pokemon`'s form (or female) sprite, or 0 when the base sprite
    // of the Pokémon sheet shows it.
    size_t formSpriteIndex(const pksm::PKX& pokemon);
}
