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

// The Switch app's pure rules: which games it supports and how editor values move.

#include "domain/FieldValues.hpp"
#include "domain/GameProfile.hpp"
#include <cstdio>
#include <string>
#include <vector>

namespace
{
    int failures = 0;

    void check(bool condition, const char* description)
    {
        if (!condition)
        {
            std::printf("SwitchDomain: FAILED %s\n", description);
            ++failures;
        }
    }

    using pksm::GameVersion;
    using pksm::Language;
    using namespace pksm::switchui;

    void testGameProfiles()
    {
        const GameProfile* sword = findGameProfile(GameVersion::SW);
        check(sword != nullptr, "Sword is supported");
        if (sword)
        {
            check(std::string(sword->name) == "Pokemon Espada", "Sword name");
            check(sword->mascotSpecies == 888, "Sword mascot is Zacian");
            check(sword->hasBattlePoints, "Sword has BP");
            check(sword->maxMoney == 9999999, "Sword money cap");
            check(sword->trainerNameLengthFor(Language::JPN) == 12, "Sword JPN name length");
        }

        const GameProfile* fireRed = findGameProfile(GameVersion::FR);
        check(fireRed != nullptr, "FireRed is supported");
        if (fireRed)
        {
            check(!fireRed->hasBattlePoints, "FireRed has no BP");
            check(fireRed->maxMoney == 999999, "FireRed money cap");
            check(fireRed->trainerNameLengthFor(Language::ENG) == 7, "FireRed name length");
            check(fireRed->trainerNameLengthFor(Language::JPN) == 5, "FireRed JPN name length");
        }

        check(findGameProfile(GameVersion::SH) != nullptr, "Shield is supported");
        check(findGameProfile(GameVersion::LG) != nullptr, "LeafGreen is supported");
        check(findGameProfile(GameVersion::X) == nullptr, "X is not supported");
        check(findGameProfile(GameVersion::SL) == nullptr, "Scarlet is not supported");
    }

    void testClampedSteps()
    {
        const StepRule level{2, 100};
        check(stepNumber(level, 50, 1) == 51, "level goes up");
        check(stepNumber(level, 50, -1) == 49, "level goes down");
        check(stepNumber(level, 100, 1) == 100, "level stops at max");
        check(stepNumber(level, 2, -1) == 2, "level stops at min");
        check(stepNumber(level, 1, -1) == 2, "below min moves to min");
        check(stepNumber(level, 150, 1) == 100, "above max moves to max");

        const StepRule money{0, 999999, 1000};
        check(stepNumber(money, 5000, 1) == 6000, "money goes up by 1000");
        check(stepNumber(money, 999500, 1) == 999999, "money stops at its cap");
        check(stepNumber(money, 1500, -1) == 500, "money goes down by 1000");
        check(stepNumber(money, 1000, -1) == 0, "money reaches zero");
        check(stepNumber(money, 500, -1) == 0, "money never goes negative");

        const StepRule wide{0, 4294967295u, 1000};
        check(stepNumber(wide, 4294967000u, 1) == 4294967295u, "no overflow near the top");
    }

    void testWrappedSteps()
    {
        const StepRule minutes{0, 59, 1, true};
        check(stepNumber(minutes, 30, 1) == 31, "minutes go up");
        check(stepNumber(minutes, 59, 1) == 0, "minutes wrap to zero");
        check(stepNumber(minutes, 0, -1) == 59, "minutes wrap to 59");

        const StepRule iv{0, 31, 1, true};
        check(stepNumber(iv, 31, 1) == 0, "IV wraps up");
        check(stepNumber(iv, 0, -1) == 31, "IV wraps down");
    }

    void testChoices()
    {
        const std::vector<unsigned int> balls{1, 4, 7};
        check(nextChoice(balls, 4, 1) == 7, "next choice");
        check(nextChoice(balls, 4, -1) == 1, "previous choice");
        check(nextChoice(balls, 7, 1) == 1, "choices wrap forward");
        check(nextChoice(balls, 1, -1) == 7, "choices wrap backward");
        check(nextChoice(balls, 99, 1) == 4, "unknown current counts as the first");
        check(nextChoice({5}, 5, 1) == 5, "a single choice stays");

        check(indexOf(balls, 7) == 2, "index of a choice");
        check(indexOf(balls, 99) == 0, "index of an unknown value");
    }

    void testParseNumber()
    {
        unsigned int value = 0;
        std::string error;
        check(parseNumber("42", 100, value, error) && value == 42, "reads a number");
        check(parseNumber("100", 100, value, error) && value == 100, "accepts the maximum");

        error.clear();
        check(!parseNumber("101", 100, value, error), "rejects above maximum");
        check(error == "El número está fuera del rango permitido.", "range error message");

        error.clear();
        check(!parseNumber("", 100, value, error), "rejects empty input");
        check(error == "Introduce un número.", "empty error message");

        check(!parseNumber("12a", 100, value, error), "rejects trailing text");
        check(!parseNumber("-1", 100, value, error), "rejects negative numbers");
        check(!parseNumber("99999999999", 4294967295u, value, error), "rejects overflow");
    }
}

int main()
{
    testGameProfiles();
    testClampedSteps();
    testWrappedSteps();
    testChoices();
    testParseNumber();

    if (failures != 0)
    {
        std::printf("SwitchDomain: %d checks failed\n", failures);
        return 1;
    }
    std::printf("SwitchDomain: all checks passed\n");
    return 0;
}
