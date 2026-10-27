#pragma once

#include "SwitchUi.hpp"
#include "app/Screen.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace pksm::switchui
{
    class Session;

    // The boxes and the party of an open save. L/R pages through boxes, then the party.
    class BoxBrowserScreen : public Screen
    {
    public:
        BoxBrowserScreen(Session& session, std::string game, std::string path);

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;
        void onResume() override;

    private:
        void refreshSlots();
        void moveSelection(Action action);
        void openSelected(App& app);

        Session& session_;
        std::string game_;
        std::string path_;
        unsigned int maxBoxes_;
        unsigned int box_;
        bool party_      = false;
        size_t selected_ = 0;
        std::vector<Field> slots_;
        std::string message_;
    };
}
