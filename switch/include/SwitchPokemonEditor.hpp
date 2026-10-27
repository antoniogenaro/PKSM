#pragma once

#include <string>

namespace pksm::switchui
{
    class Session;
    class SwitchUi;

    enum class PokemonEditorResult
    {
        Back,
        Discard,
        Quit
    };

    PokemonEditorResult runPokemonEditor(Session& session, SwitchUi& ui, const std::string& game,
        const std::string& path, std::string& error);
}
