#pragma once

#include "app/Screen.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace pksm::switchui
{
    // The supported saves found on the SD card.
    class SaveListScreen : public Screen
    {
    public:
        SaveListScreen();

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;

    private:
        std::vector<std::string> files_;
        std::vector<std::string> gameNames_;
        std::vector<size_t> gameMascots_;
        size_t selected_ = 0;
        std::string storageError_;
        // Set when the chosen save could not be opened; shown until A or B is pressed.
        std::string loadError_;
        bool showingLoadError_ = false;
    };
}
