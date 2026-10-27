#pragma once

#include "SwitchUi.hpp"
#include "utils/coretypes.h"
#include <cstddef>
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
    enum class FieldKind
    {
        Species,
        Nickname,
        Level,
        Nature,
        Ability,
        Item,
        Shiny,
        Pokerus,
        OriginalTrainer,
        Gender,
        Friendship,
        Ball,
        Form,
        Egg,
        FatefulEncounter,
        MetLevel,
        MetDay,
        MetMonth,
        MetYear,
        MetLocation,
        EggDay,
        EggMonth,
        EggYear,
        EggLocation,
        Enjoyment,
        Fullness,
        IV,
        EV,
        Move,
        RelearnMove
    };

    struct EditorField
    {
        FieldKind kind;
        // Which stat or move slot, for IV, EV, Move and RelearnMove.
        size_t index;
        Field display;
    };

    std::string numericValue(unsigned int value);

    std::vector<EditorField> makeFields(const pksm::PKX& pokemon);

    // direction: -1 or 1 to step the value, 0 to open a picker or the keyboard.
    // Returns whether the Pokémon changed; `error` explains a rejected input.
    bool changeField(EditorField& field, pksm::PKX& pokemon, pksm::Sav& save, SwitchUi& ui,
        int direction, std::string& error);

    // Writes the Pokémon back into its slot.
    void savePokemon(pksm::Sav& save, pksm::PKX& pokemon, bool party, u8 box, u8 slot);

    std::string pokemonSummary(const pksm::PKX& pokemon);
    std::vector<Field> boxFields(pksm::Sav& save, u8 box);
    std::vector<Field> partyFields(pksm::Sav& save);
}
