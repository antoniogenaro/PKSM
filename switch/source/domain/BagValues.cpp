#include "domain/BagValues.hpp"

#include <algorithm>

namespace pksm::switchui
{
    std::vector<unsigned int> pouchItems(std::span<const int> valid, unsigned int maxItem)
    {
        std::vector<unsigned int> items;
        items.reserve(valid.size());
        for (const int value : valid)
        {
            if (value <= 0 || static_cast<unsigned int>(value) > maxItem)
            {
                continue;
            }
            const auto item = static_cast<unsigned int>(value);
            if (std::find(items.begin(), items.end(), item) == items.end())
            {
                items.push_back(item);
            }
        }
        return items;
    }

    std::vector<unsigned int> slotChoices(const std::vector<unsigned int>& allowed,
        const std::vector<unsigned int>& inPouch, unsigned int current,
        const std::function<std::string(unsigned int)>& name)
    {
        struct Named
        {
            std::string name;
            unsigned int item;
        };
        std::vector<Named> named;
        named.reserve(allowed.size() + 1);
        // The slot's own item stays a choice even when the list leaves it out.
        if (current != 0 && std::find(allowed.begin(), allowed.end(), current) == allowed.end())
        {
            named.push_back({name(current), current});
        }
        for (const unsigned int item : allowed)
        {
            const bool elsewhere = item != current &&
                std::find(inPouch.begin(), inPouch.end(), item) != inPouch.end();
            if (!elsewhere)
            {
                named.push_back({name(item), item});
            }
        }
        std::sort(named.begin(), named.end(), [](const Named& a, const Named& b)
            { return a.name != b.name ? a.name < b.name : a.item < b.item; });

        std::vector<unsigned int> choices{0};
        choices.reserve(named.size() + 1);
        for (const Named& entry : named)
        {
            choices.push_back(entry.item);
        }
        return choices;
    }

    size_t firstShownRow(size_t firstRow, size_t row, size_t shownRows, size_t rows)
    {
        if (shownRows == 0)
        {
            return 0;
        }
        if (row < firstRow)
        {
            firstRow = row;
        }
        else if (row >= firstRow + shownRows)
        {
            firstRow = row - shownRows + 1;
        }
        const size_t lastFirst = rows > shownRows ? rows - shownRows : 0;
        return std::min(firstRow, lastFirst);
    }
}
