#pragma once

#include "domain/Pokedex.hpp"
#include <string>

namespace pksm::switchui
{
    class PokemonBank;
}

// The global Pokédex's records on the SD card, beside the bank they come from.
namespace pksm::switchui::dex
{
    inline constexpr char RECORDS_PATH[] = "sdmc:/switch/pksm-banks/pokedex.json";

    // Reads the records. Without a file yet they are empty. Returns false, with a message in
    // `error`, when the file is there but cannot be read or is not a records file.
    bool loadRecords(Records& records, std::string& error);

    // Registers every Pokémon in `bank` that is not registered yet, on today's date, and writes
    // the records when that added any. Returns false, with a message in `error`, when the file
    // could not be read or written; it is then left as it was.
    bool registerBank(const PokemonBank& bank, std::string& error);

    // Today as "YYYY-MM-DD".
    std::string today();
}
