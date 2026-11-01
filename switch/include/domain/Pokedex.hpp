#pragma once

#include "enums/GameVersion.hpp"
#include "enums/Generation.hpp"
#include "enums/Type.hpp"
#include "ui/Text.hpp"
#include <compare>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The global Pokédex: which Pokémon the user has registered, by form and by game. A Pokémon is
// registered when it is in the bank and the bank is saved. Each game groups its two versions.
namespace pksm::switchui::dex
{
    // The games of the Pokédex, in the order it lists them.
    enum class Game : uint8_t
    {
        FireRedLeafGreen,
        LetsGo,
        SwordShield,
        BrilliantDiamondShiningPearl,
        LegendsArceus,
        ScarletViolet,
        LegendsZA,
        Count
    };
    inline constexpr size_t GAME_COUNT = static_cast<size_t>(Game::Count);

    // Every game, in the Pokédex's order.
    std::vector<Game> games();
    // "Escarlata / Púrpura".
    Text gameName(Game game);
    // The name the records file uses for the game, such as "sv".
    const char* gameKey(Game game);
    std::optional<Game> gameFromKey(std::string_view key);
    // The generation of the game's Pokémon, which picks their form sprites.
    pksm::Generation gameGeneration(Game game);
    // A version of the game, whose tables PKSM uses for it (and for its form names).
    pksm::GameVersion gameVersion(Game game);

    // The game a Pokémon stored in the bank counts for: the game whose format it is in. Nothing
    // for formats of games the Pokédex does not list.
    std::optional<Game> gameOf(pksm::Generation generation, pksm::GameVersion version);

    // The species that appear in the game, in the order of its regional Pokédex when PKSM knows
    // it (Sword and Shield), and in national order otherwise. Species of no regional Pokédex
    // follow in national order.
    const std::vector<uint16_t>& gameSpecies(Game game);
    bool appears(Game game, uint16_t species);
    // The forms of `species` that appear in the game, by number, and how many: none when the
    // species does not appear.
    const std::vector<uint8_t>& forms(Game game, uint16_t species);
    size_t formCount(Game game, uint16_t species);
    bool hasForm(Game game, uint16_t species, size_t form);

    // Every species that appears in some game, in national order, and the forms that appear in
    // any game.
    const std::vector<uint16_t>& allSpecies();
    std::vector<uint8_t> globalForms(uint16_t species);

    // The species' types, as the newest game it appears in has them. The second is the same as
    // the first for single-typed species.
    std::pair<pksm::Type, pksm::Type> types(uint16_t species);

    // One registered form.
    struct Key
    {
        Game game;
        uint16_t species;
        uint8_t form;
        bool shiny;

        auto operator<=>(const Key&) const = default;
    };

    // What has been registered, with the day each was registered first ("2026-11-01").
    class Records
    {
    public:
        // Adds `key` unless it is there already. Returns true when it was new.
        bool add(const Key& key, const std::string& date);

        bool has(const Key& key) const { return entries_.contains(key); }

        // The day `key` was first registered, or nullptr when it is not.
        const std::string* date(const Key& key) const;

        size_t size() const { return entries_.size(); }

        // The records file: {"version": 1, "entries": [{"game": "sv", "species": 906,
        // "form": 0, "shiny": false, "date": "2026-11-01"}, ...]}.
        std::string toJson() const;
        // Reads a records file. Entries it cannot use are skipped. Returns nothing when it is not
        // a records file at all.
        static std::optional<Records> fromJson(std::string_view json);

    private:
        std::map<Key, std::string> entries_;
    };

    // The "Registro" filter.
    enum class RecordFilter : uint8_t
    {
        All,
        Registered,
        Missing,
        Count
    };

    // The "Forma" filter: a cell per species, per shiny form, or per form.
    enum class FormFilter : uint8_t
    {
        Species,
        Shiny,
        AllForms,
        Count
    };

    Text recordFilterName(RecordFilter filter);
    Text formFilterName(FormFilter filter);

    // One cell of the grid.
    struct Cell
    {
        uint16_t species = 0;
        uint8_t form     = 0;
        bool shiny       = false;
        bool registered  = false;
        // With FormFilter::Species, the species' forms and how many are registered; the cell
        // shows "registered/forms" when there is more than one.
        size_t forms           = 1;
        size_t formsRegistered = 0;
    };

    // The cells of `game`, or of every game when it is nothing, in the order the grid shows them.
    std::vector<Cell> cells(
        const Records& records, std::optional<Game> game, RecordFilter record, FormFilter form);

    struct Progress
    {
        size_t registered = 0;
        size_t total      = 0;

        bool complete() const { return total > 0 && registered >= total; }

        // Rounded down, so only a complete game shows 100.
        unsigned percent() const;
    };

    // How much of `game` is registered, counted as `form` says: species, shiny forms or forms.
    // Without a game, the sums over every game.
    Progress progress(const Records& records, std::optional<Game> game, FormFilter form);

    // Whether any non-shiny form (or shiny form, with `shiny`) of `species` is registered in
    // `game`, and how many of its forms are.
    Progress speciesProgress(const Records& records, Game game, uint16_t species, bool shiny);
    // The same for the form `form`, in any game.
    bool registeredAnywhere(const Records& records, uint16_t species, uint8_t form, bool shiny);
}
