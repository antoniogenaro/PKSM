#pragma once

#include "utils/coretypes.h"
#include <cstddef>
#include <memory>

namespace pksm
{
    class Sav;
}

// Let's Go keeps its Pokémon in one list of 1000 slots, shown here as boxes of 30, and its
// party is a list of six of those slots. These helpers keep the party pointing at its
// Pokémon when box slots change, and do nothing for the other games.
namespace pksm::switchui::letsgo
{
    // Whether the box slot is one the save has: the last of Let's Go's 34 boxes has only 10.
    bool slotExists(const pksm::Sav& save, int box, size_t slot);

    // The party place (1 to 6) of the Pokémon in a box slot, or 0 when it is not in the party.
    size_t partyPlace(const pksm::Sav& save, int box, size_t slot);

    // After the Pokémon of two box slots traded places: the party and the Pokémon that follows
    // the player go with them.
    void swapped(pksm::Sav& save, int boxA, size_t slotA, int boxB, size_t slotB);

    // Party places whose box slot is now empty leave the party, which closes up.
    void dropEmptyPartyPlaces(pksm::Sav& save);

    // The bytes to write for a Let's Go save: a copy with its Pokémon packed to the front of
    // the list, as the game keeps them, and its checksums fixed. The save in memory keeps its
    // layout, so no screen loses track of a slot. nullptr for the other games.
    std::shared_ptr<u8[]> packedForWriting(const pksm::Sav& save);
}
