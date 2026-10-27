#pragma once

#include "app/Screen.hpp"
#include "utils/coretypes.h"
#include <cstddef>
#include <memory>
#include <string>

namespace pksm
{
    class PKX;
}

namespace pksm::switchui
{
    class Session;

    // Every editable field of one Pokémon. Each change is written back to its slot at once.
    class PokemonEditorScreen : public Screen
    {
    public:
        PokemonEditorScreen(Session& session, std::string game, std::string path,
            std::unique_ptr<pksm::PKX> pokemon, bool party, u8 box, u8 slot);
        ~PokemonEditorScreen() override;

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;

    private:
        Session& session_;
        std::string game_;
        std::string path_;
        std::unique_ptr<pksm::PKX> pokemon_;
        bool party_;
        u8 box_;
        u8 slot_;
        size_t selected_ = 0;
        std::string message_;
    };
}
