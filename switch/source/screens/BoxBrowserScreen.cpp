#include "screens/BoxBrowserScreen.hpp"

#include "app/App.hpp"
#include "app/Session.hpp"
#include "pkx/PKX.hpp"
#include "sav/Sav.hpp"
#include "screens/PokemonEditorScreen.hpp"
#include "screens/PokemonFields.hpp"
#include "screens/SaveSummary.hpp"
#include "ui/Text.hpp"
#include <algorithm>

namespace pksm::switchui
{
    namespace
    {
        constexpr size_t BOX_SLOTS   = 30;
        constexpr size_t PARTY_SLOTS = 6;
        constexpr size_t COLUMNS     = BoxView::COLUMNS;
        constexpr size_t BOX_ROWS    = BOX_SLOTS / COLUMNS;
    }

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
        boxSlots_.clear();
        boxFilled_ = 0;
        for (u8 slot = 0; slot < BOX_SLOTS; ++slot)
        {
            const auto pokemon = save.pkm(static_cast<u8>(box_), slot);
            boxSlots_.push_back(pokemonSlot(pokemon.get()));
            boxFilled_ += boxSlots_.back().empty ? 0 : 1;
        }
        partySlots_.clear();
        partyCount_ = std::min<size_t>(PARTY_SLOTS, save.partyCount());
        for (u8 slot = 0; slot < PARTY_SLOTS; ++slot)
        {
            const auto pokemon = slot < partyCount_ ? save.pkm(slot) : nullptr;
            partySlots_.push_back(pokemonSlot(pokemon.get()));
        }
        refreshDetail();
    }

    void BoxBrowserScreen::refreshDetail()
    {
        pksm::Sav& save = session_.save();
        const u8 slot   = static_cast<u8>(selected_);
        std::unique_ptr<pksm::PKX> pokemon;
        if (party_)
        {
            if (selected_ < partyCount_)
            {
                pokemon = save.pkm(slot);
            }
        }
        else
        {
            pokemon = save.pkm(static_cast<u8>(box_), slot);
        }
        detail_ = pokemon ? describePokemon(*pokemon) : PokemonDetail{};
    }

    void BoxBrowserScreen::onResume()
    {
        refreshSlots();
    }

    size_t BoxBrowserScreen::selectablePartySlots() const
    {
        return std::min(PARTY_SLOTS, partyCount_ + 1);
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
                moveSelection(action);
                clearMessage();
                break;
            case Action::PreviousPage:
                changeBox(-1);
                break;
            case Action::NextPage:
                changeBox(1);
                break;
            case Action::Back:
                if (carried_.active)
                {
                    // Nothing was written yet, so cancelling only puts it down.
                    carried_ = {};
                    message_.clear();
                    break;
                }
                leaveAsking(app, session_, message_);
                break;
            case Action::Discard:
                cloneOrPlace();
                break;
            case Action::Move:
                if (!carried_.clone)
                {
                    pickUpOrDrop();
                }
                break;
            case Action::Save:
                if (session_.dirty())
                {
                    session_.saveWithMessage(message_);
                }
                break;
            case Action::Confirm:
                if (!carried_.active)
                {
                    openSelected(app);
                }
                break;
            default:
                break;
        }
    }

    void BoxBrowserScreen::changeBox(int direction)
    {
        box_ = direction < 0 ? (box_ == 0 ? maxBoxes_ - 1 : box_ - 1) : (box_ + 1) % maxBoxes_;
        clearMessage();
        refreshSlots();
    }

    void BoxBrowserScreen::moveSelection(Action action)
    {
        const size_t partySlots = selectablePartySlots();
        if (party_)
        {
            // The party is one column left of the box; right goes into the box.
            if (action == Action::Up)
            {
                selected_ = selected_ == 0 ? partySlots - 1 : selected_ - 1;
            }
            else if (action == Action::Down)
            {
                selected_ = (selected_ + 1) % partySlots;
            }
            else if (action == Action::Right)
            {
                party_    = false;
                selected_ = std::min(selected_, BOX_ROWS - 1) * COLUMNS;
            }
        }
        else
        {
            const size_t row    = selected_ / COLUMNS;
            const size_t column = selected_ % COLUMNS;
            if (action == Action::Up)
            {
                selected_ = (row == 0 ? BOX_ROWS - 1 : row - 1) * COLUMNS + column;
            }
            else if (action == Action::Down)
            {
                selected_ = ((row + 1) % BOX_ROWS) * COLUMNS + column;
            }
            else if (action == Action::Left)
            {
                if (column == 0)
                {
                    // Left from the first column crosses over to the party.
                    party_    = true;
                    selected_ = std::min(row, partySlots - 1);
                }
                else
                {
                    --selected_;
                }
            }
            else if (column != COLUMNS - 1)
            {
                ++selected_;
            }
        }
        refreshDetail();
    }

    void BoxBrowserScreen::clearMessage()
    {
        if (!carried_.active)
        {
            message_.clear();
            return;
        }
        message_ =
            tr(carried_.clone ? Text::PokemonCloning : Text::PokemonCarrying, {carried_.name});
    }

    bool BoxBrowserScreen::carriedSlotShown() const
    {
        // A clone leaves the original where it is, so its slot is not dimmed.
        return carried_.active && !carried_.clone && (carried_.party || carried_.box == box_);
    }

    void BoxBrowserScreen::pickUp(bool clone)
    {
        const PokemonSlot& slot = party_ ? partySlots_[selected_] : boxSlots_[selected_];
        if (slot.empty || detail_.empty)
        {
            return;
        }
        carried_ = {true, clone, party_, box_, selected_, slot, detail_.name};
        clearMessage();
    }

    std::unique_ptr<pksm::PKX> BoxBrowserScreen::readAt(bool party, unsigned int box, size_t slot)
    {
        pksm::Sav& save = session_.save();
        std::unique_ptr<pksm::PKX> pokemon;
        if (party)
        {
            pokemon = slot < partyCount_ ? save.pkm(static_cast<u8>(slot)) : nullptr;
        }
        else
        {
            pokemon = save.pkm(static_cast<u8>(box), static_cast<u8>(slot));
        }
        return pokemon ? std::move(pokemon) : save.emptyPkm();
    }

    void BoxBrowserScreen::writeAt(pksm::PKX& pokemon, bool party, unsigned int box, size_t slot)
    {
        pksm::Sav& save = session_.save();
        if (party)
        {
            pokemon.updatePartyData();
        }
        pokemon.refreshChecksum();
        if (party)
        {
            save.pkm(pokemon, static_cast<u8>(slot));
        }
        else
        {
            save.pkm(pokemon, static_cast<u8>(box), static_cast<u8>(slot), false);
        }
    }

    void BoxBrowserScreen::cloneOrPlace()
    {
        if (!carried_.active)
        {
            pickUp(true);
            return;
        }
        if (carried_.clone)
        {
            placeClone();
        }
    }

    void BoxBrowserScreen::placeClone()
    {
        auto copy   = readAt(carried_.party, carried_.box, carried_.slot);
        auto target = readAt(party_, box_, selected_);
        if (!copy || !target || copy->species() == pksm::Species::None)
        {
            carried_ = {};
            message_ = tr(Text::PokemonReadFailed);
            return;
        }
        if (target->species() != pksm::Species::None)
        {
            message_ = tr(Text::CloneNeedsEmptySlot);
            return;
        }

        writeAt(*copy, party_, box_, selected_);
        if (party_)
        {
            session_.save().fixParty();
        }

        carried_ = {};
        message_.clear();
        session_.markDirty();
        refreshSlots();
    }

    void BoxBrowserScreen::pickUpOrDrop()
    {
        if (!carried_.active)
        {
            pickUp(false);
            return;
        }
        if (carried_.party == party_ && (party_ || carried_.box == box_) &&
            carried_.slot == selected_)
        {
            // Dropped where it was taken from.
            carried_ = {};
            message_.clear();
            return;
        }
        dropCarried();
    }

    void BoxBrowserScreen::dropCarried()
    {
        pksm::Sav& save = session_.save();
        auto moving = readAt(carried_.party, carried_.box, carried_.slot);
        auto target = readAt(party_, box_, selected_);
        if (!moving || !target || moving->species() == pksm::Species::None)
        {
            carried_ = {};
            message_ = tr(Text::PokemonReadFailed);
            return;
        }

        // A party Pokémon dropped on an empty box slot leaves the party one short.
        if (carried_.party && !party_ && target->species() == pksm::Species::None &&
            partyCount_ <= 1)
        {
            message_ = tr(Text::PartyNeedsPokemon);
            return;
        }

        writeAt(*moving, party_, box_, selected_);
        writeAt(*target, carried_.party, carried_.box, carried_.slot);
        // Packed once both are written, so neither write lands on a shifted slot.
        if (party_ || carried_.party)
        {
            save.fixParty();
        }

        carried_ = {};
        message_.clear();
        session_.markDirty();
        refreshSlots();
        if (party_)
        {
            // The party may have closed up behind a Pokémon that left it.
            selected_ = std::min(selected_, selectablePartySlots() - 1);
            refreshDetail();
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
        BoxView view;
        view.boxName    = session_.save().boxName(static_cast<u8>(box_));
        view.boxFilled  = boxFilled_;
        view.box        = boxSlots_;
        view.party      = partySlots_;
        view.partyCount = partyCount_;
        view.inParty    = party_;
        view.selected   = selected_;
        view.detail     = detail_;
        view.message    = sessionMessage(session_, message_);
        view.carrying       = carried_.active;
        view.cloning        = carried_.clone;
        view.carriedShown   = carriedSlotShown();
        view.carriedInParty = carried_.party;
        view.carriedSlot    = carried_.slot;
        view.carried        = carried_.sprite;
        ui.drawBoxes(view);

        // A touch on a slot or an arrow.
        if (view.pageRequest != 0)
        {
            changeBox(view.pageRequest);
        }
        else if (view.inParty != party_ || view.selected != selected_)
        {
            party_    = view.inParty;
            selected_ = view.selected;
            refreshDetail();
        }
    }
}
