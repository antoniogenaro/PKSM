#pragma once

#include "SwitchUi.hpp"
#include "app/Screen.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace pksm::switchui
{
    class Session;

    // The party, the boxes of an open save and the selected Pokémon's detail, left to right. The
    // D-pad moves through the box and across to the party; L/R change the box. Y picks the
    // selected Pokémon up and, on another slot, drops it there, swapping the two.
    class BoxBrowserScreen : public Screen
    {
    public:
        BoxBrowserScreen(Session& session, std::string game, std::string path);

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;
        void onResume() override;

    private:
        // Reads the slots of the current box and the party again.
        void refreshSlots();
        // Describes the selected Pokémon again.
        void refreshDetail();
        void moveSelection(Action action);
        void changeBox(int direction);
        void openSelected(App& app);
        // The party slots that can be chosen: the filled ones and the one after them.
        size_t selectablePartySlots() const;
        // Y: picks the selected Pokémon up, or drops the carried one on the selected slot.
        void pickUpOrDrop();
        // Swaps the carried Pokémon with the selected slot and writes both to the save.
        void dropCarried();
        bool carriedSlotShown() const;
        // Clears the footer message, or shows what is being carried.
        void clearMessage();

        // A Pokémon picked up with Y, by where it was taken from.
        struct Carried
        {
            bool active      = false;
            bool party       = false;
            unsigned int box = 0;
            size_t slot      = 0;
            PokemonSlot sprite;
            std::string name;
        };

        Session& session_;
        std::string game_;
        std::string path_;
        unsigned int maxBoxes_;
        unsigned int box_;
        bool party_      = false;
        size_t selected_ = 0;
        std::vector<PokemonSlot> boxSlots_;
        std::vector<PokemonSlot> partySlots_;
        size_t boxFilled_  = 0;
        size_t partyCount_ = 0;
        PokemonDetail detail_;
        std::string message_;
        Carried carried_;
    };
}
