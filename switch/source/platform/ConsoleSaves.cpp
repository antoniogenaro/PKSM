#include "platform/ConsoleSaves.hpp"

#include "platform/SaveStorage.hpp"
#include "ui/Text.hpp"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iterator>
#include <sys/stat.h>
#include <unistd.h>

namespace pksm::switchui::console
{
    namespace
    {
        // A result code as the console shows it, such as "2002-0001".
        std::string resultCode(Result result)
        {
            char code[16];
            std::snprintf(code, sizeof(code), "%04u-%04u", 2000 + R_MODULE(result),
                R_DESCRIPTION(result));
            return code;
        }

        bool sameUser(const AccountUid& a, const AccountUid& b)
        {
            return a.uid[0] == b.uid[0] && a.uid[1] == b.uid[1];
        }

        // A user's save data of one game, open for the length of a call. Changes are kept only
        // when commit() succeeds.
        class SaveData
        {
        public:
            explicit SaveData(const SaveLocation& location)
                : result_(fsOpen_SaveData(&fs_, location.titleId, location.user.uid))
            {
            }
            ~SaveData()
            {
                if (R_SUCCEEDED(result_))
                {
                    fsFsClose(&fs_);
                }
            }
            SaveData(const SaveData&)            = delete;
            SaveData& operator=(const SaveData&) = delete;

            bool open(std::string& error) const
            {
                if (R_FAILED(result_))
                {
                    error = tr(Text::ConsoleSaveOpenFailed, {resultCode(result_)});
                    return false;
                }
                return true;
            }

            FsFileSystem* fs() { return &fs_; }

        private:
            FsFileSystem fs_{};
            Result result_;
        };

        // The console reads a whole FS_MAX_PATH buffer for every path.
        struct FsPath
        {
            explicit FsPath(const std::string& path)
            {
                std::strncpy(text, path.c_str(), sizeof(text) - 1);
            }
            char text[FS_MAX_PATH]{};
        };

        bool readFile(FsFileSystem* fs, const std::string& path, std::shared_ptr<u8[]>& data,
            size_t& size, std::string& error)
        {
            const FsPath fsPath(path);
            FsFile file;
            Result result = fsFsOpenFile(fs, fsPath.text, FsOpenMode_Read, &file);
            if (R_FAILED(result))
            {
                error = tr(Text::ConsoleSaveReadFailed, {resultCode(result)});
                return false;
            }
            s64 fileSize = 0;
            result       = fsFileGetSize(&file, &fileSize);
            if (R_FAILED(result) || fileSize <= 0 ||
                static_cast<uint64_t>(fileSize) > storage::MAX_SAVE_SIZE)
            {
                fsFileClose(&file);
                error = R_FAILED(result) ? tr(Text::ConsoleSaveReadFailed, {resultCode(result)})
                                         : std::string(tr(Text::FileSizeInvalid));
                return false;
            }

            auto buffer = std::shared_ptr<u8[]>(new u8[static_cast<size_t>(fileSize)]);
            u64 total   = 0;
            while (total < static_cast<u64>(fileSize))
            {
                u64 read = 0;
                result   = fsFileRead(&file, static_cast<s64>(total), buffer.get() + total,
                      static_cast<u64>(fileSize) - total, FsReadOption_None, &read);
                if (R_FAILED(result) || read == 0)
                {
                    break;
                }
                total += read;
            }
            fsFileClose(&file);
            if (total != static_cast<u64>(fileSize))
            {
                error = R_FAILED(result) ? tr(Text::ConsoleSaveReadFailed, {resultCode(result)})
                                         : std::string(tr(Text::FileReadIncomplete));
                return false;
            }
            data = std::move(buffer);
            size = static_cast<size_t>(fileSize);
            return true;
        }

        // The user's name as a folder name the SD card accepts.
        std::string folderName(const std::string& name)
        {
            std::string folder;
            for (const char c : name)
            {
                const bool reserved = static_cast<unsigned char>(c) < 0x20 ||
                                      std::strchr("\\/:*?\"<>|", c) != nullptr;
                folder += reserved ? '_' : c;
            }
            while (!folder.empty() && (folder.back() == ' ' || folder.back() == '.'))
            {
                folder.pop_back();
            }
            return folder.empty() ? std::string("user") : folder;
        }

        // Writes `data` to a new "<backupFolder>/<user> - <date>/<file name>" on the SD card.
        bool writeBackup(const SaveLocation& location, const u8* data, size_t size,
            std::string& path, std::string& error)
        {
            if (!storage::prepareSaveDirectory(error))
            {
                return false;
            }
            const std::string gameFolder = storage::savePath(location.backupFolder);
            if (mkdir(gameFolder.c_str(), 0777) != 0 && errno != EEXIST)
            {
                error = tr(Text::ConsoleBackupFailed, {std::strerror(errno)});
                return false;
            }

            const std::time_t now = std::time(nullptr);
            char stamp[32]        = "0000-00-00_00-00-00";
            if (const std::tm* local = std::localtime(&now))
            {
                std::strftime(stamp, sizeof(stamp), "%Y-%m-%d_%H-%M-%S", local);
            }
            const std::string folder =
                gameFolder + "/" + folderName(location.user.name) + " - " + stamp;
            // A new folder every time, so an earlier backup is never replaced.
            if (mkdir(folder.c_str(), 0777) != 0)
            {
                error = tr(Text::ConsoleBackupFailed, {std::strerror(errno)});
                return false;
            }

            const size_t slash = location.file.find_last_of('/');
            path = folder + "/" +
                   (slash == std::string::npos ? location.file : location.file.substr(slash + 1));
            FILE* file = std::fopen(path.c_str(), "wb");
            if (!file)
            {
                error = tr(Text::ConsoleBackupFailed, {std::strerror(errno)});
                rmdir(folder.c_str());
                return false;
            }
            const bool written = std::fwrite(data, 1, size, file) == size;
            const bool closed  = std::fclose(file) == 0;
            if (!written || !closed)
            {
                error = tr(Text::ConsoleBackupFailed, {std::strerror(errno)});
                std::remove(path.c_str());
                rmdir(folder.c_str());
                return false;
            }
            return true;
        }
    }

    std::vector<bool> savesPresent(std::span<const uint64_t> titleIds, const ConsoleUser& user)
    {
        std::vector<bool> present(titleIds.size(), false);
        FsSaveDataInfoReader reader;
        if (R_FAILED(fsOpenSaveDataInfoReader(&reader, FsSaveDataSpaceId_User)))
        {
            return present;
        }
        FsSaveDataInfo infos[32];
        while (true)
        {
            s64 count = 0;
            if (R_FAILED(fsSaveDataInfoReaderRead(&reader, infos, std::size(infos), &count)) ||
                count <= 0)
            {
                break;
            }
            for (s64 i = 0; i < count; ++i)
            {
                const FsSaveDataInfo& info = infos[i];
                if (info.save_data_type != FsSaveDataType_Account || !sameUser(info.uid, user.uid))
                {
                    continue;
                }
                for (size_t title = 0; title < titleIds.size(); ++title)
                {
                    if (titleIds[title] != 0 && info.application_id == titleIds[title])
                    {
                        present[title] = true;
                    }
                }
            }
        }
        fsSaveDataInfoReaderClose(&reader);
        return present;
    }

    bool readSave(const SaveLocation& location, std::shared_ptr<u8[]>& data, size_t& size,
        std::string& error)
    {
        SaveData save(location);
        return save.open(error) && readFile(save.fs(), location.file, data, size, error);
    }

    bool writeSave(const SaveLocation& location, const u8* data, size_t size,
        std::string& backupPath, std::string& error)
    {
        SaveData save(location);
        if (!save.open(error))
        {
            return false;
        }

        // The save as it is now, so it can be put back by hand.
        std::shared_ptr<u8[]> current;
        size_t currentSize = 0;
        if (!readFile(save.fs(), location.file, current, currentSize, error) ||
            !writeBackup(location, current.get(), currentSize, backupPath, error))
        {
            return false;
        }

        // Nothing reaches the console until the commit, so a failure leaves it as it was.
        const FsPath fsPath(location.file);
        FsFile file;
        Result result = fsFsOpenFile(save.fs(), fsPath.text, FsOpenMode_Write, &file);
        if (R_SUCCEEDED(result))
        {
            result = fsFileSetSize(&file, static_cast<s64>(size));
            if (R_SUCCEEDED(result))
            {
                result = fsFileWrite(&file, 0, data, size, FsWriteOption_Flush);
            }
            fsFileClose(&file);
        }
        if (R_SUCCEEDED(result))
        {
            result = fsFsCommit(save.fs());
        }
        if (R_FAILED(result))
        {
            error = tr(Text::ConsoleSaveWriteFailed, {resultCode(result)});
            return false;
        }
        return true;
    }
}
