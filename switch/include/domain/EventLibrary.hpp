#pragma once

#include "wcx/WC8.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace pksm::switchui
{
    // The Sword and Shield wonder cards of Project Pokémon's EventsGallery, packed into romfs by
    // switch/tools/pack_events.py: their names, the game each is for, and their data.
    class EventLibrary
    {
    public:
        // Which game a card was distributed for.
        enum class Game
        {
            Both,
            Sword,
            Shield
        };

        struct Entry
        {
            std::string name; // As the gallery names it, in English.
            Game game = Game::Both;
        };

        // Reads the cards. Returns nullptr, with a message in `error`, when they cannot be read.
        static std::unique_ptr<EventLibrary> load(std::string& error);

        size_t size() const { return entries_.size(); }
        const Entry& entry(size_t index) const { return entries_[index]; }
        // A copy of the card, to read or to give to the save.
        std::unique_ptr<pksm::WC8> card(size_t index) const;

    private:
        std::vector<Entry> entries_;
        std::vector<u8> data_;
    };
}
