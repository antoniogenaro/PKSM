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
#include "domain/GameGrid.hpp"
#include "domain/GameGrid.hpp"
#include "domain/GameProfile.hpp"
#include "domain/SaveCache.hpp"
#include "nlohmann/json.hpp"
#include "ui/Text.hpp"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
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
            check(std::string(tr(sword->name)) == "Pokémon Sword", "Sword name");
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

        const auto profiles = gameProfiles();
        check(profiles.size() == 4, "four games are listed");
        bool allFound = true;
        for (const auto& profile : profiles)
        {
            allFound = allFound && findGameProfile(profile.version) == &profile;
        }
        check(allFound, "every listed game can be found");
    }

    GameProfile profileIn(Text group)
    {
        return GameProfile{GameVersion::SW, Text::GameSword, group, 0, false, 0, 0, 0, 0};
    }

    void testGameGrid()
    {
        const GameGrid real(gameProfiles());
        check(real.gameCount() == 4, "the game list shows every game");
        check(real.cards().size() == 2, "paired games share a card");
        check(real.cards()[0].group == Text::GroupGen3Gba, "the GBA games come first");
        check(real.cards()[0].games == std::vector<size_t>{0, 1}, "FireRed and LeafGreen pair up");
        check(real.down(0) == 0, "down stays put with a single row");

        // Cards: 0:{0,1} 1:{2} 2:{3,4} 3:{5,6} on the first row, 4:{7,8} 5:{9} below.
        const std::vector<GameProfile> profiles{profileIn(Text::Yes), profileIn(Text::Yes),
            profileIn(Text::No), profileIn(Text::Empty), profileIn(Text::Empty),
            profileIn(Text::NotAvailable), profileIn(Text::NotAvailable), profileIn(Text::WithId),
            profileIn(Text::WithId), profileIn(Text::ExportedTo)};
        const GameGrid grid(profiles);
        check(grid.cards().size() == 6, "a new card starts when the group changes");
        check(grid.right(3) == 4 && grid.right(4) == 5, "right walks every game");
        check(grid.right(9) == 0 && grid.left(0) == 9, "left and right wrap around");
        check(grid.down(1) == 8, "down keeps the position on the card");
        check(grid.down(2) == 9, "down reaches a smaller card");
        check(grid.down(5) == 5 && grid.down(4) == 4, "down stays put without a card below");
        check(grid.up(8) == 1 && grid.up(9) == 2, "up goes to the card above");
        check(grid.up(1) == 1, "up stays put on the first row");

        const GameGrid empty(std::span<const GameProfile>{});
        check(empty.left(0) == 0 && empty.right(0) == 0 && empty.down(0) == 0,
            "an empty grid does not move");
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
        check(error == "The number is out of range.", "range error message");

        error.clear();
        check(!parseNumber("", 100, value, error), "rejects empty input");
        check(error == "Enter a number.", "empty error message");

        check(!parseNumber("12a", 100, value, error), "rejects trailing text");
        check(!parseNumber("-1", 100, value, error), "rejects negative numbers");
        check(!parseNumber("99999999999", 4294967295u, value, error), "rejects overflow");
    }

    size_t placeholders(std::string_view text)
    {
        size_t count = 0;
        for (size_t at = text.find("{}"); at != std::string_view::npos;
             at        = text.find("{}", at + 2))
        {
            ++count;
        }
        return count;
    }

    nlohmann::json readJson(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        const std::string content{std::istreambuf_iterator<char>(file), {}};
        return nlohmann::json::parse(content, nullptr, false);
    }

    void writeFile(const std::filesystem::path& path, std::string_view content)
    {
        std::ofstream(path, std::ios::binary) << content;
    }

    // Every language file has exactly the app's keys, each a text with English's values.
    void testLanguageFiles()
    {
        const nlohmann::json english = readJson(std::filesystem::path(SWITCH_STRINGS) / "en.json");
        check(english.is_object(), "en.json is a JSON object");
        for (const auto language : {TextLanguage::English, TextLanguage::Spanish})
        {
            const nlohmann::json json =
                readJson(std::filesystem::path(SWITCH_STRINGS) /
                         (std::string(textLanguageCode(language)) + ".json"));
            bool valid            = json.is_object() && english.is_object();
            bool allPresent       = valid;
            bool samePlaceholders = valid;
            for (size_t index = 0; valid && index < static_cast<size_t>(Text::Count); ++index)
            {
                const char* key    = textKey(static_cast<Text>(index));
                const auto found   = json.find(key);
                const auto base    = english.find(key);
                const bool present = found != json.end() && found->is_string() &&
                                     !found->get<std::string>().empty() && base != english.end() &&
                                     base->is_string();
                allPresent       = allPresent && present;
                samePlaceholders = samePlaceholders && present &&
                                   placeholders(found->get<std::string>()) ==
                                       placeholders(base->get<std::string>());
            }
            check(valid, "every language file is a JSON object");
            check(allPresent, "every language file has every text");
            check(samePlaceholders, "every language inserts the same values");
            check(!valid || json.size() == static_cast<size_t>(Text::Count),
                "no language file has unknown keys");
        }
    }

    void testTexts()
    {
        check(textLanguage() == TextLanguage::English, "English is the default language");
        check(std::string(tr(Text::Yes)) == "Yes", "before loading, a text is its key");

        check(loadTexts(TextLanguage::English, SWITCH_STRINGS), "English loads");
        check(std::string(tr(Text::Yes)) == "Yes", "English text");
        check(tr(Text::ExportedTo, {"sdmc:/a"}) == "Exported to sdmc:/a", "one value");
        check(tr(Text::BoxSection, {"3", "32", "Box 3"}) == "Box 3/32  /  Box 3",
            "values go in order");
        check(tr(Text::WithId, {"Pikachu"}) == "Pikachu (#{})", "missing values stay marked");
        check(tr(Text::Yes, {"ignored"}) == "Yes", "extra values are ignored");

        check(loadTexts(TextLanguage::Spanish, SWITCH_STRINGS), "Spanish loads");
        check(textLanguage() == TextLanguage::Spanish, "the language can change");
        check(std::string(tr(Text::Yes)) == "Sí", "Spanish text");
        check(tr(Text::ExportedTo, {"sdmc:/a"}) == "Exportado a sdmc:/a", "Spanish value");
        check(std::string(tr(Text::GameSword)) == "Espada", "Spanish game name");

        // A translation that lacks a text, or inserts other values, falls back to English.
        const std::filesystem::path folder =
            std::filesystem::temp_directory_path() / "pksm-switch-text-tests";
        std::filesystem::remove_all(folder);
        std::filesystem::create_directories(folder);
        writeFile(
            folder / "en.json", R"({"Yes": "Yes", "No": "No", "ExportedTo": "Exported to {}"})");
        writeFile(folder / "es.json", R"({"Yes": "Sí", "ExportedTo": "Exportado"})");
        check(loadTexts(TextLanguage::Spanish, folder.string()), "partial files still load");
        check(std::string(tr(Text::Yes)) == "Sí", "translated text is used");
        check(std::string(tr(Text::No)) == "No", "a missing text comes from English");
        check(std::string(tr(Text::ExportedTo)) == "Exported to {}",
            "a text with other values comes from English");
        check(std::string(tr(Text::Empty)) == "Empty", "a text missing everywhere is its key");

        writeFile(folder / "es.json", "{ not json");
        check(!loadTexts(TextLanguage::Spanish, folder.string()), "broken JSON is reported");
        check(std::string(tr(Text::Yes)) == "Yes", "broken JSON falls back to English");

        check(!loadTexts(TextLanguage::English, (folder / "missing").string()),
            "a missing file is reported");
        check(std::string(tr(Text::Yes)) == "Yes", "without files, a text is its key");
        std::filesystem::remove_all(folder);

        check(loadTexts(TextLanguage::English, SWITCH_STRINGS), "English loads again");
    }

    void testSaveCache()
    {
        const SaveCache::Stamp stamp{1572864, 1700000000};
        const SaveCache::Stamp resized{1572865, 1700000000};
        const SaveCache::Stamp touched{1572864, 1700000001};

        SaveCache cache;
        check(!cache.lookup("a/main", stamp), "empty cache knows nothing");
        cache.store("a/main", stamp, 44);
        cache.store("b\twith tab/main", stamp, SaveCache::UNRECOGNIZED);
        cache.store("bad\nname", stamp, 44);
        check(cache.changed(), "storing marks the cache changed");

        const SaveCache loaded = SaveCache::parse(cache.serialize());
        check(!loaded.changed(), "a parsed cache starts unchanged");
        check(loaded.lookup("a/main", stamp) == 44, "round trip keeps the version");
        check(loaded.lookup("b\twith tab/main", stamp) == SaveCache::UNRECOGNIZED,
            "round trip keeps unrecognized files and tabs in paths");
        check(!loaded.lookup("bad\nname", stamp), "paths with line breaks are not stored");
        check(!loaded.lookup("a/main", resized), "a resized file is rescanned");
        check(!loaded.lookup("a/main", touched), "a modified file is rescanned");

        SaveCache same = SaveCache::parse(cache.serialize());
        same.store("a/main", stamp, 44);
        check(!same.changed(), "storing the same entry changes nothing");
        same.keepOnly({"a/main", "b\twith tab/main"});
        check(!same.changed(), "keeping every file changes nothing");
        same.keepOnly({"a/main"});
        check(same.changed(), "forgetting a file marks the cache changed");
        check(!same.lookup("b\twith tab/main", stamp), "forgotten files are gone");

        check(!SaveCache::parse("pksm-switch-saves 0\n1\t2\t44\ta/main\n")
                   .lookup("a/main", SaveCache::Stamp{1, 2}),
            "another format version is ignored");
        const SaveCache damaged = SaveCache::parse(
            "pksm-switch-saves 1\nnot a line\n1\tx\t44\tb/main\n1\t2\t44\t\n1\t2\t45\tc/main");
        check(!damaged.lookup("b/main", SaveCache::Stamp{1, 0}), "damaged lines are dropped");
        check(damaged.lookup("c/main", SaveCache::Stamp{1, 2}) == 45,
            "good lines survive, even without a final line break");
    }
}

int main()
{
    // Before anything changes the language, so it checks the default.
    testTexts();
    testLanguageFiles();
    testGameProfiles();
    testGameGrid();
    testGameGrid();
    testClampedSteps();
    testWrappedSteps();
    testChoices();
    testParseNumber();
    testSaveCache();

    if (failures != 0)
    {
        std::printf("SwitchDomain: %d checks failed\n", failures);
        return 1;
    }
    std::printf("SwitchDomain: all checks passed\n");
    return 0;
}
