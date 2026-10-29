#include "screens/BagScreen.hpp"

#include "SwitchUi.hpp"
#include "app/App.hpp"
#include "app/Session.hpp"
#include "domain/BagValues.hpp"
#include "domain/FieldValues.hpp"
#include "domain/GameProfile.hpp"
#include "sav/Item.hpp"
#include "sav/Sav3.hpp"
#include "screens/PickerScreens.hpp"
#include "screens/SaveSummary.hpp"
#include "ui/Text.hpp"
#include "utils/i18n.hpp"
#include <algorithm>
#include <cstdint>
#include <memory>

namespace pksm::switchui
{
    namespace
    {
        constexpr size_t COLUMNS = BagView::COLUMNS;
        constexpr size_t ROWS    = BagView::ROWS;

        // Generation III saves number their items their own way; core names and lists them
        // in that numbering, so the bag works in it too.
        bool ownNumbering(const pksm::Sav& save)
        {
            return save.generation() == pksm::Generation::THREE;
        }

        unsigned int itemId(const pksm::Item& item)
        {
            if (item.generation() == pksm::Generation::THREE)
            {
                return static_cast<const pksm::Item3&>(item).id3();
            }
            return item.id();
        }

        void setItemId(pksm::Item& item, unsigned int id)
        {
            if (item.generation() == pksm::Generation::THREE)
            {
                static_cast<pksm::Item3&>(item).id3(static_cast<u16>(id));
            }
            else
            {
                item.id(static_cast<u16>(id));
            }
        }

        std::string itemName(const pksm::Sav& save, unsigned int id)
        {
            const pksm::Language language = save.language();
            return ownNumbering(save) ? i18n::item3(language, static_cast<u16>(id))
                                      : i18n::item(language, static_cast<u16>(id));
        }

        // The items the save lets `pouch` hold.
        std::vector<unsigned int> allowedItems(const pksm::Sav& save, pksm::Sav::Pouch pouch)
        {
            const auto lists = ownNumbering(save)
                                   ? static_cast<const pksm::Sav3&>(save).validItems3()
                                   : save.validItems();
            for (const auto& [listed, items] : lists)
            {
                if (listed == pouch)
                {
                    return pouchItems(items, UINT16_MAX);
                }
            }
            return {};
        }

        Text pouchName(pksm::Sav::Pouch pouch)
        {
            switch (pouch)
            {
                case pksm::Sav::Pouch::NormalItem:
                    return Text::BagPouchItems;
                case pksm::Sav::Pouch::KeyItem:
                    return Text::BagPouchKeyItems;
                case pksm::Sav::Pouch::TM:
                    return Text::BagPouchTms;
                case pksm::Sav::Pouch::Berry:
                    return Text::BagPouchBerries;
                case pksm::Sav::Pouch::Ball:
                    return Text::BagPouchBalls;
                case pksm::Sav::Pouch::PCItem:
                    return Text::BagPouchPcItems;
                case pksm::Sav::Pouch::Medicine:
                    return Text::BagPouchMedicine;
                case pksm::Sav::Pouch::Battle:
                    return Text::BagPouchBattle;
                case pksm::Sav::Pouch::Treasure:
                    return Text::BagPouchTreasures;
                case pksm::Sav::Pouch::Ingredient:
                    return Text::BagPouchIngredients;
                default:
                    return Text::BagPouchOther;
            }
        }

        icons::Icon pouchIcon(pksm::Sav::Pouch pouch)
        {
            switch (pouch)
            {
                case pksm::Sav::Pouch::KeyItem:
                    return icons::Icon::PouchKey;
                case pksm::Sav::Pouch::TM:
                    return icons::Icon::PouchTms;
                case pksm::Sav::Pouch::Berry:
                    return icons::Icon::PouchBerries;
                case pksm::Sav::Pouch::Ball:
                    return icons::Icon::PouchBalls;
                case pksm::Sav::Pouch::PCItem:
                    return icons::Icon::PouchPc;
                case pksm::Sav::Pouch::Medicine:
                    return icons::Icon::PouchMedicine;
                case pksm::Sav::Pouch::Battle:
                    return icons::Icon::PouchBattle;
                case pksm::Sav::Pouch::Treasure:
                    return icons::Icon::PouchTreasures;
                case pksm::Sav::Pouch::Ingredient:
                    return icons::Icon::PouchIngredients;
                default:
                    return icons::Icon::Bag;
            }
        }

        // Captions are written in capitals; game names are plain ASCII.
        std::string upper(std::string text)
        {
            for (char& c : text)
            {
                if (c >= 'a' && c <= 'z')
                {
                    c = static_cast<char>(c - 'a' + 'A');
                }
            }
            return text;
        }
    }

    BagScreen::BagScreen(Session& session, std::string path)
        : session_(session), path_(std::move(path))
    {
        for (const auto& pouch : session_.save().pouches())
        {
            if (pouch.second > 0)
            {
                pouches_.push_back(pouch);
            }
        }
        selected_.assign(pouches_.size(), 0);
        firstRow_.assign(pouches_.size(), 0);
        refresh();
    }

    size_t BagScreen::shownSlots() const
    {
        return carried_ ? slots_.size() : std::min(slots_.size() + 1, capacity());
    }

    void BagScreen::refresh()
    {
        const pksm::Sav& save = session_.save();
        filled_.assign(pouches_.size(), 0);
        for (size_t i = 0; i < pouches_.size(); ++i)
        {
            const auto [kind, size] = pouches_[i];
            std::vector<Slot> slots;
            for (int slot = 0; slot < size; ++slot)
            {
                const auto item = save.item(kind, static_cast<u16>(slot));
                if (!item || itemId(*item) == 0)
                {
                    break;
                }
                slots.push_back({itemId(*item), item->count()});
            }
            filled_[i] = slots.size();
            if (i == pouch_)
            {
                slots_ = std::move(slots);
            }
        }
        if (!pouches_.empty())
        {
            selected_[pouch_] = std::min(selected_[pouch_], shownSlots() - 1);
        }
    }

    void BagScreen::changePouch(int direction)
    {
        const size_t count = pouches_.size();
        pouch_ = direction > 0 ? (pouch_ + 1) % count : (pouch_ + count - 1) % count;
        message_.clear();
        refresh();
    }

    void BagScreen::move(Action action)
    {
        size_t& selected   = selected_[pouch_];
        const size_t shown = shownSlots();
        switch (action)
        {
            case Action::Up:
                if (selected >= COLUMNS)
                {
                    selected -= COLUMNS;
                }
                break;
            case Action::Down:
                // Onto the last slot when the row below is shorter.
                if (selected / COLUMNS + 1 < (shown + COLUMNS - 1) / COLUMNS)
                {
                    selected = std::min(selected + COLUMNS, shown - 1);
                }
                break;
            case Action::Left:
                if (selected % COLUMNS != 0)
                {
                    --selected;
                }
                break;
            case Action::Right:
                if (selected % COLUMNS != COLUMNS - 1 && selected + 1 < shown)
                {
                    ++selected;
                }
                break;
            default:
                break;
        }
    }

    unsigned int BagScreen::maxCount() const
    {
        return std::max<unsigned int>(1, session_.save().maxCount(pouch()));
    }

    void BagScreen::changed()
    {
        session_.markDirty();
        refresh();
        clearMessage();
    }

    void BagScreen::clearMessage()
    {
        if (carried_ && *carried_ < slots_.size())
        {
            message_ = tr(Text::BagCarrying, {itemName(session_.save(), slots_[*carried_].id)});
        }
        else
        {
            message_.clear();
        }
    }

    void BagScreen::pickUpOrDrop()
    {
        const size_t slot = selected_[pouch_];
        if (slot >= slots_.size())
        {
            return;
        }
        if (!carried_)
        {
            carried_ = slot;
            clearMessage();
            return;
        }
        const size_t from = *carried_;
        carried_.reset();
        if (from == slot || from >= slots_.size())
        {
            clearMessage();
            return;
        }
        // Swap the two slots as they are in the save, so nothing else of either item changes.
        pksm::Sav& save   = session_.save();
        const auto first  = save.item(pouch(), static_cast<u16>(from));
        const auto second = save.item(pouch(), static_cast<u16>(slot));
        save.item(*second, pouch(), static_cast<u16>(from));
        save.item(*first, pouch(), static_cast<u16>(slot));
        changed();
    }

    void BagScreen::pickItem(App& app)
    {
        const pksm::Sav& save         = session_.save();
        const size_t slot             = selected_[pouch_];
        const unsigned int current    = slot < slots_.size() ? slots_[slot].id : 0;
        std::vector<unsigned int> held;
        held.reserve(slots_.size());
        for (const Slot& filled : slots_)
        {
            held.push_back(filled.id);
        }
        std::vector<unsigned int> choices = slotChoices(allowedItems(save, pouch()), held, current,
            [&save](unsigned int id) { return itemName(save, id); });

        std::vector<std::string> options;
        options.reserve(choices.size());
        for (const unsigned int id : choices)
        {
            options.push_back(id == 0 ? std::string(tr(Text::BagRemoveItem)) : itemName(save, id));
        }
        // The picker closes before it calls back; this screen stays below it until then.
        app.push(std::make_unique<OptionPickerScreen>(tr(Text::FieldItem), std::move(options),
            indexOf(choices, current), std::vector<size_t>{},
            [this, choices, slot, current](size_t index)
            {
                const unsigned int picked = choices[index];
                if (picked == current)
                {
                    return;
                }
                if (picked == 0)
                {
                    removeItem(slot);
                }
                else
                {
                    setItem(slot, picked);
                }
            }));
    }

    void BagScreen::setItem(size_t slot, unsigned int id)
    {
        pksm::Sav& save = session_.save();
        auto item       = save.item(pouch(), static_cast<u16>(slot));
        if (!item)
        {
            return;
        }
        setItemId(*item, id);
        // A new item starts at one; a changed one keeps its quantity where the pouch allows it.
        const unsigned int count = slot < slots_.size() ? slots_[slot].count : 1;
        item->count(static_cast<u16>(std::clamp(count, 1u, maxCount())));
        save.item(*item, pouch(), static_cast<u16>(slot));
        changed();
    }

    void BagScreen::removeItem(size_t slot)
    {
        if (slot >= slots_.size())
        {
            return;
        }
        // The pouch stays packed: the items after it move up one slot.
        pksm::Sav& save   = session_.save();
        const size_t last = slots_.size() - 1;
        for (size_t i = slot; i < last; ++i)
        {
            const auto next = save.item(pouch(), static_cast<u16>(i + 1));
            save.item(*next, pouch(), static_cast<u16>(i));
        }
        save.item(pksm::Item4{}, pouch(), static_cast<u16>(last));
        changed();
    }

    void BagScreen::setCount(size_t slot, unsigned int count)
    {
        pksm::Sav& save = session_.save();
        auto item       = save.item(pouch(), static_cast<u16>(slot));
        if (!item)
        {
            return;
        }
        item->count(static_cast<u16>(count));
        save.item(*item, pouch(), static_cast<u16>(slot));
        changed();
    }

    void BagScreen::stepCount(int direction)
    {
        const size_t slot = selected_[pouch_];
        if (slot >= slots_.size())
        {
            return;
        }
        if (maxCount() <= 1)
        {
            message_ = tr(Text::BagCountFixed);
            return;
        }
        const unsigned int current = slots_[slot].count;
        const unsigned int next    = stepNumber({1, maxCount(), 1, false}, current, direction);
        if (next != current)
        {
            setCount(slot, next);
        }
    }

    void BagScreen::typeCount(App& app)
    {
        const size_t slot = selected_[pouch_];
        if (slot >= slots_.size())
        {
            return;
        }
        if (maxCount() <= 1)
        {
            message_ = tr(Text::BagCountFixed);
            return;
        }
        const unsigned int current = slots_[slot].count;
        std::string input;
        unsigned int value = 0;
        if (!app.ui().editText(tr(Text::BagCountTitle), std::to_string(current), input, 5) ||
            !parseNumber(input, maxCount(), value, message_))
        {
            return;
        }
        value = std::max(value, 1u);
        if (value != current)
        {
            setCount(slot, value);
        }
    }

    void BagScreen::update(App& app, Action action)
    {
        if (pouches_.empty())
        {
            app.pop();
            return;
        }
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
            case Action::Back:
                if (carried_)
                {
                    // Nothing was written yet, so cancelling only puts it down.
                    carried_.reset();
                    clearMessage();
                    break;
                }
                // The game menu below keeps the save open and warns about unsaved changes.
                app.pop();
                break;
            case Action::Up:
            case Action::Down:
            case Action::Left:
            case Action::Right:
                move(action);
                clearMessage();
                break;
            case Action::PreviousPage:
            case Action::NextPage:
                // The carried item stays in its pouch.
                if (!carried_)
                {
                    changePouch(action == Action::NextPage ? 1 : -1);
                }
                break;
            case Action::PreviousTab:
            case Action::NextTab:
                if (!carried_)
                {
                    stepCount(action == Action::NextTab ? 1 : -1);
                }
                break;
            case Action::Confirm:
                if (!carried_)
                {
                    pickItem(app);
                }
                break;
            case Action::Alternate:
                if (!carried_)
                {
                    typeCount(app);
                }
                break;
            case Action::Discard:
                // X takes the item out here; the game menu discards the save's changes.
                if (!carried_)
                {
                    removeItem(selected_[pouch_]);
                }
                break;
            case Action::Save:
                // Y moves items here; the game menu saves.
                pickUpOrDrop();
                break;
            default:
                break;
        }
    }

    void BagScreen::draw(SwitchUi& ui)
    {
        if (pouches_.empty())
        {
            return;
        }
        const pksm::Sav& save = session_.save();
        BagView view;
        view.caption = tr(Text::BagPouchesCaption, {upper(tr(session_.profile().name))});
        for (size_t i = 0; i < pouches_.size(); ++i)
        {
            view.pouches.push_back({tr(pouchName(pouches_[i].first)), std::to_string(filled_[i]),
                pouchIcon(pouches_[i].first)});
        }
        view.pouch     = pouch_;
        view.itemCount = slots_.size() == 1
                             ? std::string(tr(Text::BagItemsOne))
                             : tr(Text::BagItemsMany, {std::to_string(slots_.size())});

        const bool counted = maxCount() > 1;
        for (const Slot& slot : slots_)
        {
            view.items.push_back(
                {itemName(save, slot.id), counted ? std::to_string(slot.count) : std::string{}});
        }
        if (!carried_ && slots_.size() < capacity())
        {
            view.items.push_back({});
        }

        const size_t rows = (view.items.size() + COLUMNS - 1) / COLUMNS;
        firstRow_[pouch_] =
            firstShownRow(firstRow_[pouch_], selected_[pouch_] / COLUMNS, ROWS, rows);
        view.firstRow = firstRow_[pouch_];
        view.selected = selected_[pouch_];
        view.carried  = carried_ ? *carried_ : BagView::NONE;
        view.message  = sessionMessage(session_, message_);
        ui.drawBag(view);

        // A touch on a pouch or an item.
        if (view.pouch != pouch_ && view.pouch < pouches_.size() && !carried_)
        {
            pouch_ = view.pouch;
            message_.clear();
            refresh();
        }
        else if (view.selected < view.items.size())
        {
            selected_[pouch_] = view.selected;
        }
    }
}
