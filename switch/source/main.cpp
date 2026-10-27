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

#include "SwitchPokemonEditor.hpp"
#include "SwitchUi.hpp"
#include "app/Session.hpp"
#include "domain/GameProfile.hpp"
#include "platform/SaveStorage.hpp"
#include "sav/Sav.hpp"
#include "utils/i18n.hpp"
#include <switch.h>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace
{
    struct RomfsCleanup
    {
        ~RomfsCleanup()
        {
            i18n::exit();
            romfsExit();
        }
    };
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

    namespace storage = pksm::switchui::storage;
    using pksm::switchui::Session;

    std::string storageError;
    std::vector<std::string> files = storage::prepareSaveDirectory(storageError)
                                         ? storage::listSaveFiles(storageError)
                                         : std::vector<std::string>{};
    std::vector<std::string> supportedFiles;
    std::vector<std::string> gameNames;
    std::vector<size_t> gameMascots;
    supportedFiles.reserve(files.size());
    gameNames.reserve(files.size());
    gameMascots.reserve(files.size());
    for (const auto& file : files)
    {
        std::string identifyError;
        if (const auto identified = Session::open(storage::savePath(file), identifyError))
        {
            supportedFiles.push_back(file);
            gameNames.emplace_back(identified->profile().name);
            gameMascots.push_back(identified->profile().mascotSpecies);
        }
    }
    files = std::move(supportedFiles);
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

        ui.drawFiles(files, gameNames, gameMascots, selected, storageError);
        SDL_Delay(16);

        if (!files.empty() && action == pksm::switchui::Action::Confirm)
        {
            std::string error;
            const auto session = Session::open(storage::savePath(files[selected]), error);
            if (session)
            {
                auto& save = session->save();
                const auto& profile = session->profile();
                i18n::init(save.language());
                const bool hasBattlePoints = profile.hasBattlePoints;
                const size_t hoursField = hasBattlePoints ? 5 : 4;
                const size_t fieldCount = hasBattlePoints ? 8 : 7;
                const size_t nameLength = profile.trainerNameLengthFor(save.language());
                const char* selectedGameName = profile.name;
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
                        if (session->dirty())
                        {
                            menuMessage = "Exporta con Y o descarta con X antes de salir.";
                        }
                        else
                        {
                            finish = true;
                            quitRequested = true;
                        }
                    }
                    else if (menuAction == pksm::switchui::Action::Back)
                    {
                        if (session->dirty())
                        {
                            menuMessage = "Exporta con Y o descarta con X antes de volver.";
                        }
                        else
                        {
                            finish = true;
                        }
                    }
                    else if (menuAction == pksm::switchui::Action::Discard)
                    {
                        finish = true;
                    }
                    else if (menuAction == pksm::switchui::Action::Save && session->dirty())
                    {
                        std::string outputPath;
                        if (session->exportChanges(outputPath, error))
                        {
                            menuMessage = "Exportado a " + outputPath;
                        }
                        else
                        {
                            menuMessage = error;
                        }
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
                        selectedGameName, files[selected], details, selectedMenuOption, menuMessage);
                    SDL_Delay(16);

                    if (menuAction != pksm::switchui::Action::Confirm)
                    {
                        continue;
                    }
                    if (selectedMenuOption == 1)
                    {
                        const auto result = pksm::switchui::runPokemonEditor(
                            *session, ui, selectedGameName, files[selected], error);
                        if (result == pksm::switchui::PokemonEditorResult::Discard)
                        {
                            finish = true;
                        }
                        else if (result == pksm::switchui::PokemonEditorResult::Quit)
                        {
                            finish = true;
                            quitRequested = true;
                        }
                        menuMessage.clear();
                        continue;
                    }
                    if (selectedMenuOption != 5)
                    {
                        menuMessage = "Esta opción todavía no está disponible.";
                        continue;
                    }

                    bool finishEditing = false;
                    size_t selectedField = 0;
                    while (appletMainLoop() && !finishEditing && !quitRequested)
                    {
                        const auto editAction = ui.pollAction();
                        if (editAction == pksm::switchui::Action::Quit)
                        {
                            if (session->dirty())
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
                            if (session->dirty())
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
                        else if (editAction == pksm::switchui::Action::Save && session->dirty())
                        {
                            std::string outputPath;
                            if (session->exportChanges(outputPath, error))
                            {
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
                                session->markDirty();
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
                                    const u32 maxMoney = profile.maxMoney;
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
                                session->markDirty();
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
                        ui.drawTrainer(
                            selectedGameName, files[selected], fields, selectedField,
                            session->dirty(), error);
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
                    ui.drawFiles(files, gameNames, gameMascots, selected, error);
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
