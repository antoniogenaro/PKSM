#pragma once

#include "BankFile.hpp"
#include <memory>
#include <string>
#include <vector>

namespace pksm
{
    class PKX;
}

namespace pksm::switchui
{
    // The PKSM bank on the SD card: a .bnk file in the 3DS format (common/BankFile) and a .json
    // file with the box names beside it, so a bank copied from a 3DS opens here too. Changes stay
    // in memory until save().
    class PokemonBank
    {
    public:
        static constexpr char DIRECTORY[]    = "sdmc:/switch/pksm-banks";
        static constexpr char DEFAULT_NAME[] = "pksm_1";
        static constexpr int DEFAULT_BOXES   = 50;
        static constexpr int SLOTS           = BankFile::SLOTS_PER_BOX;

        // Reads the default bank, or starts an empty one when there is no file yet. Returns
        // nullptr, with a message in `error`, when the file is there but cannot be used: it is
        // then left untouched.
        static std::unique_ptr<PokemonBank> open(std::string& error);

        int boxes() const { return static_cast<int>(entries_.size()) / SLOTS; }
        std::string boxName(int box) const;
        // The .bnk file's name, as the screen shows it.
        std::string fileName() const;

        bool empty(int box, int slot) const;
        // The Pokémon in a slot, or nullptr when it is empty or its data cannot be read.
        std::unique_ptr<pksm::PKX> pkm(int box, int slot) const;
        // Stores `pokemon` as it is, in its own generation. A Pokémon without a species empties
        // the slot.
        void pkm(const pksm::PKX& pokemon, int box, int slot);
        void clear(int box, int slot);

        bool dirty() const { return dirty_; }

        // Writes both files. The old ones are kept as .bak, and each file is written aside first,
        // so a failed write never leaves half a bank behind.
        bool save(std::string& error);

    private:
        PokemonBank() = default;
        std::string bankPath() const;
        std::string namesPath() const;

        std::vector<BankFile::Entry> entries_;
        std::vector<std::string> boxNames_;
        bool dirty_ = false;
    };
}
