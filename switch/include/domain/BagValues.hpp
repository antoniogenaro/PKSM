#pragma once

#include <cstddef>
#include <functional>
#include <span>
#include <string>
#include <vector>

// How the bag editor reads and changes a pouch. Pure functions, so they can be tested on a PC.
// A pouch is a packed list: its items come first and the slots after them are empty (item 0).
namespace pksm::switchui
{
    // The items a pouch can hold, from the save's list for it: without the empty item, values
    // above `maxItem` or repeats, in the list's order.
    std::vector<unsigned int> pouchItems(std::span<const int> valid, unsigned int maxItem);

    // What a slot holding `current` can change to: 0 first, to take the item out, then every
    // item of `allowed` that is not in another slot of the pouch (`inPouch`, which may include
    // `current`), sorted by `name` and then by number. `current` is always among them.
    std::vector<unsigned int> slotChoices(const std::vector<unsigned int>& allowed,
        const std::vector<unsigned int>& inPouch, unsigned int current,
        const std::function<std::string(unsigned int)>& name);

    // The first row to show of `rows`, `shownRows` at a time, so that `row` stays in view.
    // It moves as little as it can from `firstRow`.
    size_t firstShownRow(size_t firstRow, size_t row, size_t shownRows, size_t rows);
}
