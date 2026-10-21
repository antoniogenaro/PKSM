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
#include "SwitchSaveFiles.hpp"
#include "SwitchUi.hpp"
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

    struct RomfsCleanup
    {
        ~RomfsCleanup()
        {
            romfsExit();
        }
    };

    struct LoadedSave
    {
        std::unique_ptr<pksm::Sav> save;
        std::string sourcePath;
    };

    bool isSupportedVersion(pksm::GameVersion version)
    {
        return version == pksm::GameVersion::SW || version == pksm::GameVersion::SH ||
               version == pksm::GameVersion::FR || version == pksm::GameVersion::LG;
    }

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
        files = pksm::switchui::filterMainSaveFiles(std::move(files));
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
        if (!save || !isSupportedVersion(save->version()))
        {
            error = "El archivo no es un guardado compatible de Sword/Shield o Rojo Fuego/Verde Hoja.";
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

}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    if (R_FAILED(romfsInit()))
    {
        return 1;
    }
    RomfsCleanup romfsCleanup;
    pksm::switchui::SwitchUi ui;
    std::string uiError;
    if (!ui.initialize(uiError))
    {
        SDL_Log("%s", uiError.c_str());
        return 1;
    }

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
    bool quitRequested = false;

    while (appletMainLoop() && !quitRequested)
    {
        const auto action = ui.pollAction();
        if (action == pksm::switchui::Action::Quit)
        {
            break;
        }
        if (!files.empty() && action == pksm::switchui::Action::Down)
        {
            selected = (selected + 1) % files.size();
        }
        if (!files.empty() && action == pksm::switchui::Action::Up)
        {
            selected = selected == 0 ? files.size() - 1 : selected - 1;
        }

        ui.drawFiles(files, selected, storageError);
        SDL_Delay(16);

        if (!files.empty() && action == pksm::switchui::Action::Confirm)
        {
            LoadedSave loaded;
            std::string error;
            const std::string path = std::string(SAVE_DIRECTORY) + "/" + files[selected];
            if (loadSave(path, loaded, error))
            {
                auto& save = *loaded.save;
                const pksm::GameVersion version = save.version();
                const bool hasBattlePoints = version == pksm::GameVersion::SW ||
                                             version == pksm::GameVersion::SH;
                const size_t hoursField = hasBattlePoints ? 5 : 4;
                const size_t fieldCount = hasBattlePoints ? 8 : 7;
                const size_t nameLength =
                    version == pksm::GameVersion::FR || version == pksm::GameVersion::LG
                        ? (save.language() == pksm::Language::JPN ? 5 : 7)
                        : 12;
                const char* gameName =
                    version == pksm::GameVersion::SW ? "Pokemon Sword" :
                    version == pksm::GameVersion::SH ? "Pokemon Shield" :
                                                       "Pokemon Rojo Fuego / Verde Hoja";
                const std::string details =
                    "Entrenador: " + save.otName() + "    TID/SID: " +
                    std::to_string(save.displayTID()) + "/" +
                    std::to_string(save.displaySID());
                bool finish = false;
                size_t selectedMenuOption = 0;
                std::string menuMessage;
                while (appletMainLoop() && !finish && !quitRequested)
                {
                    const auto menuAction = ui.pollAction();
                    if (menuAction == pksm::switchui::Action::Quit)
                    {
                        finish = true;
                        quitRequested = true;
                    }
                    else if (menuAction == pksm::switchui::Action::Back)
                    {
                        finish = true;
                    }
                    else if (menuAction == pksm::switchui::Action::Up)
                    {
                        selectedMenuOption =
                            selectedMenuOption < 2 ? selectedMenuOption + 4 :
                                                     selectedMenuOption - 2;
                        menuMessage.clear();
                    }
                    else if (menuAction == pksm::switchui::Action::Down)
                    {
                        selectedMenuOption = (selectedMenuOption + 2) % 6;
                        menuMessage.clear();
                    }
                    else if (menuAction == pksm::switchui::Action::Left &&
                             selectedMenuOption % 2 != 0)
                    {
                        --selectedMenuOption;
                        menuMessage.clear();
                    }
                    else if (menuAction == pksm::switchui::Action::Right &&
                             selectedMenuOption % 2 == 0)
                    {
                        ++selectedMenuOption;
                        menuMessage.clear();
                    }

                    ui.drawGameMenu(
                        gameName, files[selected], details, selectedMenuOption, menuMessage);
                    SDL_Delay(16);

                    if (menuAction != pksm::switchui::Action::Confirm)
                    {
                        continue;
                    }
                    if (selectedMenuOption != 1)
                    {
                        menuMessage = "Esta opción todavía no está disponible.";
                        continue;
                    }

                    bool dirty = false;
                    bool finishEditing = false;
                    size_t selectedField = 0;
                    while (appletMainLoop() && !finishEditing && !quitRequested)
                    {
                        const auto editAction = ui.pollAction();
                        if (editAction == pksm::switchui::Action::Quit)
                        {
                            if (dirty)
                            {
                                error = "Exporta con Y o descarta con X antes de salir.";
                            }
                            else
                            {
                                finishEditing = true;
                                quitRequested = true;
                            }
                        }
                        if (editAction == pksm::switchui::Action::Up)
                        {
                            selectedField = selectedField == 0 ? fieldCount - 1 : selectedField - 1;
                        }
                        else if (editAction == pksm::switchui::Action::Down)
                        {
                            selectedField = (selectedField + 1) % fieldCount;
                        }
                        else if (editAction == pksm::switchui::Action::Back)
                        {
                            if (dirty)
                            {
                                error = "Exporta con Y o descarta con X antes de volver.";
                            }
                            else
                            {
                                finishEditing = true;
                            }
                        }
                        else if (editAction == pksm::switchui::Action::Discard)
                        {
                            finishEditing = true;
                            finish = true;
                        }
                        else if (editAction == pksm::switchui::Action::Save && dirty)
                        {
                            std::string outputPath;
                            if (exportSave(loaded, outputPath, error))
                            {
                                dirty = false;
                                error = "Exportado a " + outputPath;
                            }
                        }
                        else if (editAction == pksm::switchui::Action::Confirm && selectedField == 0)
                        {
                            std::string name;
                            if (ui.editTrainerName(save.otName(), name, nameLength) &&
                                name != save.otName())
                            {
                                save.otName(name);
                                dirty = true;
                                error.clear();
                            }
                        }
                        else if (editAction == pksm::switchui::Action::Left ||
                                 editAction == pksm::switchui::Action::Right)
                        {
                            const bool increase = editAction == pksm::switchui::Action::Right;
                            bool changed = false;
                            switch (selectedField)
                            {
                                case 1:
                                    if ((increase && save.TID() < UINT16_MAX) ||
                                        (!increase && save.TID() > 0))
                                    {
                                        save.TID(static_cast<u16>(
                                            increase ? save.TID() + 1 : save.TID() - 1));
                                        changed = true;
                                    }
                                    break;
                                case 2:
                                    if ((increase && save.SID() < UINT16_MAX) ||
                                        (!increase && save.SID() > 0))
                                    {
                                        save.SID(static_cast<u16>(
                                            increase ? save.SID() + 1 : save.SID() - 1));
                                        changed = true;
                                    }
                                    break;
                                case 3:
                                {
                                    const u32 current = save.money();
                                    const u32 maxMoney = hasBattlePoints ? 9999999 : 999999;
                                    const u32 next = increase
                                        ? std::min(maxMoney, current + 1000)
                                        : (current > 1000 ? current - 1000 : 0);
                                    if (next != current)
                                    {
                                        save.money(next);
                                        changed = true;
                                    }
                                    break;
                                }
                                case 4:
                                {
                                    if (hasBattlePoints)
                                    {
                                        const u32 current = save.BP();
                                        const u32 next = increase
                                            ? std::min<u32>(9999, current + 100)
                                            : (current > 100 ? current - 100 : 0);
                                        if (next != current)
                                        {
                                            save.BP(next);
                                            changed = true;
                                        }
                                    }
                                    break;
                                }
                                default:
                                    break;
                            }
                            if (selectedField >= hoursField)
                            {
                                switch (selectedField - hoursField)
                                {
                                    case 0:
                                        if ((increase && save.playedHours() < UINT16_MAX) ||
                                            (!increase && save.playedHours() > 0))
                                        {
                                            save.playedHours(static_cast<u16>(
                                                increase ? save.playedHours() + 1 :
                                                           save.playedHours() - 1));
                                            changed = true;
                                        }
                                        break;
                                    case 1:
                                    {
                                        const u8 current = save.playedMinutes();
                                        const u8 next = increase
                                            ? static_cast<u8>((current + 1) % 60)
                                            : static_cast<u8>(current == 0 ? 59 : current - 1);
                                        if (next != current)
                                        {
                                            save.playedMinutes(next);
                                            changed = true;
                                        }
                                        break;
                                    }
                                    case 2:
                                    {
                                        const u8 current = save.playedSeconds();
                                        const u8 next = increase
                                            ? static_cast<u8>((current + 1) % 60)
                                            : static_cast<u8>(current == 0 ? 59 : current - 1);
                                        if (next != current)
                                        {
                                            save.playedSeconds(next);
                                            changed = true;
                                        }
                                        break;
                                    }
                                    default:
                                        break;
                                }
                            }
                            if (changed)
                            {
                                dirty = true;
                                error.clear();
                            }
                        }

                        std::vector<pksm::switchui::Field> fields{
                            {"Nombre", save.otName()},
                            {"TID", std::to_string(save.TID())},
                            {"SID", std::to_string(save.SID())},
                            {"Dinero", std::to_string(save.money())},
                        };
                        if (hasBattlePoints)
                        {
                            fields.push_back({"BP", std::to_string(save.BP())});
                        }
                        fields.push_back({"Horas jugadas", std::to_string(save.playedHours())});
                        fields.push_back({"Minutos jugados", std::to_string(save.playedMinutes())});
                        fields.push_back({"Segundos jugados", std::to_string(save.playedSeconds())});
                        ui.drawTrainer(gameName, files[selected], fields, selectedField, dirty, error);
                        SDL_Delay(16);
                    }
                    if (finish || quitRequested)
                    {
                        break;
                    }
                }
            }
            else
            {
                while (appletMainLoop())
                {
                    ui.drawFiles(files, selected, error);
                    const auto loadAction = ui.pollAction();
                    if (loadAction == pksm::switchui::Action::Confirm ||
                        loadAction == pksm::switchui::Action::Back)
                    {
                        break;
                    }
                    if (loadAction == pksm::switchui::Action::Quit)
                    {
                        quitRequested = true;
                        break;
                    }
                    SDL_Delay(16);
                }
            }
        }
    }

    return 0;
}
