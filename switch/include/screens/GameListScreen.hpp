#pragma once

#include "SwitchUi.hpp"
#include "app/SaveLibrary.hpp"
#include "app/Screen.hpp"
#include "domain/GameGrid.hpp"
#include <cstddef>
#include <vector>

namespace pksm::switchui
{
    // The first screen: every supported game, whether or not the SD card has saves of it.
    class GameListScreen : public Screen
    {
    public:
        GameListScreen();

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;
        // Scans again, so saves exported meanwhile are counted.
        void onResume() override;

    private:
        void refresh();

        GameGrid grid_;
        SaveLibrary library_;
        std::vector<GameCard> games_;
        size_t selected_ = 0;
    };
}
