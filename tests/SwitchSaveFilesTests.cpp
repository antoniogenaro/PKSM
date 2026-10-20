/*
 *   This file is part of PKSM
 *   Copyright (C) 2016-2026 Bernardo Giordano, Admiral Fish, piepie62
 *
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
 *         or requiring that modified versions of such material be marked in
 *         reasonable ways as different from the original version.
 */

#include "SwitchSaveFiles.hpp"
#include <cstdio>
#include <string>
#include <vector>

int main()
{
    const std::vector<std::string> files = {
        "Checkpoint/saves/0x1234/Game/slot-1/main",
        "Checkpoint/saves/0x1234/Game/slot-1/main.bak",
        "Checkpoint/saves/0x1234/Game/slot-1/backup",
        "Checkpoint/saves/0x1234/Game/slot-2/main",
        "manual/custom-save.sav",
        "manual/another-save.sav",
    };
    const std::vector<std::string> expected = {
        "Checkpoint/saves/0x1234/Game/slot-1/main",
        "Checkpoint/saves/0x1234/Game/slot-2/main",
        "manual/custom-save.sav",
        "manual/another-save.sav",
    };
    const auto filtered = pksm::switchui::filterMainSaveFiles(files);
    if (filtered != expected)
    {
        std::printf("SwitchSaveFiles: unexpected filtered file list\n");
        return 1;
    }

    std::printf("SwitchSaveFiles: all checks passed\n");
    return 0;
}
