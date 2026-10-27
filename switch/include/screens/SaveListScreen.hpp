#pragma once

#include "SwitchUi.hpp"
#include "app/Screen.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace pksm::switchui
{
    // The saves of one game, chosen in the game list.
    class SaveListScreen : public Screen
    {
    public:
        // `game` is an index into gameProfiles(); `files` are its saves, relative to the save
        // folder.
        SaveListScreen(size_t game, std::vector<std::string> files);

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;
        // Scans again, so a save exported from this game shows up.
        void onResume() override;

    private:
        size_t game_;
        GameCard card_;
        std::string group_;
        std::vector<std::string> files_;
        size_t selected_ = 0;
        std::string storageError_;
        // Set when the chosen save could not be opened; shown until A or B is pressed.
        std::string loadError_;
        bool showingLoadError_ = false;
    };
}
