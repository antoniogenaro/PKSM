#pragma once

#include "app/Screen.hpp"
#include <cstddef>
#include <string>

namespace pksm::switchui
{
    class Session;

    // Trainer name, IDs, money, BP and play time.
    class TrainerEditorScreen : public Screen
    {
    public:
        TrainerEditorScreen(Session& session, std::string game, std::string path);

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;

    private:
        enum class Row
        {
            Name,
            TID,
            SID,
            Money,
            BP,
            Hours,
            Minutes,
            Seconds
        };

        Row rowAt(size_t index) const;
        size_t rowCount() const;
        // Moves the value on the selected row one step. Returns whether it changed.
        bool step(Row row, bool increase);

        Session& session_;
        std::string game_;
        std::string path_;
        size_t selected_ = 0;
        std::string message_;
    };
}
