#include "domain/LocationNames.hpp"

#include <charconv>
#include <fstream>
#include <map>
#include <sstream>
#include <utility>

namespace pksm::switchui
{
    namespace
    {
        // Core's folder for each language.
        std::string_view languageFolder(pksm::Language language)
        {
            switch (language)
            {
                case pksm::Language::JPN:
                    return "jpn";
                case pksm::Language::FRE:
                    return "fre";
                case pksm::Language::ITA:
                    return "ita";
                case pksm::Language::GER:
                    return "ger";
                case pksm::Language::SPA:
                    return "spa";
                case pksm::Language::KOR:
                    return "kor";
                case pksm::Language::CHS:
                    return "chs";
                case pksm::Language::CHT:
                    return "cht";
                default:
                    return "eng";
            }
        }

        bool readText(const std::string& path, std::string& text)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file)
            {
                return false;
            }
            std::ostringstream contents;
            contents << file.rdbuf();
            text = contents.str();
            return true;
        }
    }

    std::vector<LocationName> parseLocationNames(std::string_view text)
    {
        std::vector<LocationName> names;
        while (!text.empty())
        {
            const size_t end      = text.find('\n');
            std::string_view line = text.substr(0, end);
            text = end == std::string_view::npos ? std::string_view{} : text.substr(end + 1);
            if (!line.empty() && line.back() == '\r')
            {
                line.remove_suffix(1);
            }

            const size_t tab = line.find('\t');
            if (tab == std::string_view::npos || tab == 0 || tab + 1 == line.size())
            {
                continue;
            }
            unsigned int number = 0;
            const auto [rest, error] =
                std::from_chars(line.data(), line.data() + tab, number);
            if (error != std::errc{} || rest != line.data() + tab)
            {
                continue;
            }
            names.push_back({number, std::string(line.substr(tab + 1))});
        }
        return names;
    }

    std::string_view locationListFile(pksm::GameVersion origin, bool generationThree)
    {
        if (origin == pksm::GameVersion::S || origin == pksm::GameVersion::R ||
            origin == pksm::GameVersion::E || origin == pksm::GameVersion::FR ||
            origin == pksm::GameVersion::LG)
        {
            return generationThree ? "gen3" : "";
        }
        if (origin == pksm::GameVersion::GP || origin == pksm::GameVersion::GE)
        {
            return "lgpe";
        }
        if (origin == pksm::GameVersion::SW || origin == pksm::GameVersion::SH)
        {
            return "swsh";
        }
        if (origin == pksm::GameVersion::PLA)
        {
            return "pla";
        }
        if (origin == pksm::GameVersion::SL || origin == pksm::GameVersion::VL)
        {
            return "sv";
        }
        if (origin == pksm::GameVersion::ZA)
        {
            return "za";
        }
        return "";
    }

    const std::vector<LocationName>& locationNames(pksm::Language language,
        pksm::GameVersion origin, bool generationThree, std::string_view root)
    {
        static const std::vector<LocationName> none;
        static std::map<std::string, std::vector<LocationName>> loaded;

        const std::string_view file = locationListFile(origin, generationThree);
        if (file.empty())
        {
            return none;
        }

        const std::string folder(languageFolder(language));
        std::string key = std::string(root) + folder + "/" + std::string(file);
        if (auto found = loaded.find(key); found != loaded.end())
        {
            return found->second;
        }

        std::string text;
        if (!readText(std::string(root) + folder + "/locations/" + std::string(file) + ".txt",
                text))
        {
            readText(std::string(root) + "eng/locations/" + std::string(file) + ".txt", text);
        }
        return loaded.emplace(std::move(key), parseLocationNames(text)).first->second;
    }

    std::string_view locationName(const std::vector<LocationName>& names, unsigned int number)
    {
        for (const auto& name : names)
        {
            if (name.number == number)
            {
                return name.name;
            }
        }
        return {};
    }
}
