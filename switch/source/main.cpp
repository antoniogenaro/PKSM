/*
 *   This file is part of PKSM
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
 *       * Requiring preservation of specified reasonable legal notices or
 *         author attributions in that material or in the Appropriate Legal
 *         Notices displayed by works containing it.
 *       * Prohibiting misrepresentation of the origin of that material,
 *         or requiring that modified versions of that material be marked in
 *         reasonable ways as different from the original version.
 */

#include "sav/Sav.hpp"
#include "sav/SavSWSH.hpp"
#include <switch.h>
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <exception>
#include <fcntl.h>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace
{
    constexpr char SAVE_DIRECTORY[] = "sdmc:/switch/pksm";
    constexpr size_t MAX_SAVE_SIZE  = 16 * 1024 * 1024;

    struct LoadedSave
    {
        std::unique_ptr<pksm::Sav> save;
        std::string sourcePath;
    };

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
            errno = 0;
            dirent* entry = readdir(directory);
            if (!entry)
            {
                if (errno != 0)
                {
                    error =
                        std::string("Error al enumerar una carpeta de respaldos: ") +
                        std::strerror(errno);
                    success = false;
                }
                break;
            }
            if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            const std::string path = directoryPath + "/" + entry->d_name;
            const std::string relativePath =
                relativeDirectory.empty() ? entry->d_name : relativeDirectory + "/" + entry->d_name;
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

    std::vector<std::string> listSaveFiles(std::string& error)
    {
        std::vector<std::string> files;
        if (!appendSaveFiles(SAVE_DIRECTORY, "", files, error, 0))
        {
            files.clear();
            return files;
        }
        std::sort(files.begin(), files.end());
        return files;
    }

    bool loadSave(const std::string& path, LoadedSave& loaded, std::string& error)
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

        auto data = std::shared_ptr<u8[]>(new u8[static_cast<size_t>(fileSize)]);
        const size_t bytesRead = std::fread(data.get(), 1, static_cast<size_t>(fileSize), file);
        const bool readError   = std::ferror(file) != 0;
        std::fclose(file);
        if (readError || bytesRead != static_cast<size_t>(fileSize))
        {
            error = "Lectura incompleta del archivo.";
            return false;
        }

        std::unique_ptr<pksm::Sav> save;
        try
        {
            save = pksm::Sav::getSave(data, static_cast<size_t>(fileSize));
        }
        catch (const std::exception& exception)
        {
            error = std::string("No se pudo interpretar el guardado: ") + exception.what();
            return false;
        }
        if (!save || (save->version() != pksm::GameVersion::SW &&
                         save->version() != pksm::GameVersion::SH))
        {
            error = "El archivo no es un guardado valido de Sword/Shield.";
            return false;
        }

        save->beginEditing();
        loaded.save       = std::move(save);
        loaded.sourcePath = path;
        return true;
    }

    bool exportSave(LoadedSave& loaded, std::string& outputPath, std::string& error)
    {
        int descriptor = -1;
        for (unsigned int suffix = 0; suffix < 1000; ++suffix)
        {
            outputPath = loaded.sourcePath + ".pksm-edited";
            if (suffix != 0)
            {
                outputPath += "-" + std::to_string(suffix);
            }
            descriptor = open(outputPath.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);
            if (descriptor >= 0 || errno != EEXIST)
            {
                break;
            }
        }
        if (descriptor < 0)
        {
            error = errno == EEXIST ? "No se encontro un nombre libre para exportar." :
                                      std::string("No se pudo crear la salida: ") +
                                          std::strerror(errno);
            return false;
        }

        loaded.save->finishEditing();
        FILE* file = fdopen(descriptor, "wb");
        if (!file)
        {
            const int openError = errno;
            close(descriptor);
            const bool removed = std::remove(outputPath.c_str()) == 0;
            loaded.save->beginEditing();
            error = std::string("No se pudo abrir la salida: ") + std::strerror(openError);
            if (!removed)
            {
                error += std::string("; tampoco se pudo eliminar el archivo vacio: ") + outputPath;
            }
            return false;
        }

        const size_t length = loaded.save->getEntireLengthIncludingFooter();
        const size_t written = std::fwrite(loaded.save->rawData().get(), 1, length, file);
        const bool writeError = std::ferror(file) != 0;
        const bool closeError = std::fclose(file) != 0;
        loaded.save->beginEditing();
        if (written != length || writeError || closeError)
        {
            if (std::remove(outputPath.c_str()) == 0)
            {
                error = "Escritura incompleta; se elimino la salida parcial.";
            }
            else
            {
                error = std::string("Escritura incompleta y no se pudo eliminar la salida parcial: ") +
                        outputPath;
            }
            return false;
        }
        return true;
    }

    void drawFilePicker(
        const std::vector<std::string>& files, size_t selected, const std::string& storageError)
    {
        consoleClear();
        std::printf("PKSM Switch - respaldo de Sword/Shield\n\n");
        std::printf("Carpeta: %s/\n", SAVE_DIRECTORY);
        std::printf("Busca respaldos en subcarpetas; no se leen guardados activos.\n\n");
        if (!storageError.empty())
        {
            std::printf("%s\n", storageError.c_str());
            return;
        }
        if (files.empty())
        {
            std::printf("No se encontraron archivos.\n");
            return;
        }

        const size_t first = selected > 7 ? selected - 7 : 0;
        const size_t last  = std::min(files.size(), first + 15);
        for (size_t i = first; i < last; ++i)
        {
            std::printf("%s %s\n", i == selected ? ">" : " ", files[i].c_str());
        }
    }
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    consoleInit(nullptr);
    if (R_FAILED(romfsInit()))
    {
        std::printf("No se pudo montar los datos internos de PKSM.\n");
        consoleUpdate(nullptr);
        consoleExit(nullptr);
        return 1;
    }
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    std::string storageError;
    if (mkdir("sdmc:/switch", 0777) != 0 && errno != EEXIST)
    {
        storageError = std::string("No se pudo acceder a la SD: ") + std::strerror(errno);
    }
    else if (mkdir(SAVE_DIRECTORY, 0777) != 0 && errno != EEXIST)
    {
        storageError =
            std::string("No se pudo preparar la carpeta de respaldos: ") + std::strerror(errno);
    }
    std::vector<std::string> files =
        storageError.empty() ? listSaveFiles(storageError) : std::vector<std::string>{};
    size_t selected = 0;

    while (appletMainLoop())
    {
        padUpdate(&pad);
        const u64 buttons = padGetButtonsDown(&pad);
        if (buttons & HidNpadButton_Plus)
        {
            break;
        }
        if (!files.empty() && (buttons & HidNpadButton_Down))
        {
            selected = (selected + 1) % files.size();
        }
        if (!files.empty() && (buttons & HidNpadButton_Up))
        {
            selected = selected == 0 ? files.size() - 1 : selected - 1;
        }

        drawFilePicker(files, selected, storageError);
        if (storageError.empty())
        {
            std::printf("\nA: abrir    +: salir\n");
        }
        consoleUpdate(nullptr);

        if (!files.empty() && (buttons & HidNpadButton_A))
        {
            LoadedSave loaded;
            std::string error;
            const std::string path = std::string(SAVE_DIRECTORY) + "/" + files[selected];
            if (loadSave(path, loaded, error))
            {
                auto& save = static_cast<pksm::SavSWSH&>(*loaded.save);
                bool dirty = false;
                bool finish = false;
                while (appletMainLoop() && !finish)
                {
                    padUpdate(&pad);
                    const u64 editButtons = padGetButtonsDown(&pad);
                    if (editButtons & HidNpadButton_Plus)
                    {
                        if (dirty)
                        {
                            error = "Hay cambios sin exportar; pulsa Y para guardar o X para descartar.";
                        }
                        else
                        {
                            finish = true;
                        }
                    }
                    if ((editButtons & HidNpadButton_Right) && save.money() < 9999999)
                    {
                        save.money(save.money() + 1000);
                        dirty = true;
                        error.clear();
                    }
                    if ((editButtons & HidNpadButton_Left) && save.money() > 0)
                    {
                        save.money(save.money() > 1000 ? save.money() - 1000 : 0);
                        dirty = true;
                        error.clear();
                    }
                    if (editButtons & HidNpadButton_X)
                    {
                        dirty = false;
                        finish = true;
                    }
                    if ((editButtons & HidNpadButton_Y) && dirty)
                    {
                        std::string outputPath;
                        if (exportSave(loaded, outputPath, error))
                        {
                            dirty = false;
                            error = "Exportado a " + outputPath;
                        }
                    }

                    consoleClear();
                    std::printf("PKSM Switch - Pokemon %s\n\n",
                        save.version() == pksm::GameVersion::SW ? "Sword" : "Shield");
                    std::printf("Archivo: %s\n", files[selected].c_str());
                    std::printf("Entrenador: %s\n", save.otName().c_str());
                    std::printf("Dinero: %u\n\n", save.money());
                    if (dirty)
                    {
                        std::printf("Cambios pendientes de exportar.\n");
                    }
                    std::printf("Izq/Der: -/+ 1000 de dinero\n");
                    std::printf("Y: exportar copia    X: descartar y volver\n");
                    std::printf("+: volver al selector\n");
                    if (!error.empty())
                    {
                        std::printf("\n%s\n", error.c_str());
                    }
                    consoleUpdate(nullptr);
                }
            }
            else
            {
                consoleClear();
                std::printf("No se pudo cargar el respaldo:\n\n%s\n\n", error.c_str());
                std::printf("Pulsa A para volver al selector.\n");
                consoleUpdate(nullptr);
                while (appletMainLoop())
                {
                    padUpdate(&pad);
                    if (padGetButtonsDown(&pad) & HidNpadButton_A)
                    {
                        break;
                    }
                }
            }
        }
    }

    consoleExit(nullptr);
    romfsExit();
    return 0;
}
