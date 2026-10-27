#include "screens/BoxBrowserScreen.hpp"

#include "app/App.hpp"
#include "app/Session.hpp"
#include "sav/Sav.hpp"
#include "screens/PokemonEditorScreen.hpp"
#include "screens/PokemonFields.hpp"
#include "ui/Text.hpp"
#include <algorithm>

namespace pksm::switchui
{
    BoxBrowserScreen::BoxBrowserScreen(Session& session, std::string game, std::string path)
        : session_(session),
          game_(std::move(game)),
          path_(std::move(path)),
          maxBoxes_(static_cast<unsigned int>(session.save().maxBoxes())),
          box_(std::min<unsigned int>(session.save().currentBox(), maxBoxes_ - 1))
    {
        refreshSlots();
    }

    void BoxBrowserScreen::refreshSlots()
    {
        pksm::Sav& save = session_.save();
        slots_          = party_ ? partyFields(save) : boxFields(save, static_cast<u8>(box_));
    }

    void BoxBrowserScreen::onResume()
    {
        refreshSlots();
    }

    void BoxBrowserScreen::update(App& app, Action action)
    {
        switch (action)
        {
            case Action::Quit:
                if (!session_.dirty())
                {
                    app.quit();
                    return;
                }
                message_ = tr(Text::UnsavedBeforeQuit);
                break;
            case Action::Up:
            case Action::Down:
            case Action::Left:
            case Action::Right:
                if (!slots_.empty())
                {
                    moveSelection(action);
                    message_.clear();
                }
                break;
            case Action::PreviousPage:
                selected_ = 0;
                if (party_)
                {
                    party_ = false;
                    box_   = maxBoxes_ - 1;
                }
                else
                {
                    box_ = box_ == 0 ? maxBoxes_ - 1 : box_ - 1;
                }
                refreshSlots();
                message_.clear();
                break;
            case Action::NextPage:
                selected_ = 0;
                if (party_)
                {
                    party_ = false;
                    box_   = 0;
                }
                else if (box_ + 1 == maxBoxes_)
                {
                    party_ = true;
                }
                else
                {
                    ++box_;
                }
                refreshSlots();
                message_.clear();
                break;
            case Action::Back:
                app.pop();
                break;
            case Action::Discard:
                app.closeSession();
                break;
            case Action::Save:
                if (session_.dirty())
                {
                    session_.exportWithMessage(message_);
                }
                break;
            case Action::Confirm:
                if (!slots_.empty())
                {
                    openSelected(app);
                }
                break;
            default:
                break;
        }
    }

    void BoxBrowserScreen::moveSelection(Action action)
    {
        const size_t columns   = party_ ? 3 : 6;
        const size_t rows      = (slots_.size() + columns - 1) / columns;
        const size_t row       = selected_ / columns;
        const size_t column    = selected_ % columns;
        const size_t rowStart  = row * columns;
        const size_t rowLength = std::min(columns, slots_.size() - rowStart);
        if (action == Action::Left)
        {
            selected_ = rowStart + (column == 0 ? rowLength - 1 : column - 1);
        }
        else if (action == Action::Right)
        {
            selected_ = rowStart + (column + 1) % rowLength;
        }
        else
        {
            const size_t nextRow =
                action == Action::Up ? (row == 0 ? rows - 1 : row - 1) : (row + 1) % rows;
            selected_ = std::min(nextRow * columns + column, slots_.size() - 1);
        }
    }

    void BoxBrowserScreen::openSelected(App& app)
    {
        pksm::Sav& save = session_.save();
        const u8 slot   = static_cast<u8>(selected_);
        auto pokemon    = party_ ? save.pkm(slot) : save.pkm(static_cast<u8>(box_), slot);
        if (!pokemon)
        {
            message_ = tr(Text::PokemonReadFailed);
            return;
        }
        app.push(std::make_unique<PokemonEditorScreen>(session_, game_, path_, std::move(pokemon),
            party_, static_cast<u8>(box_), slot));
    }

    void BoxBrowserScreen::draw(SwitchUi& ui)
    {
        pksm::Sav& save = session_.save();
        const std::string section =
            party_ ? tr(Text::PartySection, {numericValue(save.partyCount())})
                   : tr(Text::BoxSection, {numericValue(box_ + 1), numericValue(maxBoxes_),
                                              save.boxName(static_cast<u8>(box_))});
        ui.drawPokemonSlots(game_, path_, section, slots_, selected_, session_.dirty(), message_);
    }
}
