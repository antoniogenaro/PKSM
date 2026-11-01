#pragma once

#include "utils/coretypes.h"
#include <cstddef>
#include <string>
#include <vector>

namespace pksm::switchui
{
    // Reads at most `limit` bytes of `path` into `data`. Returns false when it cannot be read;
    // `missing` then says whether the file is not there at all.
    bool readFile(const std::string& path, size_t limit, std::vector<u8>& data, bool& missing);

    // Writes `path` aside, keeps the old file as .bak and moves the new one in its place, so a
    // failed write never leaves half a file behind. On failure `error` says why.
    bool replaceFile(const std::string& path, const void* data, size_t size, std::string& error);
}
