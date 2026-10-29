#pragma once

#include "app/Screen.hpp"
#include "sav/Sav.hpp"
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace pksm::switchui
{
    class Session;

    // The bag: every pouch of the save and the items in it. L/R change pouch, A picks the item in
    // a slot, ZL/ZR step its quantity and − types it, X takes it out and Y picks it up to swap it
    // with another. Changes go straight to the save; the game menu below saves them.
    class BagScreen : public Screen
    {
    public:
        BagScreen(Session& session, std::string path);

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;

    private:
        // An item of the shown pouch, as the save numbers it, and its quantity.
        struct Slot
        {
            unsigned int id;
            unsigned int count;
        };

        pksm::Sav::Pouch pouch() const { return pouches_[pouch_].first; }
        size_t capacity() const { return static_cast<size_t>(pouches_[pouch_].second); }
        // The slots the selection can reach: the items, then one empty slot to add another while
        // there is room. Carrying an item, only the items.
        size_t shownSlots() const;

        // Reads the shown pouch again, and how many items every pouch holds.
        void refresh();
        void changePouch(int direction);
        void move(Action action);

        void pickItem(App& app);
        void setItem(size_t slot, unsigned int id);
        void removeItem(size_t slot);
        void stepCount(int direction);
        void typeCount(App& app);
        void setCount(size_t slot, unsigned int count);
        // Y: picks the selected item up, or swaps the carried one with the selected item.
        void pickUpOrDrop();

        unsigned int maxCount() const;
        void changed();
        // The footer while nothing else is said: what is being carried, if anything.
        void clearMessage();

        Session& session_;
        std::string path_;
        std::vector<std::pair<pksm::Sav::Pouch, int>> pouches_;
        // How many items each pouch holds.
        std::vector<size_t> filled_;
        std::vector<Slot> slots_;
        size_t pouch_ = 0;
        // The selection and first row shown of each pouch, kept while another pouch is shown.
        std::vector<size_t> selected_;
        std::vector<size_t> firstRow_;
        // The slot of the item picked up with Y, in the shown pouch.
        std::optional<size_t> carried_;
        std::string message_;
    };
}
