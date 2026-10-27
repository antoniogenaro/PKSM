#include "domain/GameGrid.hpp"

#include <algorithm>

namespace pksm::switchui
{
    GameGrid::GameGrid(std::span<const GameProfile> profiles) : gameCount_(profiles.size())
    {
        for (size_t i = 0; i < profiles.size(); ++i)
        {
            if (cards_.empty() || cards_.back().group != profiles[i].group)
            {
                cards_.push_back(Card{profiles[i].group, {}});
            }
            cards_.back().games.push_back(i);
        }
    }

    size_t GameGrid::left(size_t game) const
    {
        if (gameCount_ == 0)
        {
            return 0;
        }
        return game == 0 || game >= gameCount_ ? gameCount_ - 1 : game - 1;
    }

    size_t GameGrid::right(size_t game) const
    {
        return gameCount_ == 0 || game + 1 >= gameCount_ ? 0 : game + 1;
    }

    size_t GameGrid::up(size_t game) const
    {
        const Place place = placeOf(game);
        return place.card < COLUMNS ? game : toCard(game, place.card - COLUMNS);
    }

    size_t GameGrid::down(size_t game) const
    {
        const Place place = placeOf(game);
        return place.card + COLUMNS >= cards_.size() ? game : toCard(game, place.card + COLUMNS);
    }

    GameGrid::Place GameGrid::placeOf(size_t game) const
    {
        for (size_t card = 0; card < cards_.size(); ++card)
        {
            const auto& games = cards_[card].games;
            const auto found  = std::find(games.begin(), games.end(), game);
            if (found != games.end())
            {
                return {card, static_cast<size_t>(found - games.begin())};
            }
        }
        // Not on the grid: no card can be above or below it.
        return {0, 0};
    }

    size_t GameGrid::toCard(size_t game, size_t card) const
    {
        const auto& games = cards_[card].games;
        if (games.empty())
        {
            return game;
        }
        return games[std::min(placeOf(game).slot, games.size() - 1)];
    }
}
