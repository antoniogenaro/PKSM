#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// What the option picker lists: the options that match the search, in the chosen order, split
// into pages. Pure functions, so they can be tested on a PC.
namespace pksm::switchui
{
    enum class PickerOrder
    {
        Number, // By each option's number, such as the species' Pokédex number.
        Name    // Alphabetically, ignoring case and accents.
    };

    // Lists whose order the option picker remembers between openings, each starting with the
    // order the editor ideas asked for: species by number, moves and items by name.
    enum class PickerKind
    {
        Other,
        Species,
        Moves,
        Items
    };

    // Where a search was found in an option: a byte range of the option's text.
    struct TextMatch
    {
        size_t start  = 0;
        size_t length = 0;
    };

    // Where `query` is in `text`, ignoring case and the accents of Latin letters, so "gelida"
    // finds "Llama Gélida". An empty query is found, with length 0, at the start.
    std::optional<TextMatch> findText(std::string_view text, std::string_view query);

    // The positions in `options` to show: those where `query` is found, sorted by `order`.
    // `numbers` holds each option's number; without them, Number keeps the given order. Equal
    // keys keep the given order.
    std::vector<size_t> pickerEntries(const std::vector<std::string>& options,
        const std::vector<unsigned int>& numbers, std::string_view query, PickerOrder order);

    // How many pages `entries` take at `perPage` a page; at least one, even when empty.
    size_t pageCount(size_t entries, size_t perPage);

    // The entry L (direction < 0) or R (direction > 0) moves to: the same place on the page
    // before or after, or the last entry when the next page is shorter. On the first or last
    // page it does not move.
    size_t turnPage(size_t index, size_t entries, size_t perPage, int direction);
}
