#pragma once

#include "app/Screen.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace pksm::switchui
{
    class SwitchUi;

    // The main loop and the stack of screens. Requests made while a screen is updating are
    // queued and applied once it returns.
    class App
    {
    public:
        explicit App(SwitchUi& ui) : ui_(ui) {}
        App(const App&)            = delete;
        App& operator=(const App&) = delete;

        SwitchUi& ui() { return ui_; }

        void push(std::unique_ptr<Screen> screen);
        void pop();
        // Closes every screen of the open save, back to the save list.
        void closeSession();
        void quit();

        // Runs until quit() is called, the stack empties or the system asks the app to close.
        void run();

    private:
        void applyPending();

        SwitchUi& ui_;
        std::vector<std::unique_ptr<Screen>> stack_;
        std::vector<std::unique_ptr<Screen>> pendingPushes_;
        size_t pendingPops_ = 0;
        bool quitRequested_ = false;
    };
}
