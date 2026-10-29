#pragma once

#include "SwitchUi.hpp"
#include "app/Screen.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace pksm
{
    class PKX;
}

namespace pksm::switchui
{
    class Session;
    class PokemonBank;

    // A box of the bank beside a box of the open save, and the selected Pokémon's detail. The
    // D-pad moves through both boxes as one grid; L/R change the box the cursor is in. A opens
    // the selected Pokémon in the editor. Y picks it up and, on another slot, drops it there,
    // swapping the two. A Pokémon that enters the save from the bank is converted to the save's
    // generation first.
    class BankScreen : public Screen
    {
    public:
        BankScreen(Session& session, PokemonBank& bank);

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;
        void onResume() override;

    private:
        // Where a slot is: in the bank or in the save, in which box.
        struct Place
        {
            bool game        = false;
            int box          = 0;
            size_t slot      = 0;
            bool operator==(const Place&) const = default;
        };

        void refreshSlots();
        void refreshDetail();
        void moveSelection(Action action);
        void changeBox(bool game, int direction);
        Place selectedPlace() const;
        int boxCount(bool game) const;

        // The Pokémon at `place`, or nullptr when the slot is empty. `readable` is false when
        // the slot holds data that cannot be read.
        std::unique_ptr<pksm::PKX> read(const Place& place, bool& readable) const;
        // Puts `pokemon` (nullptr to empty it) at `place`. Into the save, it must already be
        // in the save's format.
        void write(const Place& place, pksm::PKX* pokemon, bool fromBank);

        void pickUpOrDrop();
        // A: the selected Pokémon in the editor.
        void openSelected(App& app);
        void drop();
        // `pokemon` as the save stores it, or nullptr with the reason in message_.
        std::unique_ptr<pksm::PKX> convertForSave(const pksm::PKX& pokemon);
        void clearMessage();

        struct Carried
        {
            bool active = false;
            Place from;
            PokemonSlot sprite;
            std::string name;
        };

        Session& session_;
        PokemonBank& bank_;
        int bankBox_ = 0;
        int gameBox_ = 0;
        bool inGame_ = false;
        size_t selected_ = 0;
        std::vector<PokemonSlot> bankSlots_;
        std::vector<PokemonSlot> gameSlots_;
        size_t bankFilled_ = 0;
        size_t gameFilled_ = 0;
        PokemonDetail detail_;
        std::string message_;
        Carried carried_;
        // A drop that changes a Pokémon's generation waits for a second press on the same slot.
        bool confirming_ = false;
        Place confirmTarget_;
    };
}
