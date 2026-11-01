#include "domain/PokemonBank.hpp"

#include "nlohmann/json.hpp"
#include "platform/AtomicFile.hpp"
#include "pkx/PK1.hpp"
#include "pkx/PK2.hpp"
#include "pkx/PK3.hpp"
#include "pkx/PK4.hpp"
#include "pkx/PK5.hpp"
#include "pkx/PK6.hpp"
#include "pkx/PK7.hpp"
#include "pkx/PB7.hpp"
#include "pkx/PK8.hpp"
#include "pkx/PK9.hpp"
#include "pkx/PKX.hpp"
#include "ui/Text.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <variant>

namespace pksm::switchui
{
    namespace
    {
        // The length of a stored Pokémon of `gen`, as the 3DS bank reads it.
        size_t storedLength(pksm::Generation gen, const u8* data)
        {
            switch (gen)
            {
                case pksm::Generation::ONE:
                {
                    const u8 jpEnd = data[pksm::PK1::JP_LENGTH_WITH_NAMES - 1];
                    return jpEnd == 0x50 || jpEnd == 0 ? pksm::PK1::JP_LENGTH_WITH_NAMES
                                                       : pksm::PK1::INT_LENGTH_WITH_NAMES;
                }
                case pksm::Generation::TWO:
                {
                    const u8 jpEnd = data[pksm::PK2::JP_LENGTH_WITH_NAMES - 1];
                    return jpEnd == 0x50 || jpEnd == 0 ? pksm::PK2::JP_LENGTH_WITH_NAMES
                                                       : pksm::PK2::INT_LENGTH_WITH_NAMES;
                }
                case pksm::Generation::THREE:
                    return pksm::PK3::BOX_LENGTH;
                case pksm::Generation::FOUR:
                    return pksm::PK4::BOX_LENGTH;
                case pksm::Generation::FIVE:
                    return pksm::PK5::BOX_LENGTH;
                case pksm::Generation::SIX:
                    return pksm::PK6::BOX_LENGTH;
                case pksm::Generation::SEVEN:
                    return pksm::PK7::BOX_LENGTH;
                case pksm::Generation::LGPE:
                    return pksm::PB7::BOX_LENGTH;
                case pksm::Generation::EIGHT:
                    return pksm::PK8::BOX_LENGTH;
                case pksm::Generation::NINE:
                    return pksm::PK9::BOX_LENGTH;
                default:
                    return 0;
            }
        }

        std::string defaultBoxName(int box)
        {
            return tr(Text::BankBoxName, {std::to_string(box + 1)});
        }
    }

    std::string PokemonBank::bankPath() const
    {
        return std::string(DIRECTORY) + "/" + DEFAULT_NAME + ".bnk";
    }

    std::string PokemonBank::namesPath() const
    {
        return std::string(DIRECTORY) + "/" + DEFAULT_NAME + ".json";
    }

    std::string PokemonBank::fileName() const
    {
        return std::string(DEFAULT_NAME) + ".bnk";
    }

    std::unique_ptr<PokemonBank> PokemonBank::open(std::string& error)
    {
        std::unique_ptr<PokemonBank> bank(new PokemonBank());

        std::vector<u8> raw;
        bool missing = false;
        if (readFile(bank->bankPath(), BankFile::MAX_SIZE, raw, missing))
        {
            auto parsed = BankFile::parse(raw);
            if (auto* contents = std::get_if<BankFile::Contents>(&parsed))
            {
                // An older format is rewritten in the current one with the next save.
                bank->entries_ = std::move(contents->entries);
            }
            else
            {
                const BankFile::Error reason = std::get<BankFile::Error>(parsed);
                error = tr(reason == BankFile::Error::NewerVersion ? Text::BankNewerVersion
                                                                   : Text::BankCorrupt,
                    {bank->bankPath()});
                return nullptr;
            }
        }
        else if (missing)
        {
            bank->entries_.assign(size_t(DEFAULT_BOXES) * SLOTS, BankFile::emptyEntry());
        }
        else
        {
            error = tr(Text::BankOpenFailed, {std::strerror(errno)});
            return nullptr;
        }

        // Box names are a convenience: a missing or broken file only means default names.
        std::vector<u8> names;
        if (readFile(bank->namesPath(), 1024 * 1024, names, missing))
        {
            const nlohmann::json json = nlohmann::json::parse(names.begin(),
                std::find(names.begin(), names.end(), u8(0)), nullptr, false);
            if (json.is_array())
            {
                for (const auto& name : json)
                {
                    bank->boxNames_.push_back(name.is_string() ? name.get<std::string>() : "");
                }
            }
        }
        for (int box = 0; box < bank->boxes(); ++box)
        {
            if (size_t(box) >= bank->boxNames_.size())
            {
                bank->boxNames_.push_back(defaultBoxName(box));
            }
            else if (bank->boxNames_[box].empty())
            {
                bank->boxNames_[box] = defaultBoxName(box);
            }
        }
        bank->boxNames_.resize(size_t(bank->boxes()));
        return bank;
    }

    std::string PokemonBank::boxName(int box) const
    {
        return boxNames_[size_t(box)];
    }

    bool PokemonBank::empty(int box, int slot) const
    {
        return entries_[size_t(box * SLOTS + slot)].gen == pksm::Generation::UNUSED;
    }

    std::unique_ptr<pksm::PKX> PokemonBank::pkm(int box, int slot) const
    {
        const BankFile::Entry& entry = entries_[size_t(box * SLOTS + slot)];
        if (entry.gen == pksm::Generation::UNUSED)
        {
            return nullptr;
        }
        const size_t length = storedLength(entry.gen, entry.data);
        if (length == 0)
        {
            return nullptr;
        }
        // getPKM copies the data; it only takes it as non-const.
        auto pokemon =
            pksm::PKX::getPKM(entry.gen, const_cast<u8*>(entry.data), length);
        if (!pokemon || pokemon->species() == pksm::Species::None)
        {
            return nullptr;
        }
        return pokemon;
    }

    void PokemonBank::pkm(const pksm::PKX& pokemon, int box, int slot)
    {
        if (pokemon.species() == pksm::Species::None)
        {
            clear(box, slot);
            return;
        }
        BankFile::Entry entry = BankFile::emptyEntry();
        entry.gen             = pokemon.generation();
        const auto data       = pokemon.rawData();
        std::copy_n(data.begin(), std::min(data.size(), sizeof(entry.data)), entry.data);
        entries_[size_t(box * SLOTS + slot)] = entry;
        dirty_                               = true;
    }

    void PokemonBank::clear(int box, int slot)
    {
        entries_[size_t(box * SLOTS + slot)] = BankFile::emptyEntry();
        dirty_                               = true;
    }

    bool PokemonBank::save(std::string& error)
    {
        if (mkdir(DIRECTORY, 0777) != 0 && errno != EEXIST)
        {
            error = tr(Text::BankWriteFailed, {std::strerror(errno)});
            return false;
        }

        BankFile::Header header;
        std::memcpy(header.MAGIC, BankFile::MAGIC.data(), BankFile::MAGIC.size());
        header.version = BankFile::VERSION;
        header.boxes   = u32(boxes());
        std::vector<u8> bytes(sizeof(header) + entries_.size() * sizeof(BankFile::Entry));
        std::memcpy(bytes.data(), &header, sizeof(header));
        std::memcpy(bytes.data() + sizeof(header), entries_.data(),
            entries_.size() * sizeof(BankFile::Entry));

        std::string reason;
        if (!replaceFile(bankPath(), bytes.data(), bytes.size(), reason))
        {
            error = tr(Text::BankWriteFailed, {reason});
            return false;
        }
        dirty_ = false;

        // The 3DS writes the names with a trailing NUL; it reads them either way.
        const std::string names = nlohmann::json(boxNames_).dump(2);
        if (!replaceFile(namesPath(), names.c_str(), names.size() + 1, reason))
        {
            error = tr(Text::BankWriteFailed, {reason});
            return false;
        }
        return true;
    }
}
