#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pksm::switchui
{
    // Remembers which game each backup file belongs to, so the save list does not have to
    // read and parse every file on start. An entry is only trusted while the file keeps the
    // size and modification time it had when it was identified.
    class SaveCache
    {
    public:
        // Stored for files that are not a save PKSM can read.
        static constexpr int UNRECOGNIZED = -1;

        struct Stamp
        {
            uint64_t size     = 0;
            int64_t modified  = 0;
            bool operator==(const Stamp&) const = default;
        };

        // Reads what serialize() wrote. Anything it does not understand is dropped, so a
        // damaged or outdated file only costs a rescan.
        static SaveCache parse(std::string_view text);
        std::string serialize() const;

        // The stored game version, or UNRECOGNIZED; nothing when the file is new or changed.
        std::optional<int> lookup(const std::string& path, const Stamp& stamp) const;
        void store(const std::string& path, const Stamp& stamp, int version);

        // Forgets files that are no longer on the SD card.
        void keepOnly(const std::vector<std::string>& paths);

        bool changed() const { return changed_; }

    private:
        struct Entry
        {
            Stamp stamp;
            int version;
        };

        std::unordered_map<std::string, Entry> entries_;
        bool changed_ = false;
    };
}
