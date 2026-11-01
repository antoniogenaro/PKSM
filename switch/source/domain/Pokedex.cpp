#include "domain/Pokedex.hpp"

#include "enums/Species.hpp"
#include "nlohmann/json.hpp"
#include "personal/personal.hpp"
#include "utils/VersionTables.hpp"
#include <algorithm>
#include <array>
#include <set>

namespace pksm::switchui::dex
{
    namespace
    {
        // The version whose tables describe each game.
        pksm::GameVersion tableVersion(Game game)
        {
            switch (game)
            {
                case Game::FireRedLeafGreen:
                    return pksm::GameVersion::FR;
                case Game::LetsGo:
                    return pksm::GameVersion::GP;
                case Game::SwordShield:
                    return pksm::GameVersion::SW;
                case Game::BrilliantDiamondShiningPearl:
                    // PKSM has no tables of its own for the remakes; they hold every species
                    // and form of Platinum.
                    return pksm::GameVersion::Pt;
                case Game::LegendsArceus:
                    return pksm::GameVersion::PLA;
                case Game::ScarletViolet:
                    return pksm::GameVersion::SL;
                case Game::LegendsZA:
                case Game::Count:
                    break;
            }
            return pksm::GameVersion::ZA;
        }

        // The row of a form in a personal table: the species' own row for its first form.
        template <typename StatIndex>
        uint16_t formRow(uint16_t species, size_t form, StatIndex statIndex)
        {
            if (form == 0)
            {
                return species;
            }
            const uint16_t first = statIndex(species);
            return first == 0 ? species : static_cast<uint16_t>(first + form - 1);
        }

        std::vector<uint8_t> listForms(Game game, uint16_t species)
        {
            const size_t count = std::clamp<size_t>(
                pksm::VersionTables::formCount(tableVersion(game), pksm::Species{species}), 1, 255);
            // Scarlet, Violet and Legends: Z-A hold rows for forms that are not in the game.
            const auto present = [&](auto presentInGame, auto statIndex, size_t form)
            { return form == 0 || presentInGame(formRow(species, form, statIndex)); };
            std::vector<uint8_t> result;
            for (size_t form = 0; form < count; ++form)
            {
                if ((game == Game::ScarletViolet && !present(pksm::PersonalSV::isPresentInGame,
                                                        pksm::PersonalSV::formStatIndex, form)) ||
                    (game == Game::LegendsZA && !present(pksm::PersonalZA::isPresentInGame,
                                                    pksm::PersonalZA::formStatIndex, form)))
                {
                    continue;
                }
                result.push_back(static_cast<uint8_t>(form));
            }
            return result;
        }

        struct GameTable
        {
            std::vector<uint16_t> order;
            // By species number: the forms that appear, none when the species does not.
            std::vector<std::vector<uint8_t>> forms;
        };

        GameTable buildTable(Game game)
        {
            GameTable table;
            const auto& available = pksm::VersionTables::availableSpecies(tableVersion(game));
            for (const pksm::Species species : available)
            {
                const auto number = static_cast<uint16_t>(u16(species));
                if (number == 0)
                {
                    continue;
                }
                if (game == Game::ScarletViolet && !pksm::PersonalSV::isPresentInGame(number))
                {
                    continue;
                }
                if (game == Game::LegendsZA && !pksm::PersonalZA::isPresentInGame(number))
                {
                    continue;
                }
                table.order.push_back(number);
            }
            std::sort(table.order.begin(), table.order.end());

            if (game == Game::SwordShield)
            {
                // Galar first, then the Isle of Armor and the Crown Tundra, then the rest.
                const auto rank = [](uint16_t species) -> std::pair<int, int>
                {
                    if (const u16 galar = pksm::PersonalSWSH::pokedexIndex(species))
                    {
                        return {0, galar};
                    }
                    if (const u16 armor = pksm::PersonalSWSH::armordexIndex(species))
                    {
                        return {1, armor};
                    }
                    if (const u16 crown = pksm::PersonalSWSH::crowndexIndex(species))
                    {
                        return {2, crown};
                    }
                    return {3, species};
                };
                std::stable_sort(table.order.begin(), table.order.end(),
                    [&](uint16_t a, uint16_t b) { return rank(a) < rank(b); });
            }

            const uint16_t last =
                table.order.empty() ? 0 : *std::max_element(table.order.begin(), table.order.end());
            table.forms.assign(size_t(last) + 1, {});
            for (const uint16_t species : table.order)
            {
                table.forms[species] = listForms(game, species);
            }
            return table;
        }

        const GameTable& table(Game game)
        {
            static const std::array<GameTable, GAME_COUNT> tables = []
            {
                std::array<GameTable, GAME_COUNT> result;
                for (size_t i = 0; i < GAME_COUNT; ++i)
                {
                    result[i] = buildTable(static_cast<Game>(i));
                }
                return result;
            }();
            return tables[static_cast<size_t>(game)];
        }

        constexpr std::array<const char*, GAME_COUNT> KEYS = {
            "frlg", "lgpe", "swsh", "bdsp", "pla", "sv", "za"};

        bool hasAnyForm(const Records& records, Game game, uint16_t species, bool shiny)
        {
            for (const uint8_t form : forms(game, species))
            {
                if (records.has({game, species, form, shiny}))
                {
                    return true;
                }
            }
            return false;
        }

        bool keep(RecordFilter filter, bool registered)
        {
            return filter == RecordFilter::All ||
                   (filter == RecordFilter::Registered) == registered;
        }
    }

    std::vector<Game> games()
    {
        std::vector<Game> result;
        for (size_t i = 0; i < GAME_COUNT; ++i)
        {
            result.push_back(static_cast<Game>(i));
        }
        return result;
    }

    Text gameName(Game game)
    {
        switch (game)
        {
            case Game::FireRedLeafGreen:
                return Text::DexGameFireRedLeafGreen;
            case Game::LetsGo:
                return Text::DexGameLetsGo;
            case Game::SwordShield:
                return Text::DexGameSwordShield;
            case Game::BrilliantDiamondShiningPearl:
                return Text::DexGameDiamondPearl;
            case Game::LegendsArceus:
                return Text::DexGameLegendsArceus;
            case Game::ScarletViolet:
                return Text::DexGameScarletViolet;
            case Game::LegendsZA:
            case Game::Count:
                break;
        }
        return Text::DexGameLegendsZA;
    }

    const char* gameKey(Game game)
    {
        const auto index = static_cast<size_t>(game);
        return index < KEYS.size() ? KEYS[index] : "";
    }

    std::optional<Game> gameFromKey(std::string_view key)
    {
        for (size_t i = 0; i < KEYS.size(); ++i)
        {
            if (key == KEYS[i])
            {
                return static_cast<Game>(i);
            }
        }
        return std::nullopt;
    }

    pksm::GameVersion gameVersion(Game game)
    {
        return tableVersion(game);
    }

    pksm::Generation gameGeneration(Game game)
    {
        switch (game)
        {
            case Game::FireRedLeafGreen:
                return pksm::Generation::THREE;
            case Game::LetsGo:
                return pksm::Generation::LGPE;
            case Game::SwordShield:
            case Game::BrilliantDiamondShiningPearl:
            case Game::LegendsArceus:
                return pksm::Generation::EIGHT;
            case Game::ScarletViolet:
            case Game::LegendsZA:
            case Game::Count:
                break;
        }
        return pksm::Generation::NINE;
    }

    std::optional<Game> gameOf(pksm::Generation generation, pksm::GameVersion version)
    {
        if (generation == pksm::Generation::THREE)
        {
            return Game::FireRedLeafGreen;
        }
        if (generation == pksm::Generation::LGPE)
        {
            return Game::LetsGo;
        }
        if (generation == pksm::Generation::EIGHT)
        {
            // The bank's entries are too short for Legends: Arceus and the Sinnoh remakes.
            return Game::SwordShield;
        }
        if (generation == pksm::Generation::NINE)
        {
            // Both games share the format; only the origin tells them apart.
            return version == pksm::GameVersion::ZA ? Game::LegendsZA : Game::ScarletViolet;
        }
        return std::nullopt;
    }

    const std::vector<uint16_t>& gameSpecies(Game game)
    {
        return table(game).order;
    }

    const std::vector<uint8_t>& forms(Game game, uint16_t species)
    {
        static const std::vector<uint8_t> none;
        if (game >= Game::Count)
        {
            return none;
        }
        const auto& all = table(game).forms;
        return species < all.size() ? all[species] : none;
    }

    size_t formCount(Game game, uint16_t species)
    {
        return forms(game, species).size();
    }

    bool hasForm(Game game, uint16_t species, size_t form)
    {
        const auto& list = forms(game, species);
        return std::find(list.begin(), list.end(), form) != list.end();
    }

    bool appears(Game game, uint16_t species)
    {
        return formCount(game, species) > 0;
    }

    const std::vector<uint16_t>& allSpecies()
    {
        static const std::vector<uint16_t> species = []
        {
            std::set<uint16_t> all;
            for (const Game game : games())
            {
                all.insert(gameSpecies(game).begin(), gameSpecies(game).end());
            }
            return std::vector<uint16_t>(all.begin(), all.end());
        }();
        return species;
    }

    std::vector<uint8_t> globalForms(uint16_t species)
    {
        std::set<uint8_t> all;
        for (const Game game : games())
        {
            all.insert(forms(game, species).begin(), forms(game, species).end());
        }
        return std::vector<uint8_t>(all.begin(), all.end());
    }

    std::pair<pksm::Type, pksm::Type> types(uint16_t species)
    {
        if (appears(Game::LegendsZA, species))
        {
            return {pksm::PersonalZA::type1(species), pksm::PersonalZA::type2(species)};
        }
        if (appears(Game::ScarletViolet, species))
        {
            return {pksm::PersonalSV::type1(species), pksm::PersonalSV::type2(species)};
        }
        if (appears(Game::LegendsArceus, species))
        {
            return {pksm::PersonalPLA::type1(species), pksm::PersonalPLA::type2(species)};
        }
        if (species <= u16(pksm::Species::Calyrex))
        {
            return {pksm::PersonalSWSH::type1(species), pksm::PersonalSWSH::type2(species)};
        }
        return {pksm::PersonalSV::type1(species), pksm::PersonalSV::type2(species)};
    }

    bool Records::add(const Key& key, const std::string& date)
    {
        return entries_.emplace(key, date).second;
    }

    const std::string* Records::date(const Key& key) const
    {
        const auto found = entries_.find(key);
        return found == entries_.end() ? nullptr : &found->second;
    }

    std::string Records::toJson() const
    {
        nlohmann::json entries = nlohmann::json::array();
        for (const auto& [key, date] : entries_)
        {
            entries.push_back({
                {"game",    gameKey(key.game)},
                {"species", key.species      },
                {"form",    key.form         },
                {"shiny",   key.shiny        },
                {"date",    date             }
            });
        }
        return nlohmann::json{
            {"version", 1                 },
            {"entries", std::move(entries)}
        }
            .dump(2);
    }

    std::optional<Records> Records::fromJson(std::string_view text)
    {
        const nlohmann::json json = nlohmann::json::parse(text, nullptr, false);
        if (json.is_discarded() || !json.is_object())
        {
            return std::nullopt;
        }
        const auto entries = json.find("entries");
        if (entries == json.end() || !entries->is_array())
        {
            return std::nullopt;
        }
        Records records;
        for (const auto& entry : *entries)
        {
            if (!entry.is_object())
            {
                continue;
            }
            const auto game    = entry.find("game");
            const auto species = entry.find("species");
            const auto form    = entry.find("form");
            const auto shiny   = entry.find("shiny");
            const auto date    = entry.find("date");
            if (game == entry.end() || !game->is_string() || species == entry.end() ||
                !species->is_number_unsigned() || form == entry.end() ||
                !form->is_number_unsigned() || shiny == entry.end() || !shiny->is_boolean())
            {
                continue;
            }
            const auto parsedGame = gameFromKey(game->get<std::string>());
            const auto number     = species->get<uint64_t>();
            const auto formNumber = form->get<uint64_t>();
            if (!parsedGame || number == 0 || number > 0xFFFF || formNumber > 0xFF)
            {
                continue;
            }
            records.add({*parsedGame, static_cast<uint16_t>(number),
                            static_cast<uint8_t>(formNumber), shiny->get<bool>()},
                date != entry.end() && date->is_string() ? date->get<std::string>() : "");
        }
        return records;
    }

    Text recordFilterName(RecordFilter filter)
    {
        switch (filter)
        {
            case RecordFilter::Registered:
                return Text::DexRegistered;
            case RecordFilter::Missing:
                return Text::DexNotRegistered;
            default:
                return Text::DexFilterAny;
        }
    }

    Text formFilterName(FormFilter filter)
    {
        switch (filter)
        {
            case FormFilter::Shiny:
                return Text::DexShiny;
            case FormFilter::AllForms:
                return Text::DexAllForms;
            default:
                return Text::DexFilterAny;
        }
    }

    unsigned Progress::percent() const
    {
        if (total == 0)
        {
            return 0;
        }
        return static_cast<unsigned>(std::min(registered, total) * 100 / total);
    }

    Progress speciesProgress(const Records& records, Game game, uint16_t species, bool shiny)
    {
        Progress result;
        for (const uint8_t form : forms(game, species))
        {
            ++result.total;
            if (records.has({game, species, form, shiny}))
            {
                ++result.registered;
            }
        }
        return result;
    }

    bool registeredAnywhere(const Records& records, uint16_t species, uint8_t form, bool shiny)
    {
        for (const Game game : games())
        {
            if (records.has({game, species, form, shiny}))
            {
                return true;
            }
        }
        return false;
    }

    std::vector<Cell> cells(
        const Records& records, std::optional<Game> game, RecordFilter record, FormFilter form)
    {
        std::vector<Cell> result;
        const auto& species = game ? gameSpecies(*game) : allSpecies();
        const bool shiny    = form == FormFilter::Shiny;
        for (const uint16_t number : species)
        {
            const std::vector<uint8_t> list = game ? forms(*game, number) : globalForms(number);
            const auto registered           = [&](uint8_t form, bool shinyForm)
            {
                return game ? records.has({*game, number, form, shinyForm})
                            : registeredAnywhere(records, number, form, shinyForm);
            };
            if (form == FormFilter::Species)
            {
                Cell cell;
                cell.species = number;
                cell.forms   = list.size();
                for (const uint8_t each : list)
                {
                    cell.formsRegistered += registered(each, false) ? 1 : 0;
                }
                cell.registered = cell.formsRegistered > 0;
                if (keep(record, cell.registered))
                {
                    result.push_back(cell);
                }
                continue;
            }
            for (const uint8_t each : list)
            {
                Cell cell;
                cell.species         = number;
                cell.form            = each;
                cell.shiny           = shiny;
                cell.registered      = registered(each, shiny);
                cell.formsRegistered = cell.registered ? 1 : 0;
                if (keep(record, cell.registered))
                {
                    result.push_back(cell);
                }
            }
        }
        return result;
    }

    Progress progress(const Records& records, std::optional<Game> game, FormFilter form)
    {
        if (!game)
        {
            Progress sum;
            for (const Game each : games())
            {
                const Progress part = progress(records, each, form);
                sum.registered     += part.registered;
                sum.total          += part.total;
            }
            return sum;
        }
        Progress result;
        const bool shiny = form == FormFilter::Shiny;
        for (const uint16_t species : gameSpecies(*game))
        {
            if (form == FormFilter::Species)
            {
                ++result.total;
                if (hasAnyForm(records, *game, species, false))
                {
                    ++result.registered;
                }
                continue;
            }
            const Progress forms = speciesProgress(records, *game, species, shiny);
            result.total        += forms.total;
            result.registered   += forms.registered;
        }
        return result;
    }
}
