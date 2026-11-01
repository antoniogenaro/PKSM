#include "screens/PokedexScreen.hpp"

#include "app/App.hpp"
#include "domain/FieldValues.hpp"
#include "domain/FormSprites.hpp"
#include "domain/PickerList.hpp"
#include "domain/PokedexFile.hpp"
#include "enums/Species.hpp"
#include "ui/Text.hpp"
#include "utils/i18n.hpp"
#include <algorithm>
#include <cctype>

namespace pksm::switchui
{
    namespace
    {
        constexpr size_t PAGE = PokedexView::PAGE;

        // The next value of a filter, back to the first after the last.
        template <typename Filter>
        Filter next(Filter filter)
        {
            const auto value = static_cast<size_t>(filter) + 1;
            return value < static_cast<size_t>(Filter::Count) ? static_cast<Filter>(value)
                                                              : static_cast<Filter>(0);
        }

        Text globalNote(dex::FormFilter filter)
        {
            switch (filter)
            {
                case dex::FormFilter::Shiny:
                    return Text::DexGlobalShiny;
                case dex::FormFilter::AllForms:
                    return Text::DexGlobalForms;
                default:
                    return Text::DexGlobalSpecies;
            }
        }

        std::string upper(std::string text)
        {
            // The game names are ASCII but for "Púrpura", whose accented letter is kept.
            std::transform(text.begin(), text.end(), text.begin(),
                [](unsigned char c) { return c < 0x80 ? static_cast<char>(std::toupper(c)) : c; });
            for (size_t at = text.find("ú"); at != std::string::npos; at = text.find("ú", at))
            {
                text.replace(at, std::string("ú").size(), "Ú");
            }
            return text;
        }
    }

    PokedexScreen::PokedexScreen()
        : language_(
              textLanguage() == TextLanguage::Spanish ? pksm::Language::SPA : pksm::Language::ENG)
    {
        i18n::init(language_);
        // For the names a language lacks.
        i18n::init(pksm::Language::ENG);
        if (!dex::loadRecords(records_, error_))
        {
            records_ = dex::Records{};
        }
        refresh();
    }

    std::optional<dex::Game> PokedexScreen::game() const
    {
        if (game_ == 0 || game_ > dex::GAME_COUNT)
        {
            return std::nullopt;
        }
        return static_cast<dex::Game>(game_ - 1);
    }

    std::string PokedexScreen::formName(
        pksm::Species species, uint8_t form, pksm::GameVersion version) const
    {
        // Core names the forms it has no name for as it names Bulbasaur's only form.
        const std::string& unnamed = i18n::form(language_, version, pksm::Species::Bulbasaur, 0);
        std::string name           = i18n::form(language_, version, species, form);
        if (name.empty())
        {
            name = i18n::form(pksm::Language::ENG, version, species, form);
        }
        if (!name.empty() && name != unnamed &&
            name != i18n::form(pksm::Language::ENG, version, pksm::Species::Bulbasaur, 0))
        {
            return name;
        }
        const bool genderForms =
            species == pksm::Species::Meowstic || species == pksm::Species::Indeedee ||
            species == pksm::Species::Basculegion || species == pksm::Species::Oinkologne;
        if (genderForms && form < 2)
        {
            return tr(form == 0 ? Text::DexFormMale : Text::DexFormFemale);
        }
        if (form == 0)
        {
            return speciesName(species);
        }
        return tr(Text::DexFormNumber, {std::to_string(form)});
    }

    std::string PokedexScreen::speciesName(pksm::Species species) const
    {
        // Some languages stop at older generations; their newer Pokémon keep the English names.
        const std::string& name = i18n::species(language_, species);
        return name.empty() ? i18n::species(pksm::Language::ENG, species) : name;
    }

    dex::Game PokedexScreen::shownGame(uint16_t species) const
    {
        if (const auto chosen = game())
        {
            return *chosen;
        }
        const auto games = dex::games();
        for (auto it = games.rbegin(); it != games.rend(); ++it)
        {
            if (dex::appears(*it, species))
            {
                return *it;
            }
        }
        return dex::Game::ScarletViolet;
    }

    void PokedexScreen::refresh()
    {
        std::optional<dex::Cell> focused;
        if (selected_ < cells_.size())
        {
            focused = cells_[selected_];
        }

        cells_ = dex::cells(records_, game(), record_, form_);
        progress_.clear();
        progress_.push_back(dex::progress(records_, std::nullopt, form_));
        for (const dex::Game each : dex::games())
        {
            progress_.push_back(dex::progress(records_, each, form_));
        }

        // Stay on the same Pokémon, or its species, when the new list has it.
        size_t index = std::min(selected_, cells_.empty() ? 0 : cells_.size() - 1);
        if (focused)
        {
            const auto sameForm =
                std::find_if(cells_.begin(), cells_.end(), [&](const dex::Cell& cell)
                    { return cell.species == focused->species && cell.form == focused->form; });
            const auto sameSpecies = std::find_if(cells_.begin(), cells_.end(),
                [&](const dex::Cell& cell) { return cell.species == focused->species; });
            if (sameForm != cells_.end())
            {
                index = static_cast<size_t>(sameForm - cells_.begin());
            }
            else if (sameSpecies != cells_.end())
            {
                index = static_cast<size_t>(sameSpecies - cells_.begin());
            }
        }
        selected_ = index;
    }

    void PokedexScreen::moveFocus(Action action)
    {
        if (cells_.empty())
        {
            return;
        }
        const size_t columns = PokedexView::COLUMNS;
        const size_t inPage  = selected_ % PAGE;
        switch (action)
        {
            case Action::Left:
                if (selected_ > 0)
                {
                    --selected_;
                }
                break;
            case Action::Right:
                if (selected_ + 1 < cells_.size())
                {
                    ++selected_;
                }
                break;
            case Action::Up:
                if (inPage >= columns)
                {
                    selected_ -= columns;
                }
                break;
            case Action::Down:
            {
                // To the row below on the same page; a shorter last row takes its last cell.
                const size_t rowStart = selected_ - inPage % columns + columns;
                if (inPage + columns < PAGE && rowStart < cells_.size())
                {
                    selected_ = std::min(selected_ + columns, cells_.size() - 1);
                }
                break;
            }
            default:
                break;
        }
    }

    void PokedexScreen::update(App& app, Action action)
    {
        switch (action)
        {
            case Action::Back:
            case Action::Quit:
                app.pop();
                break;
            case Action::Left:
            case Action::Right:
            case Action::Up:
            case Action::Down:
                moveFocus(action);
                break;
            case Action::PreviousPage:
                selected_ = turnPage(selected_, cells_.size(), PAGE, -1);
                break;
            case Action::NextPage:
                selected_ = turnPage(selected_, cells_.size(), PAGE, 1);
                break;
            case Action::PreviousTab:
                game_ = game_ == 0 ? dex::GAME_COUNT : game_ - 1;
                refresh();
                break;
            case Action::NextTab:
                game_ = game_ >= dex::GAME_COUNT ? 0 : game_ + 1;
                refresh();
                break;
            case Action::Discard: // X
                record_ = next(record_);
                refresh();
                break;
            case Action::Move: // Y
                form_ = next(form_);
                refresh();
                break;
            default:
                break;
        }
    }

    void PokedexScreen::fillFocus(PokedexView& view) const
    {
        if (selected_ >= cells_.size())
        {
            return;
        }
        const dex::Cell& cell    = cells_[selected_];
        const pksm::Species kind = pksm::Species{cell.species};
        const dex::Game shown    = shownGame(cell.species);
        const auto generation    = dex::gameGeneration(shown);
        const auto version       = dex::gameVersion(shown);
        const bool shiny         = form_ == dex::FormFilter::Shiny;

        view.hasFocus             = true;
        view.number               = tr(Text::DexNumber, {padDigits(cell.species, 4)});
        view.name                 = speciesName(kind);
        view.species              = cell.species;
        view.formSprite           = formSpriteIndex(generation, kind, cell.form);
        view.shiny                = cell.shiny;
        const auto [type1, type2] = dex::types(cell.species);
        view.types.push_back({type1.localize(language_), static_cast<size_t>(u8(type1))});
        if (type2 != type1)
        {
            view.types.push_back({type2.localize(language_), static_cast<size_t>(u8(type2))});
        }

        // The forms, in the chosen game or in any game, four at a time around the focused one.
        const auto chosen = game();
        const std::vector<uint8_t> forms =
            chosen ? dex::forms(*chosen, cell.species) : dex::globalForms(cell.species);
        const auto formRegistered = [&](uint8_t form)
        {
            return chosen ? records_.has({*chosen, cell.species, form, shiny})
                          : dex::registeredAnywhere(records_, cell.species, form, shiny);
        };
        const size_t registered =
            static_cast<size_t>(std::count_if(forms.begin(), forms.end(), formRegistered));
        view.formProgress = std::to_string(registered) + " / " + std::to_string(forms.size());
        constexpr size_t shownForms = 4;
        const size_t at =
            static_cast<size_t>(std::find(forms.begin(), forms.end(), cell.form) - forms.begin());
        const size_t first = at < forms.size() ? at / shownForms * shownForms : 0;
        for (size_t i = first; i < std::min(forms.size(), first + shownForms); ++i)
        {
            const uint8_t form = forms[i];
            PokedexView::Form entry;
            entry.name       = formName(kind, form, version);
            entry.species    = cell.species;
            entry.formSprite = formSpriteIndex(generation, kind, form);
            entry.registered = formRegistered(form);
            entry.focused    = form_ != dex::FormFilter::Species && form == cell.form;
            view.forms.push_back(std::move(entry));
        }

        for (const dex::Game each : dex::games())
        {
            PokedexView::GameState state;
            state.name    = tr(dex::gameName(each));
            state.appears = dex::appears(each, cell.species);
            if (state.appears)
            {
                const dex::Progress part =
                    dex::speciesProgress(records_, each, cell.species, shiny);
                state.registered = part.registered > 0;
                state.state = std::to_string(part.registered) + " / " + std::to_string(part.total);
            }
            else
            {
                state.state = tr(Text::DexNotInGame);
            }
            view.byGame.push_back(std::move(state));
        }
    }

    void PokedexScreen::draw(SwitchUi& ui)
    {
        PokedexView view;
        const auto toGame = [](const dex::Progress& progress, std::string name)
        { return PokedexView::Game{std::move(name), progress.percent(), progress.complete()}; };
        view.global      = toGame(progress_[0], tr(Text::DexGlobal));
        view.globalNote  = tr(globalNote(form_));
        const auto games = dex::games();
        for (size_t i = 0; i < games.size(); ++i)
        {
            view.games.push_back(toGame(progress_[i + 1], tr(dex::gameName(games[i]))));
        }
        view.game         = game_;
        view.caption      = game() ? upper(tr(dex::gameName(*game()))) : tr(Text::DexGlobalCaption);
        view.recordFilter = tr(dex::recordFilterName(record_));
        view.formFilter   = tr(dex::formFilterName(form_));

        view.page          = selected_ / PAGE;
        view.pageCount     = pageCount(cells_.size(), PAGE);
        view.selected      = selected_ % PAGE;
        const size_t first = view.page * PAGE;
        for (size_t i = first; i < std::min(cells_.size(), first + PAGE); ++i)
        {
            const dex::Cell& cell = cells_[i];
            PokedexView::Cell shown;
            shown.species    = cell.species;
            shown.formSprite = formSpriteIndex(dex::gameGeneration(shownGame(cell.species)),
                pksm::Species{cell.species}, cell.form);
            shown.number     = padDigits(cell.species, 4);
            shown.registered = cell.registered;
            shown.shiny      = cell.shiny;
            if (form_ == dex::FormFilter::Species && cell.forms > 1)
            {
                shown.badge =
                    std::to_string(cell.formsRegistered) + "/" + std::to_string(cell.forms);
            }
            view.cells.push_back(std::move(shown));
        }
        if (cells_.empty())
        {
            view.message = error_.empty() ? tr(Text::DexNoMatches) : error_;
        }
        else if (!error_.empty())
        {
            view.message = error_;
        }
        fillFocus(view);

        ui.drawPokedex(view);

        // Touches: a game, a cell or a filter.
        if (view.game != game_ && view.game <= dex::GAME_COUNT)
        {
            game_ = view.game;
            refresh();
        }
        else if (first + view.selected < cells_.size() && view.selected != selected_ % PAGE)
        {
            selected_ = first + view.selected;
        }
        if (view.recordRequest)
        {
            record_ = next(record_);
            refresh();
        }
        if (view.formRequest)
        {
            form_ = next(form_);
            refresh();
        }
    }
}
