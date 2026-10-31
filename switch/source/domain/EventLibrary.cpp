#include "domain/EventLibrary.hpp"

#include "nlohmann/json.hpp"
#include "ui/Text.hpp"
#include <cstdio>
#include <fstream>
#include <iterator>

namespace pksm::switchui
{
    namespace
    {
        constexpr char NAMES_PATH[] = "romfs:/mg/swsh.json";
        constexpr char DATA_PATH[]  = "romfs:/mg/swsh.bin";

        bool readFile(const char* path, std::vector<u8>& data)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file)
            {
                return false;
            }
            data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
            return !file.bad();
        }
    }

    std::unique_ptr<EventLibrary> EventLibrary::load(std::string& error)
    {
        std::vector<u8> names;
        auto library = std::unique_ptr<EventLibrary>(new EventLibrary());
        if (!readFile(NAMES_PATH, names) || !readFile(DATA_PATH, library->data_))
        {
            error = tr(Text::EventsUnreadable);
            return nullptr;
        }
        const nlohmann::json json = nlohmann::json::parse(names.begin(), names.end(), nullptr, false);
        if (!json.is_array() || json.size() * pksm::WC8::length != library->data_.size())
        {
            error = tr(Text::EventsUnreadable);
            return nullptr;
        }
        library->entries_.reserve(json.size());
        for (const nlohmann::json& card : json)
        {
            Entry entry;
            if (card.contains("name") && card["name"].is_string())
            {
                entry.name = card["name"].get<std::string>();
            }
            const std::string game = card.value("game", std::string{});
            entry.game = game == "SW" ? Game::Sword : game == "SH" ? Game::Shield : Game::Both;
            library->entries_.push_back(std::move(entry));
        }
        return library;
    }

    std::unique_ptr<pksm::WC8> EventLibrary::card(size_t index) const
    {
        // WC8 copies the bytes it is given.
        return std::make_unique<pksm::WC8>(
            const_cast<u8*>(data_.data() + index * pksm::WC8::length));
    }
}
