#include "app/SaveLibrary.hpp"

#include "app/Session.hpp"
#include "domain/GameProfile.hpp"
#include "domain/SaveCache.hpp"
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

    SaveLibrary SaveLibrary::scan()
    {
        SaveLibrary library;
        const auto profiles = gameProfiles();
        library.filesByGame.resize(profiles.size());

        const std::vector<std::string> files = storage::prepareSaveDirectory(library.error)
                                                   ? storage::listSaveFiles(library.error)
                                                   : std::vector<std::string>{};
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
