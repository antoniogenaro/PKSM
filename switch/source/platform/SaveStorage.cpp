#include "platform/SaveStorage.hpp"

#include "SwitchSaveFiles.hpp"
#include "nlohmann/json.hpp"
#include "ui/Text.hpp"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cinttypes>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace pksm::switchui::storage
{
    namespace
    {
        constexpr char CONFIG_DIRECTORY[]     = "sdmc:/config";
        constexpr char APP_CONFIG_DIRECTORY[] = "sdmc:/config/pksm-switch";
        constexpr char SAVE_CACHE_PATH[]      = "sdmc:/config/pksm-switch/saves.cache";

        constexpr char SD_PREFIX[]         = "sdmc:/";
        constexpr char JKSV_DEFAULT_ROOT[] = "sdmc:/JKSV";
        constexpr char JKSV_DIR_FILE[]     = "sdmc:/switch/jksv_dir.txt";
        constexpr char JKSV_CONFIG_FILE[]  = "sdmc:/config/JKSV/JKSV.json";
        constexpr char CHECKPOINT_ROOT[]   = "sdmc:/switch/Checkpoint/saves";
        constexpr char CHECKPOINT_MARKER[] = "/switch/Checkpoint/";

        std::string readSmallTextFile(const char* path)
        {
            std::string text;
            FILE* file = std::fopen(path, "rb");
            if (!file)
            {
                return text;
            }
            char buffer[1024];
            size_t read = 0;
            while (text.size() < 64 * 1024 &&
                   (read = std::fread(buffer, 1, sizeof(buffer), file)) > 0)
            {
                text.append(buffer, read);
            }
            std::fclose(file);
            return text;
        }

        // "sdmc:/x/" or "/x" as "sdmc:/x"; empty when it is not a folder on the SD card.
        std::string sdFolder(std::string path)
        {
            const size_t start = path.find_first_not_of(" \t\r\n");
            const size_t end   = path.find_last_not_of(" \t\r\n");
            path = start == std::string::npos ? std::string{} : path.substr(start, end - start + 1);
            if (!path.empty() && path.front() == '/')
            {
                path = "sdmc:" + path;
            }
            if (path.rfind(SD_PREFIX, 0) != 0)
            {
                return {};
            }
            while (path.size() > sizeof(SD_PREFIX) - 1 && path.back() == '/')
            {
                path.pop_back();
            }
            return path;
        }

        // The folders JKSV may keep its backups in: its default one and the one set in its
        // settings, by older versions in jksv_dir.txt and by newer ones in JKSV.json.
        std::vector<std::string> jksvRoots()
        {
            std::vector<std::string> roots{JKSV_DEFAULT_ROOT};
            auto add = [&roots](std::string folder)
            {
                folder = sdFolder(std::move(folder));
                if (!folder.empty() &&
                    std::find(roots.begin(), roots.end(), folder) == roots.end())
                {
                    roots.push_back(std::move(folder));
                }
            };
            add(readSmallTextFile(JKSV_DIR_FILE));
            const nlohmann::json config =
                nlohmann::json::parse(readSmallTextFile(JKSV_CONFIG_FILE), nullptr, false);
            if (config.is_object())
            {
                const auto found = config.find("WorkingDirectory");
                if (found != config.end() && found->is_string())
                {
                    add(found->get<std::string>());
                }
            }
            return roots;
        }

        // The names of the folders inside `path`; none when it cannot be read.
        std::vector<std::string> subfolders(const std::string& path)
        {
            std::vector<std::string> names;
            DIR* directory = opendir(path.c_str());
            if (!directory)
            {
                return names;
            }
            while (dirent* entry = readdir(directory))
            {
                if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0)
                {
                    continue;
                }
                struct stat info
                {
                };
                const std::string child = path + "/" + entry->d_name;
                if (stat(child.c_str(), &info) == 0 && S_ISDIR(info.st_mode))
                {
                    names.emplace_back(entry->d_name);
                }
            }
            closedir(directory);
            return names;
        }

        bool isFile(const std::string& path)
        {
            struct stat info
            {
            };
            return stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode);
        }

        // Adds "<gameFolder>/<backup>/<save file>" for every backup that has the file.
        void appendBackups(const std::string& gameFolder,
            const std::vector<std::string>& saveFiles, std::vector<std::string>& files)
        {
            for (const auto& backup : subfolders(gameFolder))
            {
                for (const auto& saveFile : saveFiles)
                {
                    const std::string path = gameFolder + "/" + backup + "/" + saveFile;
                    if (isFile(path))
                    {
                        files.push_back(path);
                    }
                }
            }
        }

        bool startsWithNoCase(const std::string& text, const std::string& prefix)
        {
            return text.size() >= prefix.size() &&
                   std::equal(prefix.begin(), prefix.end(), text.begin(),
                       [](char a, char b)
                       {
                           return std::tolower(static_cast<unsigned char>(a)) ==
                                  std::tolower(static_cast<unsigned char>(b));
                       });
        }

        bool appendSaveFiles(const std::string& directoryPath, const std::string& relativeDirectory,
            std::vector<std::string>& files, std::string& error, size_t depth)
        {
            constexpr size_t MAX_DIRECTORY_DEPTH = 32;
            if (depth > MAX_DIRECTORY_DEPTH)
            {
                error = tr(Text::DirectoryTooDeep);
                return false;
            }
            DIR* directory = opendir(directoryPath.c_str());
            if (!directory)
            {
                error = tr(Text::DirectoryReadFailed, {std::strerror(errno)});
                return false;
            }

            bool success = true;
            while (true)
            {
                errno         = 0;
                dirent* entry = readdir(directory);
                if (!entry)
                {
                    if (errno != 0)
                    {
                        error = tr(Text::DirectoryListFailed, {std::strerror(errno)});
                        success = false;
                    }
                    break;
                }
                if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0)
                {
                    continue;
                }

                const std::string path         = directoryPath + "/" + entry->d_name;
                const std::string relativePath = relativeDirectory.empty()
                                                     ? entry->d_name
                                                     : relativeDirectory + "/" + entry->d_name;
                struct stat info
                {
                };
                if (lstat(path.c_str(), &info) != 0)
                {
                    error = tr(Text::FileInspectFailed, {std::strerror(errno)});
                    success = false;
                    break;
                }
                if (S_ISDIR(info.st_mode))
                {
                    if (!appendSaveFiles(path, relativePath, files, error, depth + 1))
                    {
                        success = false;
                        break;
                    }
                }
                else if (S_ISREG(info.st_mode))
                {
                    files.push_back(relativePath);
                }
            }
            if (closedir(directory) != 0 && success)
            {
                error = tr(Text::DirectoryCloseFailed, {std::strerror(errno)});
                success = false;
            }
            return success;
        }
    }

    bool prepareSaveDirectory(std::string& error)
    {
        if (mkdir("sdmc:/switch", 0777) != 0 && errno != EEXIST)
        {
            error = tr(Text::SdAccessFailed, {std::strerror(errno)});
            return false;
        }
        if (mkdir(SAVE_DIRECTORY, 0777) != 0 && errno != EEXIST)
        {
            error = tr(Text::SaveDirectoryFailed, {std::strerror(errno)});
            return false;
        }
        return true;
    }

    std::vector<std::string> listSaveFiles(std::string& error)
    {
        std::vector<std::string> files;
        if (!appendSaveFiles(SAVE_DIRECTORY, "", files, error, 0))
        {
            files.clear();
            return files;
        }
        files = filterMainSaveFiles(std::move(files));
        std::sort(files.begin(), files.end());
        return files;
    }

    std::vector<std::string> listManagerSaveFiles(const std::vector<ManagedTitle>& titles)
    {
        std::vector<std::string> saveFiles;
        for (const auto& title : titles)
        {
            std::string name = title.saveFile;
            while (!name.empty() && name.front() == '/')
            {
                name.erase(name.begin());
            }
            if (!name.empty() &&
                std::find(saveFiles.begin(), saveFiles.end(), name) == saveFiles.end())
            {
                saveFiles.push_back(std::move(name));
            }
        }
        std::vector<std::string> files;
        if (saveFiles.empty())
        {
            return files;
        }

        // JKSV: <root>/<game name>/<user - date>/. The game folder is named after the game
        // in the console language, so every game is looked at; files that are not a
        // supported save are left out when they are identified.
        for (const auto& root : jksvRoots())
        {
            for (const auto& game : subfolders(root))
            {
                appendBackups(root + "/" + game, saveFiles, files);
            }
        }

        // Checkpoint: saves/0x<title ID> <game name>/<backup name>/.
        for (const auto& game : subfolders(CHECKPOINT_ROOT))
        {
            for (const auto& title : titles)
            {
                char prefix[24];
                std::snprintf(prefix, sizeof(prefix), "0x%016" PRIX64, title.titleId);
                if (startsWithNoCase(game, prefix))
                {
                    appendBackups(std::string(CHECKPOINT_ROOT) + "/" + game, saveFiles, files);
                    break;
                }
            }
        }

        std::sort(files.begin(), files.end());
        files.erase(std::unique(files.begin(), files.end()), files.end());
        return files;
    }

    std::string backupTool(const std::string& path)
    {
        if (path.rfind(SD_PREFIX, 0) != 0)
        {
            return {};
        }
        return path.find(CHECKPOINT_MARKER) != std::string::npos ? "Checkpoint" : "JKSV";
    }

    std::string savePath(const std::string& relativePath)
    {
        if (relativePath.rfind(SD_PREFIX, 0) == 0)
        {
            return relativePath;
        }
        return std::string(SAVE_DIRECTORY) + "/" + relativePath;
    }

    bool fileStamp(const std::string& path, uint64_t& size, int64_t& modified)
    {
        struct stat info
        {
        };
        if (stat(path.c_str(), &info) != 0)
        {
            return false;
        }
        size     = static_cast<uint64_t>(info.st_size);
        modified = static_cast<int64_t>(info.st_mtime);
        return true;
    }

    std::string readSaveCache()
    {
        std::string text;
        FILE* file = std::fopen(SAVE_CACHE_PATH, "rb");
        if (!file)
        {
            return text;
        }
        char buffer[4096];
        size_t read = 0;
        while ((read = std::fread(buffer, 1, sizeof(buffer), file)) > 0)
        {
            text.append(buffer, read);
        }
        const bool failed = std::ferror(file) != 0;
        std::fclose(file);
        return failed ? std::string{} : text;
    }

    void writeSaveCache(const std::string& text)
    {
        if ((mkdir(CONFIG_DIRECTORY, 0777) != 0 && errno != EEXIST) ||
            (mkdir(APP_CONFIG_DIRECTORY, 0777) != 0 && errno != EEXIST))
        {
            return;
        }
        // Written aside and then moved over the old cache, so a crash never leaves half a file.
        const std::string temporary = std::string(SAVE_CACHE_PATH) + ".tmp";
        FILE* file                  = std::fopen(temporary.c_str(), "wb");
        if (!file)
        {
            return;
        }
        const bool written = std::fwrite(text.data(), 1, text.size(), file) == text.size();
        const bool closed  = std::fclose(file) == 0;
        if (!written || !closed)
        {
            std::remove(temporary.c_str());
            return;
        }
        std::remove(SAVE_CACHE_PATH);
        if (std::rename(temporary.c_str(), SAVE_CACHE_PATH) != 0)
        {
            std::remove(temporary.c_str());
        }
    }

    bool readSaveFile(
        const std::string& path, std::shared_ptr<u8[]>& data, size_t& size, std::string& error)
    {
        FILE* file = std::fopen(path.c_str(), "rb");
        if (!file)
        {
            error = tr(Text::FileOpenFailed);
            return false;
        }

        if (std::fseek(file, 0, SEEK_END) != 0)
        {
            std::fclose(file);
            error = tr(Text::FileSizeFailed);
            return false;
        }
        const long fileSize = std::ftell(file);
        if (fileSize <= 0 || static_cast<size_t>(fileSize) > MAX_SAVE_SIZE ||
            std::fseek(file, 0, SEEK_SET) != 0)
        {
            std::fclose(file);
            error = tr(Text::FileSizeInvalid);
            return false;
        }

        auto buffer = std::shared_ptr<u8[]>(new u8[static_cast<size_t>(fileSize)]);
        const size_t bytesRead = std::fread(buffer.get(), 1, static_cast<size_t>(fileSize), file);
        const bool readError   = std::ferror(file) != 0;
        std::fclose(file);
        if (readError || bytesRead != static_cast<size_t>(fileSize))
        {
            error = tr(Text::FileReadIncomplete);
            return false;
        }

        data = std::move(buffer);
        size = static_cast<size_t>(fileSize);
        return true;
    }

    bool createExportFile(const std::string& sourcePath, ExportFile& output, std::string& error)
    {
        int descriptor = -1;
        std::string path;
        for (unsigned int suffix = 0; suffix < 1000; ++suffix)
        {
            path = sourcePath + ".pksm-edited";
            if (suffix != 0)
            {
                path += "-" + std::to_string(suffix);
            }
            descriptor = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);
            if (descriptor >= 0 || errno != EEXIST)
            {
                break;
            }
        }
        if (descriptor < 0)
        {
            error = errno == EEXIST ? std::string(tr(Text::ExportNoFreeName))
                                    : tr(Text::ExportCreateFailed, {std::strerror(errno)});
            return false;
        }

        FILE* file = fdopen(descriptor, "wb");
        if (!file)
        {
            const int openError = errno;
            close(descriptor);
            const bool removed = std::remove(path.c_str()) == 0;
            error = tr(Text::ExportOpenFailed, {std::strerror(openError)});
            if (!removed)
            {
                error += tr(Text::ExportEmptyNotRemoved, {path});
            }
            return false;
        }

        output.file = file;
        output.path = std::move(path);
        return true;
    }

    bool writeExportFile(ExportFile& output, const u8* data, size_t length, std::string& error)
    {
        const size_t written  = std::fwrite(data, 1, length, output.file);
        const bool writeError = std::ferror(output.file) != 0;
        const bool closeError = std::fclose(output.file) != 0;
        output.file           = nullptr;
        if (written != length || writeError || closeError)
        {
            if (std::remove(output.path.c_str()) == 0)
            {
                error = tr(Text::ExportPartialRemoved);
            }
            else
            {
                error = tr(Text::ExportPartialKept, {output.path});
            }
            return false;
        }
        return true;
    }
}
