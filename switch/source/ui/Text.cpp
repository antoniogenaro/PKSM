#include "ui/Text.hpp"

#include <array>
#include <cstddef>

namespace pksm::switchui
{
    namespace
    {
        constexpr std::array<const char*, static_cast<size_t>(Text::Count)> SPANISH = {
#define PKSM_SWITCH_TEXT_VALUE(key, spanish) spanish,
            PKSM_SWITCH_TEXTS(PKSM_SWITCH_TEXT_VALUE)
#undef PKSM_SWITCH_TEXT_VALUE
        };
    }

    const char* tr(Text key)
    {
        const auto index = static_cast<size_t>(key);
        return index < SPANISH.size() ? SPANISH[index] : "";
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
