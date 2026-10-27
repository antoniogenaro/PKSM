#pragma once

#include "app/Screen.hpp"
#include <cstddef>
#include <memory>
#include <string>

namespace pksm::switchui
{
    class Session;

    // The sections available for an open save. Owns the save while it is open.
    class GameMenuScreen : public Screen
    {
    public:
        GameMenuScreen(std::unique_ptr<Session> session, std::string path);
        ~GameMenuScreen() override;

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;
        void onResume() override;
        bool ownsSession() const override { return true; }

    private:
        void open(App& app);

        std::unique_ptr<Session> session_;
        std::string path_;
        size_t selected_ = 0;
        std::string message_;
    };
}
