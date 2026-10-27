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

    // Every editable field of `pokemon`, in display order. The fields keep references to
    // `pokemon` and `save`.
    std::vector<EditableField> pokemonFields(pksm::PKX& pokemon, pksm::Sav& save);

    // Writes the Pokémon back into its slot.
    void savePokemon(pksm::Sav& save, pksm::PKX& pokemon, bool party, u8 box, u8 slot);

    std::string pokemonSummary(const pksm::PKX& pokemon);
    std::vector<Field> boxFields(pksm::Sav& save, u8 box);
    std::vector<Field> partyFields(pksm::Sav& save);
}
