#include "domain/PokedexFile.hpp"

#include "domain/PokemonBank.hpp"
#include "pkx/PKX.hpp"
#include "platform/AtomicFile.hpp"
#include "ui/Text.hpp"
#include <cerrno>
#include <cstring>
#include <ctime>
#include <sys/stat.h>

namespace pksm::switchui::dex
{
    namespace
    {
        constexpr size_t MAX_RECORDS_SIZE = 16 * 1024 * 1024;
    }

    bool loadRecords(Records& records, std::string& error)
    {
        std::vector<u8> data;
        bool missing = false;
        if (!readFile(RECORDS_PATH, MAX_RECORDS_SIZE, data, missing))
        {
            if (missing)
            {
                records = Records{};
                return true;
            }
            error = tr(Text::DexReadFailed, {std::strerror(errno)});
            return false;
        }
        auto parsed = Records::fromJson(
            std::string_view(reinterpret_cast<const char*>(data.data()), data.size()));
        if (!parsed)
        {
            error = tr(Text::DexReadFailed, {RECORDS_PATH});
            return false;
        }
        records = std::move(*parsed);
        return true;
    }

    bool registerBank(const PokemonBank& bank, std::string& error)
    {
        Records records;
        if (!loadRecords(records, error))
        {
            // Never write over records that could not be read.
            return false;
        }

        const std::string date = today();
        bool added             = false;
        for (int box = 0; box < bank.boxes(); ++box)
        {
            for (int slot = 0; slot < PokemonBank::SLOTS; ++slot)
            {
                const auto pokemon = bank.pkm(box, slot);
                if (!pokemon || pokemon->egg())
                {
                    continue;
                }
                const auto game    = gameOf(pokemon->generation(), pokemon->version());
                const auto species = static_cast<uint16_t>(u16(pokemon->species()));
                if (!game || !hasForm(*game, species, pokemon->alternativeForm()))
                {
                    continue;
                }
                added |=
                    records.add({*game, species, static_cast<uint8_t>(pokemon->alternativeForm()),
                                    pokemon->shiny()},
                        date);
            }
        }
        if (!added)
        {
            return true;
        }

        if (mkdir(PokemonBank::DIRECTORY, 0777) != 0 && errno != EEXIST)
        {
            error = tr(Text::DexWriteFailed, {std::strerror(errno)});
            return false;
        }
        const std::string json = records.toJson();
        std::string reason;
        if (!replaceFile(RECORDS_PATH, json.data(), json.size(), reason))
        {
            error = tr(Text::DexWriteFailed, {reason});
            return false;
        }
        return true;
    }

    std::string today()
    {
        const std::time_t now = std::time(nullptr);
        std::tm local{};
        if (!localtime_r(&now, &local))
        {
            return {};
        }
        char text[16];
        std::strftime(text, sizeof(text), "%Y-%m-%d", &local);
        return text;
    }
}
