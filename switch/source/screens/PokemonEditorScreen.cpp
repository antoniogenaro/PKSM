#include "screens/PokemonEditorScreen.hpp"

#include "SwitchUi.hpp"
#include "app/App.hpp"
#include "app/Session.hpp"
#include "pkx/PKX.hpp"
#include "sav/Sav.hpp"
#include "screens/PokemonFields.hpp"
#include "ui/Text.hpp"
#include "utils/i18n.hpp"
#include <algorithm>

namespace pksm::switchui
{
    namespace
    {
        constexpr size_t COLUMNS   = EditorView::COLUMNS;
        constexpr size_t STAT_ROWS = EditorView::STAT_ROWS;
        constexpr size_t PAGES     = 2;
        constexpr size_t MOVES     = 4;
    }

    PokemonEditorScreen::PokemonEditorScreen(Session& session, std::string game, std::string path,
        std::unique_ptr<pksm::PKX> pokemon, bool party, u8 box, u8 slot)
        : session_(session),
          game_(std::move(game)),
          path_(std::move(path)),
          pokemon_(std::move(pokemon)),
          party_(party),
          box_(box),
          slot_(slot)
    {
        i18n::init(pokemon_->language());
    }

    PokemonEditorScreen::~PokemonEditorScreen() = default;

    const EditableField* PokemonEditorScreen::selectedField(const PokemonEditorFields& fields) const
    {
        const std::vector<EditableField>& page = page_ == 0 ? fields.main : fields.more;
        switch (area_)
        {
            case Area::Fields:
                return selected_ < page.size() ? &page[selected_] : nullptr;
            case Area::Iv:
                return selected_ < fields.ivs.size() ? &fields.ivs[selected_] : nullptr;
            case Area::Ev:
                return selected_ < fields.evs.size() ? &fields.evs[selected_] : nullptr;
            case Area::Moves:
                return selected_ < fields.moves.size() ? &fields.moves[selected_] : nullptr;
        }
        return nullptr;
    }

    void PokemonEditorScreen::saveChanges()
    {
        savePokemon(session_.save(), *pokemon_, party_, box_, slot_);
        session_.markDirty();
        unsaved_ = false;
        message_ = tr(Text::EditorSaved);
    }

    void PokemonEditorScreen::resetChanges()
    {
        pksm::Sav& save = session_.save();
        auto pokemon    = party_ ? save.pkm(slot_) : save.pkm(box_, slot_);
        if (!pokemon)
        {
            message_ = tr(Text::PokemonReadFailed);
            return;
        }
        pokemon_ = std::move(pokemon);
        unsaved_ = false;
        message_ = tr(Text::EditorReset);
    }

    void PokemonEditorScreen::move(Action action, size_t pageFields)
    {
        switch (area_)
        {
            case Area::Fields:
            {
                const size_t row    = selected_ / COLUMNS;
                const size_t column = selected_ % COLUMNS;
                if (action == Action::Left && column > 0)
                {
                    --selected_;
                }
                else if (action == Action::Right)
                {
                    // Past the row's last card, over to the stats table.
                    if (column + 1 < COLUMNS && selected_ + 1 < pageFields)
                    {
                        ++selected_;
                    }
                    else
                    {
                        area_     = Area::Iv;
                        selected_ = std::min(row, STAT_ROWS - 1);
                    }
                }
                else if (action == Action::Up && row > 0)
                {
                    selected_ -= COLUMNS;
                }
                else if (action == Action::Down && (row + 1) * COLUMNS < pageFields)
                {
                    selected_ = std::min(selected_ + COLUMNS, pageFields - 1);
                }
                break;
            }
            case Area::Iv:
            case Area::Ev:
                if (action == Action::Left)
                {
                    if (area_ == Area::Ev)
                    {
                        area_ = Area::Iv;
                    }
                    else
                    {
                        // Back to the last card of the matching row.
                        area_     = Area::Fields;
                        selected_ = std::min(selected_ * COLUMNS + COLUMNS - 1, pageFields - 1);
                    }
                }
                else if (action == Action::Right && area_ == Area::Iv)
                {
                    area_ = Area::Ev;
                }
                else if (action == Action::Up && selected_ > 0)
                {
                    --selected_;
                }
                else if (action == Action::Down)
                {
                    if (selected_ + 1 < STAT_ROWS)
                    {
                        ++selected_;
                    }
                    else
                    {
                        area_     = Area::Moves;
                        selected_ = 0;
                    }
                }
                break;
            case Area::Moves:
                if (action == Action::Left)
                {
                    area_     = Area::Fields;
                    selected_ = pageFields - 1;
                }
                else if (action == Action::Up)
                {
                    if (selected_ > 0)
                    {
                        --selected_;
                    }
                    else
                    {
                        area_     = Area::Iv;
                        selected_ = STAT_ROWS - 1;
                    }
                }
                else if (action == Action::Down && selected_ + 1 < MOVES)
                {
                    ++selected_;
                }
                break;
        }
    }

    void PokemonEditorScreen::update(App& app, Action action)
    {
        const PokemonEditorFields fields = pokemonEditorFields(*pokemon_, session_.save());
        const size_t pageFields          = (page_ == 0 ? fields.main : fields.more).size();
        // The edit stays here until +; a picker calls this later, while this screen is below it.
        const auto onChanged = [this]
        {
            unsaved_ = true;
            message_.clear();
        };
        if (action != Action::Back && action != Action::None)
        {
            warnedBack_ = false;
        }
        switch (action)
        {
            case Action::Quit:
                if (unsaved_)
                {
                    saveChanges();
                }
                break;
            case Action::Discard:
                if (unsaved_)
                {
                    resetChanges();
                }
                break;
            case Action::Back:
                if (unsaved_ && !warnedBack_)
                {
                    message_    = tr(Text::EditorUnsavedBack);
                    warnedBack_ = true;
                }
                else
                {
                    app.pop();
                }
                break;
            case Action::Up:
            case Action::Down:
            case Action::Left:
            case Action::Right:
                if (pageFields > 0)
                {
                    move(action, pageFields);
                }
                if (!unsaved_)
                {
                    message_.clear();
                }
                break;
            case Action::PreviousTab:
            case Action::NextTab:
                page_ = (page_ + (action == Action::NextTab ? 1 : PAGES - 1)) % PAGES;
                if (area_ == Area::Fields)
                {
                    const size_t count = (page_ == 0 ? fields.main : fields.more).size();
                    selected_          = std::min(selected_, count == 0 ? 0 : count - 1);
                }
                break;
            case Action::PreviousPage:
            case Action::NextPage:
                if (const EditableField* field = selectedField(fields))
                {
                    if (stepField(*field, action == Action::NextPage ? 1 : -1))
                    {
                        onChanged();
                    }
                }
                break;
            case Action::Confirm:
                if (const EditableField* field = selectedField(fields))
                {
                    openField(*field, app, message_, onChanged);
                }
                break;
            default:
                break;
        }
    }

    void PokemonEditorScreen::draw(SwitchUi& ui)
    {
        const PokemonEditorFields fields = pokemonEditorFields(*pokemon_, session_.save());
        EditorView view;
        view.header    = describePokemon(*pokemon_);
        view.fields    = displayFields(page_ == 0 ? fields.main : fields.more);
        view.page      = page_;
        view.pageCount = PAGES;
        view.stats     = pokemonStats(*pokemon_);
        view.area      = area_;
        view.selected  = selected_;
        view.message   = !message_.empty() ? message_
                         : unsaved_        ? std::string(tr(Text::EditorUnsaved))
                                           : std::string{};
        ui.drawPokemonEditor(view);

        // A touch on a card, a stat cell or a move.
        area_     = view.area;
        selected_ = view.selected;
    }
}
