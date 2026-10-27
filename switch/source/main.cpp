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


#include "SwitchUi.hpp"
#include "app/App.hpp"
#include "screens/SaveListScreen.hpp"
#include "utils/i18n.hpp"
#include <switch.h>
#include <memory>
#include <string>

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

    pksm::switchui::App app(ui);
    app.push(std::make_unique<pksm::switchui::SaveListScreen>());
    app.run();
    return 0;
}
