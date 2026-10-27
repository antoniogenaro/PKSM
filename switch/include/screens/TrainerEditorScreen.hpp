#pragma once

#include "app/Screen.hpp"
#include "screens/EditableField.hpp"
#include <cstddef>
#include <string>
#include <vector>

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
        std::vector<EditableField> fields();

        Session& session_;
        std::string game_;
        std::string path_;
        size_t selected_ = 0;
        std::string message_;
    };
}
