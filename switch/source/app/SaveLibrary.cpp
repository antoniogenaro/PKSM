#include "app/SaveLibrary.hpp"

#include "app/Session.hpp"
#include "domain/GameProfile.hpp"
#include "domain/SaveCache.hpp"
#include "platform/ConsoleSaves.hpp"
#include "platform/SaveStorage.hpp"
#include <cstdint>
#include <optional>

namespace pksm::switchui
{
    namespace
    {
        // The profile of a supported save, using the cache when the file did not change.
        const GameProfile* identify(SaveCache& cache, const std::string& file)
        {
            const std::string path = storage::savePath(file);
            SaveCache::Stamp stamp;
            const bool stamped = storage::fileStamp(path, stamp.size, stamp.modified);

            std::optional<int> version = stamped ? cache.lookup(file, stamp) : std::nullopt;
            if (!version)
            {
                const auto identified = Session::identify(path);
                version = identified ? static_cast<int>(static_cast<u8>(*identified))
                                     : SaveCache::UNRECOGNIZED;
                if (stamped)
                {
                    cache.store(file, stamp, *version);
                }
            }
            if (*version < 0 || *version > UINT8_MAX)
            {
                return nullptr;
            }
            return findGameProfile(pksm::GameVersion{static_cast<u8>(*version)});
        }
    }

    SaveLibrary SaveLibrary::scan(const ConsoleUser* user)
    {
        SaveLibrary library;
        const auto profiles = gameProfiles();
        library.filesByGame.resize(profiles.size());
        library.consoleSaveByGame.assign(profiles.size(), false);
        if (user)
        {
            std::vector<uint64_t> titles;
            titles.reserve(profiles.size());
            for (const auto& profile : profiles)
            {
                titles.push_back(profile.consoleTitleId);
            }
            library.consoleSaveByGame = console::savesPresent(titles, *user);
        }

        std::vector<std::string> files = storage::prepareSaveDirectory(library.error)
                                             ? storage::listSaveFiles(library.error)
                                             : std::vector<std::string>{};
        // Then the backups that JKSV and Checkpoint made of the Switch games.
        std::vector<storage::ManagedTitle> titles;
        for (const auto& profile : profiles)
        {
            if (profile.hasConsoleSaves())
            {
                titles.push_back({profile.consoleTitleId, profile.consoleSaveFile});
            }
        }
        for (auto& file : storage::listManagerSaveFiles(titles))
        {
            files.push_back(std::move(file));
        }
        SaveCache cache = SaveCache::parse(storage::readSaveCache());
        for (const auto& file : files)
        {
            if (const GameProfile* profile = identify(cache, file))
            {
                library.filesByGame[static_cast<size_t>(profile - profiles.data())].push_back(file);
            }
        }
        cache.keepOnly(files);
        if (cache.changed())
        {
            storage::writeSaveCache(cache.serialize());
        }
        return library;
    }
}
