#include "platform/AtomicFile.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>

namespace pksm::switchui
{
    bool readFile(const std::string& path, size_t limit, std::vector<u8>& data, bool& missing)
    {
        missing    = false;
        FILE* file = std::fopen(path.c_str(), "rb");
        if (!file)
        {
            missing = errno == ENOENT;
            return false;
        }
        data.clear();
        u8 buffer[16 * 1024];
        bool ok = true;
        while (data.size() < limit)
        {
            const size_t wanted = std::min(sizeof(buffer), limit - data.size());
            const size_t read   = std::fread(buffer, 1, wanted, file);
            data.insert(data.end(), buffer, buffer + read);
            if (read < wanted)
            {
                ok = std::ferror(file) == 0;
                break;
            }
        }
        std::fclose(file);
        return ok;
    }

    bool replaceFile(const std::string& path, const void* data, size_t size, std::string& error)
    {
        const std::string temporary = path + ".tmp";
        FILE* file                  = std::fopen(temporary.c_str(), "wb");
        if (!file)
        {
            error = std::strerror(errno);
            return false;
        }
        const bool written   = std::fwrite(data, 1, size, file) == size;
        const int writeErrno = errno;
        const bool closed    = std::fclose(file) == 0;
        if (!written || !closed)
        {
            error = std::strerror(written ? errno : writeErrno);
            std::remove(temporary.c_str());
            return false;
        }
        const std::string backup = path + ".bak";
        std::remove(backup.c_str());
        if (std::rename(path.c_str(), backup.c_str()) != 0 && errno != ENOENT)
        {
            error = std::strerror(errno);
            std::remove(temporary.c_str());
            return false;
        }
        if (std::rename(temporary.c_str(), path.c_str()) != 0)
        {
            error = std::strerror(errno);
            // Put the old file back so the bank is never missing.
            std::rename(backup.c_str(), path.c_str());
            std::remove(temporary.c_str());
            return false;
        }
        return true;
    }
}
