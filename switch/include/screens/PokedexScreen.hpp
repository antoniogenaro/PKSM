#pragma once

#include "app/Screen.hpp"
#include "domain/Pokedex.hpp"
#include "enums/GameVersion.hpp"
#include "enums/Language.hpp"
#include "enums/Species.hpp"
#include "SwitchUi.hpp"
#include <array>
#include <optional>
#include <string>
#include <vector>

namespace pksm::switchui
{
    // The global Pokédex: what the user has registered by saving Pokémon in the bank, game by
    // game and form by form. ZL/ZR change the game, X and Y the filters, L/R the page.
    class PokedexScreen : public Screen
    {
    public:
        PokedexScreen();

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;

    private:
        // The chosen game, or nothing for Global.
        std::optional<dex::Game> game() const;
        // Lists the cells again for the game and filters, keeping the focus on the same Pokémon
        // when it is still listed.
        void refresh();
        void moveFocus(Action action);
        // The game whose names and sprites show `species`: the chosen one, or for Global the
        // newest game it appears in.
        dex::Game shownGame(uint16_t species) const;
        void fillFocus(PokedexView& view) const;
        // The form's name, or one made up when core has none: the species', "Macho" or
        // "Hembra", or "Forma 2".
        std::string formName(pksm::Species species, uint8_t form, pksm::GameVersion version) const;
        // The species' name, in English when the language has none for it.
        std::string speciesName(pksm::Species species) const;

        dex::Records records_;
        std::string error_;
        pksm::Language language_;
        // 0 for Global, then the games in dex::games() order.
        size_t game_              = 0;
        dex::RecordFilter record_ = dex::RecordFilter::All;
        dex::FormFilter form_     = dex::FormFilter::Species;
        std::vector<dex::Cell> cells_;
        size_t selected_ = 0;
        // Each game's progress, Global first, for the form filter in use.
        std::vector<dex::Progress> progress_;
    };
}
