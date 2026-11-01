#pragma once

#include "SwitchUi.hpp"
#include "app/SaveLibrary.hpp"
#include "app/Screen.hpp"
#include "domain/GameGrid.hpp"
#include "platform/ConsoleUsers.hpp"
#include <cstddef>
#include <vector>

namespace pksm::switchui
{
    // The first screen: every supported game, whether or not the SD card or the selected
    // console user has saves of it. X chooses another console user; − or the header's button
    // opens the global Pokédex.
    class GameListScreen : public Screen
    {
    public:
        // `user` is the index in `users` of the user to start with.
        GameListScreen(std::vector<ConsoleUser> users, size_t user);

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;
        // Scans again, so saves changed meanwhile, or those of a newly chosen user, are
        // counted.
        void onResume() override;

    private:
        void refresh();
        const ConsoleUser* user() const;

        GameGrid grid_;
        std::vector<ConsoleUser> users_;
        size_t user_ = 0;
        SaveLibrary library_;
        std::vector<GameCard> games_;
        size_t selected_ = 0;
        // The header's Pokédex button was touched; update() opens it.
        bool pokedexRequested_ = false;
    };
}
