#include "domain/SaveCache.hpp"

#include <algorithm>
#include <charconv>
#include <system_error>
#include <unordered_set>

namespace pksm::switchui
{
    namespace
    {
        // First line of the file. Bump the number when the format changes.
        constexpr std::string_view HEADER = "pksm-switch-saves 1";

        template <typename T>
        bool readField(std::string_view& line, T& value)
        {
            const size_t tab = line.find('\t');
            if (tab == std::string_view::npos)
            {
                return false;
            }
            const auto result = std::from_chars(line.data(), line.data() + tab, value);
            if (result.ec != std::errc{} || result.ptr != line.data() + tab)
            {
                return false;
            }
            line.remove_prefix(tab + 1);
            return true;
        }
    }

    SaveCache SaveCache::parse(std::string_view text)
    {
        SaveCache cache;
        bool first = true;
        while (!text.empty())
        {
            const size_t end      = text.find('\n');
            std::string_view line = text.substr(0, end);
            text.remove_prefix(end == std::string_view::npos ? text.size() : end + 1);
            if (first)
            {
                if (line != HEADER)
                {
                    return SaveCache{};
                }
                first = false;
                continue;
            }

            // size, modified, version, path; the path is last so it may hold tabs.
            Entry entry{};
            if (!readField(line, entry.stamp.size) || !readField(line, entry.stamp.modified) ||
                !readField(line, entry.version) || line.empty())
            {
                continue;
            }
            cache.entries_.insert_or_assign(std::string(line), entry);
        }
        return cache;
    }

    std::string SaveCache::serialize() const
    {
        std::vector<const std::string*> paths;
        paths.reserve(entries_.size());
        for (const auto& [path, entry] : entries_)
        {
            paths.push_back(&path);
        }
        std::sort(paths.begin(), paths.end(),
            [](const std::string* a, const std::string* b) { return *a < *b; });

        std::string text(HEADER);
        text += '\n';
        for (const std::string* path : paths)
        {
            const Entry& entry = entries_.at(*path);
            text += std::to_string(entry.stamp.size) + '\t' +
                    std::to_string(entry.stamp.modified) + '\t' + std::to_string(entry.version) +
                    '\t' + *path + '\n';
        }
        return text;
    }

    std::optional<int> SaveCache::lookup(const std::string& path, const Stamp& stamp) const
    {
        const auto it = entries_.find(path);
        if (it == entries_.end() || it->second.stamp != stamp)
        {
            return std::nullopt;
        }
        return it->second.version;
    }

    void SaveCache::store(const std::string& path, const Stamp& stamp, int version)
    {
        // One entry per line: a path with a line break cannot be stored, so it is rescanned.
        if (path.empty() || path.find('\n') != std::string::npos)
        {
            return;
        }
        const auto it = entries_.find(path);
        if (it != entries_.end() && it->second.stamp == stamp && it->second.version == version)
        {
            return;
        }
        entries_.insert_or_assign(path, Entry{stamp, version});
        changed_ = true;
    }

    void SaveCache::keepOnly(const std::vector<std::string>& paths)
    {
        const std::unordered_set<std::string> keep(paths.begin(), paths.end());
        const size_t before = entries_.size();
        std::erase_if(entries_, [&keep](const auto& item) { return !keep.contains(item.first); });
        changed_ = changed_ || entries_.size() != before;
    }
}
