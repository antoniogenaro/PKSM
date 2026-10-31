#include "domain/PickerList.hpp"

#include <algorithm>

namespace pksm::switchui
{
    namespace
    {
        // The plain lowercase letter for a Latin-1 letter (U+00C0 to U+00FF), or 0 when it has
        // none, such as for "×" and "÷".
        char plainLatin1(unsigned codePoint)
        {
            // From U+00C0, two rows of 32: uppercase, then lowercase.
            static constexpr char letters[] = "aaaaaaaceeeeiiii" "dnooooo\0ouuuuyts"
                                              "aaaaaaaceeeeiiii" "dnooooo\0ouuuuyty";
            if (codePoint < 0xC0 || codePoint > 0xFF)
            {
                return 0;
            }
            return letters[codePoint - 0xC0];
        }

        // `text` folded for searching and sorting: ASCII letters in lowercase and accented
        // Latin letters without their accent. `source[i]` is where folded byte `i` came from in
        // `text`, with one more entry for the end.
        struct Folded
        {
            std::string text;
            std::vector<size_t> source;
        };

        Folded fold(std::string_view text)
        {
            Folded folded;
            folded.text.reserve(text.size());
            folded.source.reserve(text.size() + 1);
            for (size_t i = 0; i < text.size(); ++i)
            {
                const unsigned char byte = static_cast<unsigned char>(text[i]);
                // Two-byte UTF-8 for U+00C0 to U+00FF starts with 0xC3.
                if (byte == 0xC3 && i + 1 < text.size())
                {
                    const unsigned next      = static_cast<unsigned char>(text[i + 1]);
                    const unsigned codePoint = 0xC0 + (next & 0x3F);
                    if ((next & 0xC0) == 0x80)
                    {
                        if (const char plain = plainLatin1(codePoint))
                        {
                            folded.text.push_back(plain);
                            folded.source.push_back(i);
                            ++i;
                            continue;
                        }
                    }
                }
                folded.text.push_back(byte >= 'A' && byte <= 'Z' ? static_cast<char>(byte - 'A' + 'a')
                                                                 : static_cast<char>(byte));
                folded.source.push_back(i);
            }
            folded.source.push_back(text.size());
            return folded;
        }
    }

    std::optional<TextMatch> findText(std::string_view text, std::string_view query)
    {
        if (query.empty())
        {
            return TextMatch{};
        }
        const Folded haystack = fold(text);
        const std::string needle = fold(query).text;
        const size_t at = haystack.text.find(needle);
        if (at == std::string::npos)
        {
            return std::nullopt;
        }
        const size_t start = haystack.source[at];
        return TextMatch{start, haystack.source[at + needle.size()] - start};
    }

    std::vector<size_t> pickerEntries(const std::vector<std::string>& options,
        const std::vector<unsigned int>& numbers, std::string_view query, PickerOrder order)
    {
        std::vector<size_t> entries;
        entries.reserve(options.size());
        for (size_t i = 0; i < options.size(); ++i)
        {
            if (findText(options[i], query))
            {
                entries.push_back(i);
            }
        }

        if (order == PickerOrder::Name)
        {
            std::vector<std::string> keys(options.size());
            for (const size_t i : entries)
            {
                keys[i] = fold(options[i]).text;
            }
            std::stable_sort(entries.begin(), entries.end(),
                [&keys](size_t a, size_t b) { return keys[a] < keys[b]; });
        }
        else if (numbers.size() == options.size())
        {
            std::stable_sort(entries.begin(), entries.end(),
                [&numbers](size_t a, size_t b) { return numbers[a] < numbers[b]; });
        }
        return entries;
    }

    size_t pageCount(size_t entries, size_t perPage)
    {
        if (perPage == 0 || entries == 0)
        {
            return 1;
        }
        return (entries + perPage - 1) / perPage;
    }

    size_t turnPage(size_t index, size_t entries, size_t perPage, int direction)
    {
        if (entries == 0 || perPage == 0)
        {
            return 0;
        }
        index = std::min(index, entries - 1);
        const size_t page = index / perPage;
        if (direction < 0)
        {
            return page == 0 ? index : index - perPage;
        }
        if (direction > 0 && page + 1 < pageCount(entries, perPage))
        {
            return std::min(entries - 1, index + perPage);
        }
        return index;
    }
}
