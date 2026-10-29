#include "screens/BankScreen.hpp"

#include "app/App.hpp"
#include "app/Session.hpp"
#include "domain/GameProfile.hpp"
#include "domain/PokemonBank.hpp"
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
        constexpr size_t BOX_SLOTS = PokemonBank::SLOTS;
        constexpr size_t COLUMNS   = BankView::COLUMNS;
        constexpr size_t ROWS      = BOX_SLOTS / COLUMNS;

        // "3", "8" or "LGPE", as the transfer messages name a generation.
        std::string generationName(pksm::Generation generation)
        {
            return static_cast<std::string>(generation);
        }

        Text transferReasonText(pksm::Sav::BadTransferReason reason)
        {
            switch (reason)
            {
                case pksm::Sav::BadTransferReason::MOVE:
                    return Text::TransferBadMove;
                case pksm::Sav::BadTransferReason::SPECIES:
                    return Text::TransferBadSpecies;
                case pksm::Sav::BadTransferReason::FORM:
                    return Text::TransferBadForm;
                case pksm::Sav::BadTransferReason::ABILITY:
                    return Text::TransferBadAbility;
                case pksm::Sav::BadTransferReason::ITEM:
                    return Text::TransferBadItem;
                case pksm::Sav::BadTransferReason::BALL:
                default:
                    return Text::TransferBadBall;
            }
        }
    }

    BankScreen::BankScreen(Session& session, PokemonBank& bank)
        : session_(session),
          bank_(bank),
          gameBox_(std::min(static_cast<int>(session.save().currentBox()),
              session.save().maxBoxes() - 1))
    {
        refreshSlots();
    }

    int BankScreen::boxCount(bool game) const
    {
        return game ? session_.save().maxBoxes() : bank_.boxes();
    }

    BankScreen::Place BankScreen::selectedPlace() const
    {
        return {inGame_, inGame_ ? gameBox_ : bankBox_, selected_};
    }

    std::unique_ptr<pksm::PKX> BankScreen::read(const Place& place, bool& readable) const
    {
        readable = true;
        const int slot = static_cast<int>(place.slot);
        if (place.game)
        {
            auto pokemon = session_.save().pkm(static_cast<u8>(place.box), static_cast<u8>(slot));
            if (!pokemon || pokemon->species() == pksm::Species::None)
            {
                return nullptr;
            }
            return pokemon;
        }
        auto pokemon = bank_.pkm(place.box, slot);
        readable     = pokemon || bank_.empty(place.box, slot);
        return pokemon;
    }

    void BankScreen::refreshSlots()
    {
        const auto fill = [this](bool game, int box, std::vector<PokemonSlot>& slots,
                              size_t& filled)
        {
            slots.clear();
            filled = 0;
            for (size_t slot = 0; slot < BOX_SLOTS; ++slot)
            {
                bool readable  = true;
                const auto pkm = read({game, box, slot}, readable);
                slots.push_back(pokemonSlot(pkm.get()));
                filled += pkm || !readable ? 1 : 0;
            }
        };
        fill(false, bankBox_, bankSlots_, bankFilled_);
        fill(true, gameBox_, gameSlots_, gameFilled_);
        refreshDetail();
    }

    void BankScreen::refreshDetail()
    {
        bool readable      = true;
        const auto pokemon = read(selectedPlace(), readable);
        detail_            = pokemon ? describePokemon(*pokemon) : PokemonDetail{};
    }

    void BankScreen::update(App& app, Action action)
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
                confirming_ = false;
                clearMessage();
                break;
            case Action::PreviousPage:
                changeBox(inGame_, -1);
                break;
            case Action::NextPage:
                changeBox(inGame_, 1);
                break;
            case Action::Back:
                if (carried_.active)
                {
                    // Nothing was written yet, so cancelling only puts it down.
                    carried_    = {};
                    confirming_ = false;
                    message_.clear();
                    break;
                }
                // The game menu below keeps the save and the bank open and saves both.
                app.pop();
                break;
            case Action::Confirm:
                if (!carried_.active)
                {
                    openSelected(app);
                }
                break;
            case Action::Save:
                pickUpOrDrop();
                break;
            default:
                break;
        }
    }

    void BankScreen::changeBox(bool game, int direction)
    {
        int& box        = game ? gameBox_ : bankBox_;
        const int count = boxCount(game);
        box             = (box + (direction < 0 ? count - 1 : 1)) % count;
        confirming_     = false;
        clearMessage();
        refreshSlots();
    }

    void BankScreen::moveSelection(Action action)
    {
        const size_t row    = selected_ / COLUMNS;
        const size_t column = selected_ % COLUMNS;
        if (action == Action::Up)
        {
            selected_ = (row == 0 ? ROWS - 1 : row - 1) * COLUMNS + column;
        }
        else if (action == Action::Down)
        {
            selected_ = ((row + 1) % ROWS) * COLUMNS + column;
        }
        else if (action == Action::Left)
        {
            if (column > 0)
            {
                --selected_;
            }
            else if (inGame_)
            {
                // The bank sits left of the save, so the two boxes read as one grid.
                inGame_   = false;
                selected_ = row * COLUMNS + COLUMNS - 1;
            }
        }
        else if (column < COLUMNS - 1)
        {
            ++selected_;
        }
        else if (!inGame_)
        {
            inGame_   = true;
            selected_ = row * COLUMNS;
        }
        refreshDetail();
    }

    void BankScreen::clearMessage()
    {
        message_ = carried_.active ? tr(Text::PokemonCarrying, {carried_.name}) : std::string{};
    }

    void BankScreen::pickUpOrDrop()
    {
        const Place place = selectedPlace();
        if (!carried_.active)
        {
            bool readable      = true;
            const auto pokemon = read(place, readable);
            if (!readable)
            {
                message_ = tr(Text::BankSlotUnreadable);
                return;
            }
            if (!pokemon)
            {
                return;
            }
            const auto& slots = place.game ? gameSlots_ : bankSlots_;
            carried_          = {true, place, slots[place.slot], detail_.name};
            clearMessage();
            return;
        }
        if (carried_.from == place)
        {
            // Dropped where it was taken from.
            carried_    = {};
            confirming_ = false;
            message_.clear();
            return;
        }
        drop();
    }

    std::unique_ptr<pksm::PKX> BankScreen::convertForSave(const pksm::PKX& pokemon)
    {
        pksm::Sav& save        = session_.save();
        const std::string name = describePokemon(pokemon).name;
        auto converted         = save.transfer(pokemon);
        if (!converted)
        {
            message_ = tr(Text::TransferNoPath,
                {name, generationName(pokemon.generation()), generationName(save.generation())});
            return nullptr;
        }
        const auto reason = save.invalidTransferReason(pokemon);
        if (reason != pksm::Sav::BadTransferReason::OKAY)
        {
            message_ = tr(transferReasonText(reason), {name});
            return nullptr;
        }
        return converted;
    }

    void BankScreen::drop()
    {
        const Place from = carried_.from;
        const Place to   = selectedPlace();
        bool readable    = true;
        auto moving      = read(from, readable);
        if (!moving)
        {
            carried_ = {};
            message_ = tr(Text::PokemonReadFailed);
            refreshSlots();
            return;
        }
        auto target = read(to, readable);
        if (!readable)
        {
            message_ = tr(Text::BankSlotUnreadable);
            return;
        }

        // Only a Pokémon going from the bank into the save changes format; one going into the
        // bank is kept as it is.
        const pksm::Sav& save         = session_.save();
        const pksm::PKX* changing     = nullptr;
        std::unique_ptr<pksm::PKX> movingOut;
        std::unique_ptr<pksm::PKX> targetOut;
        if (to.game && !from.game)
        {
            movingOut = convertForSave(*moving);
            if (!movingOut)
            {
                return;
            }
            changing = moving->generation() != save.generation() ? moving.get() : nullptr;
        }
        else
        {
            movingOut = std::move(moving);
        }
        if (target && from.game && !to.game)
        {
            targetOut = convertForSave(*target);
            if (!targetOut)
            {
                return;
            }
            changing = target->generation() != save.generation() ? target.get() : changing;
        }
        else
        {
            targetOut = std::move(target);
        }

        // Converting cannot be undone, so it waits for a second press on the same slot.
        if (changing && !(confirming_ && confirmTarget_ == to))
        {
            confirming_    = true;
            confirmTarget_ = to;
            message_       = tr(Text::TransferConfirm,
                      {describePokemon(*changing).name, generationName(save.generation())});
            return;
        }
        confirming_ = false;

        write(to, movingOut.get(), !from.game);
        write(from, targetOut.get(), !to.game);

        carried_ = {};
        message_.clear();
        refreshSlots();
    }

    void BankScreen::write(const Place& place, pksm::PKX* pokemon, bool fromBank)
    {
        const int slot = static_cast<int>(place.slot);
        if (!place.game)
        {
            if (pokemon)
            {
                bank_.pkm(*pokemon, place.box, slot);
            }
            else
            {
                bank_.clear(place.box, slot);
            }
            return;
        }

        pksm::Sav& save = session_.save();
        const u8 box    = static_cast<u8>(place.box);
        if (pokemon)
        {
            pokemon->refreshChecksum();
            // As the 3DS does, a Pokémon from the bank meets its new trainer.
            save.pkm(*pokemon, box, static_cast<u8>(slot), fromBank);
            if (fromBank)
            {
                save.dex(*pokemon);
            }
        }
        else
        {
            save.pkm(*save.emptyPkm(), box, static_cast<u8>(slot), false);
        }
        session_.markDirty();
    }

    void BankScreen::onResume()
    {
        refreshSlots();
    }

    void BankScreen::openSelected(App& app)
    {
        const Place place  = selectedPlace();
        bool readable      = true;
        auto pokemon       = read(place, readable);
        if (!readable)
        {
            message_ = tr(Text::BankSlotUnreadable);
            return;
        }
        if (!pokemon)
        {
            return;
        }
        if (place.game)
        {
            app.push(std::make_unique<PokemonEditorScreen>(session_, tr(session_.profile().name),
                session_.sourcePath(), std::move(pokemon), false, static_cast<u8>(place.box),
                static_cast<u8>(place.slot)));
            return;
        }

        // A bank Pokémon is written back to the bank, as it is. The editor's lists come from the
        // open save, so one from another generation is only shown.
        const bool readOnly = pokemon->generation() != session_.save().generation();
        PokemonBank& bank   = bank_;
        const int box       = place.box;
        const int slot      = static_cast<int>(place.slot);
        EditorSlot target;
        target.read = [&bank, box, slot]
        {
            auto stored = bank.pkm(box, slot);
            return stored;
        };
        target.write = [&bank, box, slot](pksm::PKX& edited)
        {
            edited.refreshChecksum();
            bank.pkm(edited, box, slot);
        };
        app.push(std::make_unique<PokemonEditorScreen>(
            session_, std::move(pokemon), std::move(target), readOnly));
    }

    void BankScreen::draw(SwitchUi& ui)
    {
        BankView view;
        view.bank.source  = bank_.fileName();
        view.bank.boxName = bank_.boxName(bankBox_);
        view.bank.filled  = bankFilled_;
        view.bank.slots   = bankSlots_;
        view.game.source  = tr(session_.profile().name);
        view.game.boxName = session_.save().boxName(static_cast<u8>(gameBox_));
        view.game.filled  = gameFilled_;
        view.game.slots   = gameSlots_;
        view.inGame       = inGame_;
        view.selected     = selected_;
        view.detail       = detail_;
        view.detailWhere  = tr(Text::BankSlotWhere,
             {std::to_string((inGame_ ? gameBox_ : bankBox_) + 1), std::to_string(selected_ + 1)});
        view.message       = sessionMessage(session_, message_);
        view.carrying      = carried_.active;
        view.carriedInGame = carried_.from.game;
        view.carriedShown  = carried_.active &&
                            carried_.from.box == (carried_.from.game ? gameBox_ : bankBox_);
        view.carriedSlot = carried_.from.slot;
        view.carried     = carried_.sprite;
        ui.drawBank(view);

        // A touch on a slot or an arrow.
        if (view.pageRequest != 0)
        {
            changeBox(view.pageInGame, view.pageRequest);
        }
        else if (view.inGame != inGame_ || view.selected != selected_)
        {
            inGame_     = view.inGame;
            selected_   = view.selected;
            confirming_ = false;
            refreshDetail();
        }
    }
}
