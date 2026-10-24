#pragma once

#include <functional>
#include <string>

namespace pksm
{
    class Sav;
}

namespace pksm::switchui
{
    class SwitchUi;

    enum class PokemonEditorResult
    {
        Back,
        Discard,
        Quit
    };

    PokemonEditorResult runPokemonEditor(Sav& save, SwitchUi& ui, const std::string& game,
        const std::string& path, bool& dirty, std::string& error,
        const std::function<bool(std::string&)>& exportChanges);
}
