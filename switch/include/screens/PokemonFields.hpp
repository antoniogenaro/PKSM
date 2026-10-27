#pragma once

#include "SwitchUi.hpp"
#include "screens/EditableField.hpp"
#include "utils/coretypes.h"
#include <string>
#include <vector>

namespace pksm
{
    class PKX;
    class Sav;
}

// The editable fields of one Pokémon and the box/party slot summaries, shared by the box
// browser and the Pokémon editor screens.
namespace pksm::switchui
{
    std::string numericValue(unsigned int value);

    // The editable fields of one Pokémon, grouped as the editor shows them. The fields keep
    // references to the Pokémon and the save.
    struct PokemonEditorFields
    {
        // The first page, in the canvas order, and the second ("More data").
        std::vector<EditableField> main;
        std::vector<EditableField> more;
        // HP, Attack, Defense, Sp. Atk, Sp. Def and Speed.
        std::vector<EditableField> ivs;
        std::vector<EditableField> evs;
        std::vector<EditableField> moves;
    };

    PokemonEditorFields pokemonEditorFields(pksm::PKX& pokemon, pksm::Sav& save);

    // What the editor's stats and moves panels show of `pokemon`.
    PokemonStats pokemonStats(const pksm::PKX& pokemon);

    // Writes the Pokémon back into its slot.
    void savePokemon(pksm::Sav& save, pksm::PKX& pokemon, bool party, u8 box, u8 slot);

    // A slot as the box grid shows it; a missing Pokémon is an empty slot.
    PokemonSlot pokemonSlot(const pksm::PKX* pokemon);
    // What the box screen's detail panel shows of `pokemon`.
    PokemonDetail describePokemon(const pksm::PKX& pokemon);
}
