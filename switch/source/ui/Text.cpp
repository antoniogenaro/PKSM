#include "ui/Text.hpp"

#include <array>
#include <cstddef>

namespace pksm::switchui
{
    namespace
    {
        using Table = std::array<const char*, static_cast<size_t>(Text::Count)>;

        constexpr Table ENGLISH = {
#define PKSM_SWITCH_TEXT_VALUE(key, english, spanish) english,
            PKSM_SWITCH_TEXTS(PKSM_SWITCH_TEXT_VALUE)
#undef PKSM_SWITCH_TEXT_VALUE
        };

        constexpr Table SPANISH = {
#define PKSM_SWITCH_TEXT_VALUE(key, english, spanish) spanish,
            PKSM_SWITCH_TEXTS(PKSM_SWITCH_TEXT_VALUE)
#undef PKSM_SWITCH_TEXT_VALUE
        };

        TextLanguage currentLanguage = TextLanguage::English;
    }

    void setTextLanguage(TextLanguage language)
    {
        currentLanguage = language;
    }

    TextLanguage textLanguage()
    {
        return currentLanguage;
    }

    const char* tr(Text key)
    {
        const Table& table = currentLanguage == TextLanguage::Spanish ? SPANISH : ENGLISH;
        const auto index   = static_cast<size_t>(key);
        return index < table.size() ? table[index] : "";
    }

    std::string tr(Text key, std::initializer_list<std::string_view> values)
    {
        const std::string_view pattern = tr(key);
        auto value                     = values.begin();
        std::string text;
        text.reserve(pattern.size());
        for (size_t i = 0; i < pattern.size(); ++i)
        {
            if (pattern[i] == '{' && i + 1 < pattern.size() && pattern[i + 1] == '}' &&
                value != values.end())
            {
                text += *value++;
                ++i;
            }
            else
            {
                text += pattern[i];
            }
        }
        return text;
    }
}
