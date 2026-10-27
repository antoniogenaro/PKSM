#include "platform/SaveStorage.hpp"

#include "SwitchSaveFiles.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace pksm::switchui::storage
{
    namespace
    {
        bool appendSaveFiles(const std::string& directoryPath, const std::string& relativeDirectory,
            std::vector<std::string>& files, std::string& error, size_t depth)
        {
            constexpr size_t MAX_DIRECTORY_DEPTH = 32;
            if (depth > MAX_DIRECTORY_DEPTH)
            {
                error = "La carpeta de respaldos supera la profundidad maxima de 32 niveles.";
                return false;
            }
            DIR* directory = opendir(directoryPath.c_str());
            if (!directory)
            {
                error = std::string("No se pudo leer una carpeta de respaldos: ") +
                        std::strerror(errno);
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
                        error = std::string("Error al enumerar una carpeta de respaldos: ") +
                                std::strerror(errno);
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
                    error = std::string("No se pudo inspeccionar un archivo de respaldo: ") +
                            std::strerror(errno);
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
                error = std::string("No se pudo cerrar una carpeta de respaldos: ") +
                        std::strerror(errno);
                success = false;
            }
            return success;
        }
    }

    bool prepareSaveDirectory(std::string& error)
    {
        if (mkdir("sdmc:/switch", 0777) != 0 && errno != EEXIST)
        {
            error = std::string("No se pudo acceder a la SD: ") + std::strerror(errno);
            return false;
        }
        if (mkdir(SAVE_DIRECTORY, 0777) != 0 && errno != EEXIST)
        {
            error = std::string("No se pudo preparar la carpeta de respaldos: ") +
                    std::strerror(errno);
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

    std::string savePath(const std::string& relativePath)
    {
        return std::string(SAVE_DIRECTORY) + "/" + relativePath;
    }

    bool readSaveFile(
        const std::string& path, std::shared_ptr<u8[]>& data, size_t& size, std::string& error)
    {
        FILE* file = std::fopen(path.c_str(), "rb");
        if (!file)
        {
            error = "No se pudo abrir el archivo.";
            return false;
        }

        if (std::fseek(file, 0, SEEK_END) != 0)
        {
            std::fclose(file);
            error = "No se pudo leer el tamano del archivo.";
            return false;
        }
        const long fileSize = std::ftell(file);
        if (fileSize <= 0 || static_cast<size_t>(fileSize) > MAX_SAVE_SIZE ||
            std::fseek(file, 0, SEEK_SET) != 0)
        {
            std::fclose(file);
            error = "Tamano de archivo vacio, invalido o superior a 16 MiB.";
            return false;
        }

        auto buffer = std::shared_ptr<u8[]>(new u8[static_cast<size_t>(fileSize)]);
        const size_t bytesRead = std::fread(buffer.get(), 1, static_cast<size_t>(fileSize), file);
        const bool readError   = std::ferror(file) != 0;
        std::fclose(file);
        if (readError || bytesRead != static_cast<size_t>(fileSize))
        {
            error = "Lectura incompleta del archivo.";
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
            error = errno == EEXIST ? "No se encontro un nombre libre para exportar."
                                    : std::string("No se pudo crear la salida: ") +
                                          std::strerror(errno);
            return false;
        }

        FILE* file = fdopen(descriptor, "wb");
        if (!file)
        {
            const int openError = errno;
            close(descriptor);
            const bool removed = std::remove(path.c_str()) == 0;
            error = std::string("No se pudo abrir la salida: ") + std::strerror(openError);
            if (!removed)
            {
                error += std::string("; tampoco se pudo eliminar el archivo vacio: ") + path;
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
                error = "Escritura incompleta; se elimino la salida parcial.";
            }
            else
            {
                error = std::string("Escritura incompleta y no se pudo eliminar la salida parcial: ") +
                        output.path;
            }
            return false;
        }
        return true;
    }
}
