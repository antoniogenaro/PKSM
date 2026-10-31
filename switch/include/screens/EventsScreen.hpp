#pragma once

#include "SwitchUi.hpp"
#include "app/Screen.hpp"
#include "enums/Language.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace pksm::switchui
{
    class EventLibrary;
    class Session;

    // The Sword and Shield wonder cards, a page at a time, with the selected one's detail. A adds
    // it to the save: a Pokémon to the first free box slot, items to the bag, Battle Points and
    // clothing to the trainer. ZL/ZR choose the language a Pokémon is received in; L/R change page.
    class EventsScreen : public Screen
    {
    public:
        EventsScreen(Session& session, std::unique_ptr<EventLibrary> library);
        ~EventsScreen() override;

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;

    private:
        size_t pageCount() const;
        size_t page() const { return selected_ / PAGE_SIZE; }
        void move(Action action);
        void changePage(int direction);
        void select(size_t card);
        void stepLanguage(int direction);
        void add();
        // Finishes the Pokémon the save made from card `card` in box slot `slot`: the chosen
        // language's name, nickname and trainer, and a PID as the game makes it.
        void finishPokemon(size_t card, int slot);
        void describe(EventsView& view) const;

        static constexpr size_t PAGE_SIZE = 16;

        Session& session_;
        std::unique_ptr<EventLibrary> library_;
        // How the list shows each card, made once.
        std::vector<EventsView::Card> cards_;
        size_t selected_ = 0;
        // The languages a Pokémon can be received in, the save's first, and the chosen one.
        std::vector<pksm::Language> languages_;
        size_t language_ = 0;
        std::string message_;
    };
}
