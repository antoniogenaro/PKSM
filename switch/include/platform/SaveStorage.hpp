#pragma once

#include "utils/coretypes.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

// Save files on the SD card. Errors are returned as user-facing messages.
namespace pksm::switchui::storage
{
    inline constexpr char SAVE_DIRECTORY[] = "sdmc:/switch/pksm";
    inline constexpr size_t MAX_SAVE_SIZE  = 16 * 1024 * 1024;

    // Creates the backup folder when it does not exist yet.
    bool prepareSaveDirectory(std::string& error);

    // Main save files under SAVE_DIRECTORY, relative to it and sorted.
    std::vector<std::string> listSaveFiles(std::string& error);

    // A Switch game whose backups made by save managers are looked for.
    struct ManagedTitle
    {
        uint64_t titleId;
        // The save file inside a backup, such as "main".
        std::string saveFile;
    };

    // The save files of `titles` in the backups made by JKSV and Checkpoint, as full
    // "sdmc:/" paths, sorted. Folders that do not exist are skipped.
    std::vector<std::string> listManagerSaveFiles(const std::vector<ManagedTitle>& titles);

    // The tool that made the backup at `path`, such as "JKSV", or empty for PKSM's own.
    std::string backupTool(const std::string& path);

    // The full path of a save listed by listSaveFiles() or listManagerSaveFiles().
    std::string savePath(const std::string& relativePath);

    // Size and modification time, used to tell whether a file changed since it was last seen.
    bool fileStamp(const std::string& path, uint64_t& size, int64_t& modified);

    // The save identification cache, kept outside SAVE_DIRECTORY so it is never listed as a
    // save. Both are best effort: a missing or unwritable cache only means a slower start.
    std::string readSaveCache();
    void writeSaveCache(const std::string& text);

    bool readSaveFile(
        const std::string& path, std::shared_ptr<u8[]>& data, size_t& size, std::string& error);

    // A new "<source>.pksm-edited[-N]" file, opened for writing. It never replaces an
    // existing file.
    struct ExportFile
    {
        FILE* file = nullptr;
        std::string path;
    };

    bool createExportFile(const std::string& sourcePath, ExportFile& output, std::string& error);

    // Writes and closes the file. On failure the partial file is removed.
    bool writeExportFile(ExportFile& output, const u8* data, size_t length, std::string& error);
}
