#include "ui/Text.hpp"

#include "nlohmann/json.hpp"

#include <array>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <optional>

namespace pksm::switchui
{
    namespace
    {
        constexpr size_t TEXT_COUNT = static_cast<size_t>(Text::Count);

        constexpr std::array<const char*, TEXT_COUNT> KEYS = {
#define PKSM_SWITCH_TEXT_NAME(key) #key,
            PKSM_SWITCH_TEXTS(PKSM_SWITCH_TEXT_NAME)
#undef PKSM_SWITCH_TEXT_NAME
        };

        std::array<std::string, TEXT_COUNT> texts;
        bool textsLoaded             = false;
        TextLanguage currentLanguage = TextLanguage::English;

        size_t placeholders(std::string_view text)
        {
            size_t count = 0;
            for (size_t at = text.find("{}"); at != std::string_view::npos;
                 at        = text.find("{}", at + 2))
            {
                ++count;
            }
            return count;
        }

        std::optional<nlohmann::json> readLanguage(std::string_view folder, TextLanguage language)
        {
            const std::string path =
                std::string(folder) + "/" + textLanguageCode(language) + ".json";
            std::ifstream file(path, std::ios::binary);
            if (!file)
            {
                return std::nullopt;
            }
            const std::string content{std::istreambuf_iterator<char>(file), {}};
            nlohmann::json json = nlohmann::json::parse(content, nullptr, false);
            if (json.is_discarded() || !json.is_object())
            {
                return std::nullopt;
            }
            return json;
        }

        // The text at `key` in `json`, or null when it is missing, empty or not a string.
        const std::string* textIn(const std::optional<nlohmann::json>& json, const char* key)
        {
            if (!json)
            {
                return nullptr;
            }
            const auto found = json->find(key);
            if (found == json->end() || !found->is_string() ||
                found->get_ref<const std::string&>().empty())
            {
                return nullptr;
            }
            return &found->get_ref<const std::string&>();
        }
    }

    const char* textLanguageCode(TextLanguage language)
    {
        return language == TextLanguage::Spanish ? "es" : "en";
    }

    const char* textKey(Text key)
    {
        const auto index = static_cast<size_t>(key);
        return index < KEYS.size() ? KEYS[index] : "";
    }

    bool loadTexts(TextLanguage language, std::string_view folder)
    {
        const std::optional<nlohmann::json> english = readLanguage(folder, TextLanguage::English);
        std::optional<nlohmann::json> chosen;
        if (language != TextLanguage::English)
        {
            chosen = readLanguage(folder, language);
        }

        for (size_t index = 0; index < TEXT_COUNT; ++index)
        {
            const std::string* base = textIn(english, KEYS[index]);
            const std::string* text = textIn(chosen, KEYS[index]);
            if (text && base && placeholders(*text) != placeholders(*base))
            {
                text = nullptr;
            }
            texts[index] = text ? *text : base ? *base : KEYS[index];
        }
        textsLoaded     = true;
        currentLanguage = language;
        return english.has_value() && (language == TextLanguage::English || chosen.has_value());
    }

    TextLanguage textLanguage()
    {
        return currentLanguage;
    }

    const char* tr(Text key)
    {
        const auto index = static_cast<size_t>(key);
        if (index >= TEXT_COUNT)
        {
            return "";
        }
        return textsLoaded ? texts[index].c_str() : KEYS[index];
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
