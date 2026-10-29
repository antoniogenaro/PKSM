#pragma once

#include "SwitchUi.hpp"
#include "app/Screen.hpp"
#include "platform/ConsoleUsers.hpp"
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace pksm::switchui
{
    // The saves of one game, chosen in the game list: the selected user's console save, when
    // there is one, and then the backups on the SD card.
    class SaveListScreen : public Screen
    {
    public:
        // `game` is an index into gameProfiles(); `files` are its saves, relative to the save
        // folder. `user` is the selected console user, if any, and `consoleSave` whether they
        // have a save of the game.
        SaveListScreen(size_t game, std::vector<std::string> files,
            std::optional<ConsoleUser> user, bool consoleSave);

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;
        // Scans again, so a save changed from this game shows up as it is now.
        void onResume() override;

    private:
        // The rows: the console save, if any, and then the files.
        size_t rowCount() const;
        void open(App& app);

        size_t game_;
        GameCard card_;
        std::string group_;
        std::optional<ConsoleUser> user_;
        std::vector<std::string> files_;
        size_t selected_ = 0;
        std::string storageError_;
        // Set when the chosen save could not be opened; shown until A or B is pressed.
        std::string loadError_;
        bool showingLoadError_ = false;
    };
}
