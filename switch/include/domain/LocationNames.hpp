#pragma once

#include "enums/GameVersion.hpp"
#include "enums/Language.hpp"
#include <string>
#include <string_view>
#include <vector>

// The names of the places where Pokémon are met or hatched, from PKHeX's lists, which
// switch/tools/import_pkhex_text.py writes to switch/i18n/<language>/locations.
namespace pksm::switchui
{
    struct LocationName
    {
        unsigned int number = 0;
        std::string name;
    };

    // The lines of a list, "number<TAB>name" each, in their order. Lines that are not so are
    // left out.
    std::vector<LocationName> parseLocationNames(std::string_view text);

    // The file of the list a Pokémon from `origin` is named by: the game's own list, as each
    // keeps its numbers. Empty for the games without one, and for those of generation III when
    // the Pokémon has left it (`generationThree` false), as the numbers then change.
    std::string_view locationListFile(pksm::GameVersion origin, bool generationThree);

    // The places of `origin`'s list in `language`, or in English when that language lacks the
    // list. Empty when there is no list. `root` is the folder holding a folder per language.
    const std::vector<LocationName>& locationNames(pksm::Language language,
        pksm::GameVersion origin, bool generationThree,
        std::string_view root = "romfs:/i18n/");

    // The name of place `number` in `names`, or empty.
    std::string_view locationName(const std::vector<LocationName>& names, unsigned int number);
}
