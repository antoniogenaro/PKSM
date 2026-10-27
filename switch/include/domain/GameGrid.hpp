#pragma once

#include "domain/GameProfile.hpp"
#include <cstddef>
#include <span>
#include <vector>

namespace pksm::switchui
{
    // How the game list lays out its games: games of one group share a card, and cards fill
    // rows of COLUMNS from left to right. Games are numbered in the order of the profiles.
    class GameGrid
    {
    public:
        static constexpr size_t COLUMNS = 4;

        struct Card
        {
            Text group;
            // The games on the card, as indices into the profiles.
            std::vector<size_t> games;
        };

        explicit GameGrid(std::span<const GameProfile> profiles);

        const std::vector<Card>& cards() const { return cards_; }
        size_t gameCount() const { return gameCount_; }

        // Where the selection goes from `game`. Left and right walk every game in order and
        // wrap around; up and down jump to the card above or below, keeping the position on
        // the card when it can, and stay put when there is no card there.
        size_t left(size_t game) const;
        size_t right(size_t game) const;
        size_t up(size_t game) const;
        size_t down(size_t game) const;

    private:
        struct Place
        {
            size_t card;
            size_t slot;
        };

        Place placeOf(size_t game) const;
        size_t toCard(size_t game, size_t card) const;

        std::vector<Card> cards_;
        size_t gameCount_ = 0;
    };
}
