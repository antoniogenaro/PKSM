#include "screens/PokemonFields.hpp"

#include "SwitchUi.hpp"
#include "domain/FieldValues.hpp"
#include "domain/FormSprites.hpp"
#include "ui/Text.hpp"
#include "enums/Stat.hpp"
#include "pkx/PK8.hpp"
#include "sav/Sav.hpp"
#include "utils/i18n.hpp"
#include "utils/utils.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace pksm::switchui
{
    namespace
    {
        constexpr std::array<pksm::Stat, 6> stats = {pksm::Stat::HP, pksm::Stat::ATK,
            pksm::Stat::DEF, pksm::Stat::SPATK, pksm::Stat::SPDEF, pksm::Stat::SPD};
        constexpr std::array<Text, 6> statNames = {Text::StatHp, Text::StatAttack,
            Text::StatDefense, Text::StatSpAttack, Text::StatSpDefense, Text::StatSpeed};
        constexpr std::array<Text, 3> genderNames = {
            Text::GenderMale, Text::GenderFemale, Text::GenderNone};

        Field display(std::string label, std::string value, bool checkbox = false,
            bool checked = false)
        {
            Field field{std::move(label), std::move(value)};
            field.checkbox = checkbox;
            field.checked  = checked;
            return field;
        }

        template <typename Set>
        std::vector<unsigned int> idsOf(const Set& values)
        {
            std::vector<unsigned int> ids;
            ids.reserve(values.size());
            for (const auto& value : values)
            {
                ids.push_back(static_cast<unsigned int>(value));
            }
            return ids;
        }

        std::vector<unsigned int> range(unsigned int first, unsigned int last)
        {
            std::vector<unsigned int> values;
            for (unsigned int value = first; value <= last; ++value)
            {
                values.push_back(value);
            }
            return values;
        }

        std::string withId(const std::string& name, unsigned int id)
        {
            return tr(Text::WithId, {name, numericValue(id)});
        }

        bool setSpecies(pksm::PKX& pokemon, pksm::Species species)
        {
            if (pokemon.species() == species)
            {
                return false;
            }
            const u8 previousLevel = pokemon.level();
            if (pokemon.species() == pksm::Species::None || !pokemon.nicknamed())
            {
                std::string nickname = species.localize(pokemon.language());
                if (pokemon.generation() <= pksm::Generation::FOUR)
                {
                    StringUtils::toUpper(nickname);
                }
                pokemon.nickname(nickname);
            }
            pokemon.species(species);
            pokemon.alternativeForm(0);
            pokemon.setAbility(0);
            const u8 minimumLevel = pokemon.generation() > pksm::Generation::THREE ? 1 : 2;
            pokemon.level(std::clamp<u8>(previousLevel, minimumLevel, 100));
            pokemon.PID(pksm::PKX::getRandomPID(pokemon.species(), pokemon.gender(),
                pokemon.version(), pokemon.nature(), pokemon.alternativeForm(),
                pokemon.abilityNumber(), pokemon.shiny(), pokemon.TSV(), pokemon.PID(),
                pokemon.generation()));
            return true;
        }

        // The met or egg date. The year is stored as years after 2000 in one byte; the games
        // show up to 2099.
        EditableField pokemonDateField(
            Text label, std::function<Date()> get, std::function<void(const Date&)> set)
        {
            return dateField(tr(label), tr(label), YearRange{2000, 2099}, std::move(get),
                std::move(set));
        }

        // An unsigned byte-sized counter edited with the keyboard.
        EditableField counterField(const std::string& label, const std::string& title,
            unsigned int max, std::function<unsigned int()> get, std::function<void(unsigned int)> set)
        {
            Field shown = display(label, numericValue(get()));
            return numberField(std::move(shown), title, 0, max, std::move(get), std::move(set));
        }

        EditableField moveField(pksm::PKX& pokemon, pksm::Sav& save, size_t slotIndex, bool relearn)
        {
            const pksm::Language language = pokemon.language();
            const u8 slot                 = static_cast<u8>(slotIndex);
            const std::string label =
                tr(relearn ? Text::FieldRelearnMove : Text::FieldMove, {numericValue(slot + 1)});
            const pksm::Move move = relearn ? pokemon.relearnMove(slot) : pokemon.move(slot);
            return choiceField(
                display(label, withId(move.localize(language), static_cast<unsigned int>(move))),
                // Titled with the slot, such as "Move 1", as on the canvas.
                label, [&save] { return idsOf(save.availableMoves()); },
                [&pokemon](unsigned int id)
                { return withId(pksm::Move{static_cast<u16>(id)}.localize(pokemon.language()), id); },
                [&pokemon, slot, relearn]
                {
                    return static_cast<unsigned int>(
                        relearn ? pokemon.relearnMove(slot) : pokemon.move(slot));
                },
                [&pokemon, slot, relearn](unsigned int id)
                {
                    const pksm::Move next{static_cast<u16>(id)};
                    if (relearn)
                    {
                        pokemon.relearnMove(slot, next);
                    }
                    else
                    {
                        pokemon.move(slot, next);
                    }
                    pokemon.fixMoves();
                    pokemon.healPP();
                });
        }
    }

    std::string numericValue(unsigned int value)
    {
        return std::to_string(value);
    }

    PokemonEditorFields pokemonEditorFields(pksm::PKX& pokemon, pksm::Sav& save)
    {
        const pksm::Language language     = pokemon.language();
        const pksm::Generation generation = pokemon.generation();
        PokemonEditorFields result;
        // The first page holds the canvas's fields in its order; the rest go on the second.
        std::vector<EditableField>& fields = result.main;
        std::vector<EditableField>& more   = result.more;

        const unsigned int species = static_cast<unsigned int>(pokemon.species());
        fields.push_back(choiceField(
            display(tr(Text::FieldSpecies), pokemon.species() == pksm::Species::None
                                   ? tr(Text::Empty)
                                   : withId(pokemon.species().localize(language), species)),
            tr(Text::FieldSpecies), [&save] { return idsOf(save.availableSpecies()); },
            [&pokemon](unsigned int id)
            { return withId(pksm::Species{static_cast<u16>(id)}.localize(pokemon.language()), id); },
            [&pokemon] { return static_cast<unsigned int>(pokemon.species()); },
            [&pokemon](unsigned int id) { setSpecies(pokemon, pksm::Species{static_cast<u16>(id)}); }));

        fields.push_back(textField(display(tr(Text::FieldNickname), pokemon.nickname()), tr(Text::FieldNickname),
            generation >= pksm::Generation::SIX ? 12 : 10, [&pokemon] { return pokemon.nickname(); },
            [&pokemon](const std::string& nickname)
            {
                pokemon.nickname(nickname);
                std::string speciesName = pokemon.species().localize(pokemon.language());
                if (pokemon.generation() <= pksm::Generation::FOUR)
                {
                    StringUtils::toUpper(speciesName);
                }
                pokemon.nicknamed(nickname != speciesName);
            }));

        fields.push_back(numberField(display(tr(Text::FieldLevel), numericValue(pokemon.level())), tr(Text::FieldLevel),
            generation > pksm::Generation::THREE ? 1 : 2, 100,
            [&pokemon] { return static_cast<unsigned int>(pokemon.level()); },
            [&pokemon](unsigned int value) { pokemon.level(static_cast<u8>(value)); }));

        // Fields that cannot change for this Pokémon are left out rather than shown locked.
        if (generation >= pksm::Generation::THREE)
        {
            EditableField nature = choiceField(display(tr(Text::FieldNature), pokemon.nature().localize(language)),
                tr(Text::FieldNature), [] { return range(0, 24); },
                [&pokemon](unsigned int id)
                { return pksm::Nature{static_cast<u8>(id)}.localize(pokemon.language()); },
                [&pokemon] { return static_cast<unsigned int>(pokemon.nature()); },
                [&pokemon](unsigned int id) { pokemon.nature(pksm::Nature{static_cast<u8>(id)}); });
            nature.picker = EditableField::ChoicePicker::NatureGrid;
            fields.push_back(std::move(nature));
        }

        const u8 abilitySlot = pokemon.abilityNumber() >> 1;
        fields.push_back(choiceField(
            display(tr(Text::FieldAbility),
                tr(Text::AbilityValue,
                    {pokemon.ability().localize(language), numericValue(abilitySlot)})),
            tr(Text::FieldAbility),
            [&pokemon]
            {
                std::vector<unsigned int> slots;
                for (u8 i = 0; i < 3; ++i)
                {
                    if (pokemon.abilities(i) != pksm::Ability::None)
                    {
                        slots.push_back(i);
                    }
                }
                return slots;
            },
            [&pokemon](unsigned int slot)
            {
                return tr(Text::AbilityChoice,
                    {pokemon.abilities(static_cast<u8>(slot)).localize(pokemon.language()),
                        numericValue(slot)});
            },
            [&pokemon] { return static_cast<unsigned int>(pokemon.abilityNumber() >> 1); },
            [&pokemon](unsigned int slot) { pokemon.setAbility(static_cast<u8>(slot)); }));

        fields.push_back(choiceField(
            display(tr(Text::FieldItem), withId(i18n::item(language, pokemon.heldItem()), pokemon.heldItem())),
            tr(Text::FieldItem),
            [&save]
            {
                std::vector<unsigned int> items{0};
                for (int item : save.availableItems())
                {
                    if (item > 0 && item <= UINT16_MAX)
                    {
                        items.push_back(static_cast<unsigned int>(item));
                    }
                }
                std::sort(items.begin(), items.end());
                items.erase(std::unique(items.begin(), items.end()), items.end());
                return items;
            },
            [&pokemon](unsigned int item)
            { return withId(i18n::item(pokemon.language(), static_cast<u16>(item)), item); },
            [&pokemon] { return static_cast<unsigned int>(pokemon.heldItem()); },
            [&pokemon](unsigned int item) { pokemon.heldItem(static_cast<u16>(item)); }));

        const unsigned int ball = static_cast<unsigned int>(pokemon.ball());
        EditableField ballField = choiceField(
            display(tr(Text::FieldBall), withId(pokemon.ball().localize(language), ball)),
            tr(Text::FieldBall),
            [&save] { return idsOf(save.availableBalls()); },
            [&pokemon](unsigned int id)
            { return withId(pksm::Ball{static_cast<u8>(id)}.localize(pokemon.language()), id); },
            [&pokemon] { return static_cast<unsigned int>(pokemon.ball()); },
            [&pokemon](unsigned int id) { pokemon.ball(pksm::Ball{static_cast<u8>(id)}); });
        ballField.sprite = [](unsigned int id) { return static_cast<size_t>(id <= 25 ? id : 0); };
        ballField.display.hasBallSprite   = true;
        ballField.display.ballSpriteIndex = ball <= 25 ? ball : 0;
        fields.push_back(std::move(ballField));

        const unsigned int gender = static_cast<unsigned int>(pokemon.gender());
        EditableField genderField = choiceField(
            display(tr(Text::FieldGender),
                tr(gender < genderNames.size() ? genderNames[gender] : Text::GenderUnknown)),
            tr(Text::FieldGender), [] { return range(0, genderNames.size() - 1); },
            [](unsigned int id)
            { return std::string(tr(id < genderNames.size() ? genderNames[id] : Text::GenderUnknown)); },
            [&pokemon] { return static_cast<unsigned int>(pokemon.gender()); },
            [&pokemon](unsigned int id) { pokemon.gender(pksm::Gender{static_cast<u8>(id)}); });
        genderField.display.genderMark = gender == 0 ? 1 : gender == 1 ? 2 : 0;
        fields.push_back(std::move(genderField));

        fields.push_back(toggleField(
            display(tr(Text::FieldShiny), pokemon.shiny() ? tr(Text::Yes) : tr(Text::No), true, pokemon.shiny()),
            [&pokemon] { return pokemon.shiny(); }, [&pokemon](bool value) { pokemon.shiny(value); }));

        fields.push_back(counterField(tr(Text::FieldFriendship), tr(Text::FriendshipTitle), 255,
            [&pokemon] { return static_cast<unsigned int>(pokemon.currentFriendship()); },
            [&pokemon](unsigned int value) { pokemon.currentFriendship(static_cast<u8>(value)); }));

        fields.push_back(toggleField(
            display(tr(Text::FieldPokerus), pokemon.pkrsDays() > 0 ? tr(Text::Yes) : tr(Text::No), true, pokemon.pkrsDays() > 0),
            [&pokemon] { return pokemon.pkrs() != 0; },
            [&pokemon](bool value) { pokemon.pkrs(value ? 0xF4 : 0); }));

        fields.push_back(textField(display(tr(Text::FieldOriginalTrainer), pokemon.otName()),
            tr(Text::FieldOriginalTrainer),
            generation >= pksm::Generation::SIX ? 12 : language == pksm::Language::JPN ? 5 : 7,
            [&pokemon] { return pokemon.otName(); },
            [&pokemon](const std::string& name) { pokemon.otName(name); }));

        // From generation VII on the ID is shown, and edited, as the game shows it.
        const bool splitIds = generation == pksm::Generation::SEVEN ||
                              generation == pksm::Generation::LGPE ||
                              generation == pksm::Generation::EIGHT ||
                              generation == pksm::Generation::NINE;
        if (splitIds)
        {
            const auto shown = [&pokemon] { return showTrainerIds(pokemon.TID(), pokemon.SID()); };
            fields.push_back(numberField(display(tr(Text::TrainerTid), padDigits(shown().tid, 6)),
                tr(Text::TrainerTid), 0, MAX_SHOWN_TID, [shown] { return shown().tid; },
                [&pokemon, shown](unsigned int value)
                {
                    unsigned int tid = 0;
                    unsigned int sid = 0;
                    storeTrainerIds({value, shown().sid}, tid, sid);
                    pokemon.TID(static_cast<u16>(tid));
                    pokemon.SID(static_cast<u16>(sid));
                }));
        }
        else
        {
            fields.push_back(counterField(tr(Text::TrainerTid), tr(Text::TrainerTid), UINT16_MAX,
                [&pokemon] { return static_cast<unsigned int>(pokemon.TID()); },
                [&pokemon](unsigned int value) { pokemon.TID(static_cast<u16>(value)); }));
        }

        fields.push_back(readOnlyField(
            display(tr(Text::DetailOriginGame), i18n::game(language, pokemon.version()))));

        // The second page: form, egg, met and egg data, and the relearn moves.
        const unsigned int formCount =
            pokemon.species() == pksm::Species::None ? 0 : save.formCount(pokemon.species());
        if (formCount >= 2)
        {
            const Field formDisplay = display(tr(Text::FieldForm),
                withId(i18n::form(language, pokemon.version(), pokemon.species(),
                           static_cast<u8>(pokemon.alternativeForm())),
                    pokemon.alternativeForm()));
            more.push_back(choiceField(formDisplay, tr(Text::FieldForm),
                [formCount] { return range(0, formCount - 1); },
                [&pokemon, &save](unsigned int form)
                {
                    return withId(i18n::form(pokemon.language(), save.version(), pokemon.species(),
                                      static_cast<u8>(form)),
                        form);
                },
                [&pokemon] { return static_cast<unsigned int>(pokemon.alternativeForm()); },
                [&pokemon](unsigned int form) { pokemon.alternativeForm(static_cast<u16>(form)); }));
        }

        more.push_back(toggleField(display(tr(Text::FieldEgg), pokemon.egg() ? tr(Text::Yes) : tr(Text::No), true, pokemon.egg()),
            [&pokemon] { return pokemon.egg(); }, [&pokemon](bool value) { pokemon.egg(value); }));
        more.push_back(toggleField(display(tr(Text::FieldFatefulEncounter),
                                       pokemon.fatefulEncounter() ? tr(Text::Yes) : tr(Text::No), true,
                                       pokemon.fatefulEncounter()),
            [&pokemon] { return pokemon.fatefulEncounter(); },
            [&pokemon](bool value) { pokemon.fatefulEncounter(value); }));

        more.push_back(counterField(tr(Text::FieldMetLevel), tr(Text::FieldMetLevel), 100,
            [&pokemon] { return static_cast<unsigned int>(pokemon.metLevel()); },
            [&pokemon](unsigned int value) { pokemon.metLevel(static_cast<u8>(value)); }));
        // The games before generation IV keep no dates.
        if (generation >= pksm::Generation::FOUR)
        {
            more.push_back(pokemonDateField(Text::FieldMetDate,
                [&pokemon] { return pokemon.metDate(); },
                [&pokemon](const Date& date) { pokemon.metDate(date); }));
        }
        more.push_back(numberField(
            display(tr(Text::FieldMetLocation),
                tr(Text::LocationValue, {numericValue(pokemon.metLocation())})),
            tr(Text::FieldMetLocation), 0, UINT16_MAX,
            [&pokemon] { return static_cast<unsigned int>(pokemon.metLocation()); },
            [&pokemon](unsigned int value) { pokemon.metLocation(static_cast<u16>(value)); }));

        if (generation >= pksm::Generation::FOUR)
        {
            more.push_back(pokemonDateField(Text::FieldEggDate,
                [&pokemon] { return pokemon.eggDate(); },
                [&pokemon](const Date& date) { pokemon.eggDate(date); }));
            more.push_back(numberField(
                display(tr(Text::FieldEggLocation),
                    tr(Text::LocationValue, {numericValue(pokemon.eggLocation())})),
                tr(Text::FieldEggLocation), 0, UINT16_MAX,
                [&pokemon] { return static_cast<unsigned int>(pokemon.eggLocation()); },
                [&pokemon](unsigned int value) { pokemon.eggLocation(static_cast<u16>(value)); }));
        }
        if (generation == pksm::Generation::EIGHT)
        {
            auto& pk8 = static_cast<pksm::PK8&>(pokemon);
            more.push_back(counterField(tr(Text::FieldEnjoyment), tr(Text::FieldEnjoyment), 255,
                [&pk8] { return static_cast<unsigned int>(pk8.enjoyment()); },
                [&pk8](unsigned int value) { pk8.enjoyment(static_cast<u8>(value)); }));
            more.push_back(counterField(tr(Text::FieldFullness), tr(Text::FieldFullness), 255,
                [&pk8] { return static_cast<unsigned int>(pk8.fullness()); },
                [&pk8](unsigned int value) { pk8.fullness(static_cast<u8>(value)); }));
        }
        // Relearn moves exist from generation VI on.
        if (generation >= pksm::Generation::SIX)
        {
            for (size_t i = 0; i < 4; ++i)
            {
                more.push_back(moveField(pokemon, save, i, true));
            }
        }

        for (size_t i = 0; i < stats.size(); ++i)
        {
            const pksm::Stat stat   = stats[i];
            const std::string label = tr(Text::FieldIv, {tr(statNames[i])});
            EditableField iv        = counterField(label, label, pokemon.maxIV(),
                       [&pokemon, stat] { return static_cast<unsigned int>(pokemon.iv(stat)); },
                       [&pokemon, stat](unsigned int value) { pokemon.iv(stat, static_cast<u8>(value)); });
            iv.wrap = true;
            result.ivs.push_back(std::move(iv));
        }
        for (size_t i = 0; i < stats.size(); ++i)
        {
            const pksm::Stat stat = stats[i];
            unsigned int others   = 0;
            for (const pksm::Stat other : stats)
            {
                if (other != stat)
                {
                    others += pokemon.secondaryStatCalc(other);
                }
            }
            // An EV is capped both on its own and by what the other five leave of the total.
            const unsigned int max = std::min<unsigned int>(pokemon.maxSecondaryStatCalc(),
                pokemon.maxSecondaryStatCalcTotal() > others
                    ? pokemon.maxSecondaryStatCalcTotal() - others
                    : 0);
            const std::string label = tr(Text::FieldEv, {tr(statNames[i])});
            EditableField ev        = counterField(label, label, max,
                       [&pokemon, stat] { return static_cast<unsigned int>(pokemon.secondaryStatCalc(stat)); },
                       [&pokemon, stat](unsigned int value)
                       { pokemon.secondaryStatCalc(stat, static_cast<u16>(value)); });
            ev.wrap     = true;
            ev.inputMax = pokemon.maxSecondaryStatCalc();
            result.evs.push_back(std::move(ev));
        }
        for (size_t i = 0; i < 4; ++i)
        {
            result.moves.push_back(moveField(pokemon, save, i, false));
        }
        return result;
    }

    PokemonStats pokemonStats(const pksm::PKX& pokemon)
    {
        constexpr std::array<int, 6> natureColumn = {-1, 0, 1, 3, 4, 2};
        constexpr std::array<Text, 6> shortNames  = {Text::StatShortHp, Text::StatShortAttack,
             Text::StatShortDefense, Text::StatShortSpAttack, Text::StatShortSpDefense,
             Text::StatShortSpeed};
        PokemonStats result;
        const int nature  = static_cast<int>(u8(pokemon.nature()));
        const int raised  = nature / 5;
        const int lowered = nature % 5;
        for (size_t i = 0; i < stats.size(); ++i)
        {
            PokemonStats::Row row;
            row.name   = tr(shortNames[i]);
            row.iv     = numericValue(pokemon.iv(stats[i]));
            row.ev     = numericValue(pokemon.secondaryStatCalc(stats[i]));
            row.total  = numericValue(pokemon.stat(stats[i]));
            row.nature = raised == lowered ? 0
                         : natureColumn[i] == raised ? 1
                         : natureColumn[i] == lowered ? -1
                                                      : 0;
            result.evTotal += pokemon.secondaryStatCalc(stats[i]);
            result.rows.push_back(std::move(row));
        }
        result.evMax = pokemon.maxSecondaryStatCalcTotal();
        for (u8 i = 0; i < 4; ++i)
        {
            const pksm::Move move = pokemon.move(i);
            PokemonStats::MoveRow row;
            row.name = move == pksm::Move::None ? std::string("—") : move.localize(pokemon.language());
            if (move != pksm::Move::None)
            {
                row.pp = tr(Text::MovePp, {numericValue(pokemon.PP(i)), numericValue(pokemon.maxPP(i))});
            }
            result.moves.push_back(std::move(row));
        }
        return result;
    }

    void savePokemon(pksm::Sav& save, pksm::PKX& pokemon, bool party, u8 box, u8 slot)
    {
        pokemon.updatePartyData();
        pokemon.refreshChecksum();
        if (party)
        {
            save.pkm(pokemon, slot);
            save.fixParty();
        }
        else
        {
            save.pkm(pokemon, box, slot, false);
        }
        if (pokemon.species() != pksm::Species::None)
        {
            save.dex(pokemon);
        }
    }

    PokemonSlot pokemonSlot(const pksm::PKX* pokemon)
    {
        if (!pokemon || pokemon->species() == pksm::Species::None)
        {
            return {};
        }
        return {false, static_cast<size_t>(pokemon->species()), formSpriteIndex(*pokemon)};
    }

    PokemonDetail describePokemon(const pksm::PKX& pokemon)
    {
        PokemonDetail detail;
        if (pokemon.species() == pksm::Species::None)
        {
            return detail;
        }
        const pksm::Language language = pokemon.language();
        const unsigned int species    = static_cast<unsigned int>(pokemon.species());

        detail.empty   = false;
        detail.species = species;
        detail.formSprite = formSpriteIndex(pokemon);
        detail.number  = tr(Text::DexNumber, {padDigits(species, 3)});
        detail.name    = pokemon.egg() ? std::string(tr(Text::FieldEgg))
                                       : pokemon.species().localize(language);
        detail.level   = tr(Text::LevelShort, {numericValue(pokemon.level())});
        if (pokemon.nicknamed())
        {
            detail.nickname = tr(Text::NicknameLabel, {pokemon.nickname()});
        }
        detail.male   = pokemon.gender() == pksm::Gender::Male;
        detail.female = pokemon.gender() == pksm::Gender::Female;
        detail.shiny  = pokemon.shiny();
        detail.ball   = static_cast<size_t>(pokemon.ball());

        const pksm::Type type1 = pokemon.type1();
        const pksm::Type type2 = pokemon.type2();
        detail.types.push_back({type1.localize(language), static_cast<size_t>(u8(type1))});
        if (type2 != type1)
        {
            detail.types.push_back({type2.localize(language), static_cast<size_t>(u8(type2))});
        }

        // From generation VII on, the ID is shown as the game shows it.
        const pksm::Generation generation = pokemon.generation();
        const bool splitIds = generation == pksm::Generation::SEVEN ||
                              generation == pksm::Generation::LGPE ||
                              generation == pksm::Generation::EIGHT ||
                              generation == pksm::Generation::NINE;
        const std::string tid = splitIds
                                    ? padDigits(showTrainerIds(pokemon.TID(), pokemon.SID()).tid, 6)
                                    : numericValue(pokemon.TID());
        const u16 item        = pokemon.heldItem();
        detail.info           = {
            {tr(Text::DetailTrainer), pokemon.otName()},
            {tr(Text::TrainerTid), tid},
            {tr(Text::FieldItem), item == 0 ? std::string(tr(Text::DetailNone))
                                            : i18n::item(language, item)},
            {tr(Text::FieldAbility), pokemon.ability().localize(language)},
            {tr(Text::FieldNature), pokemon.nature().localize(language)},
            {tr(Text::DetailOriginGame), i18n::game(language, pokemon.version())},
        };

        // A nature raises the stat of its row and lowers the one of its column, in the order
        // Attack, Defense, Speed, Sp. Atk, Sp. Def; the diagonal changes nothing.
        constexpr std::array<int, 6> natureColumn = {-1, 0, 1, 3, 4, 2};
        constexpr std::array<Text, 6> shortNames  = {Text::StatShortHp, Text::StatShortAttack,
             Text::StatShortDefense, Text::StatShortSpAttack, Text::StatShortSpDefense,
             Text::StatShortSpeed};
        const int nature  = static_cast<int>(u8(pokemon.nature()));
        const int raised  = nature / 5;
        const int lowered = nature % 5;
        for (size_t i = 0; i < stats.size(); ++i)
        {
            int effect = 0;
            if (raised != lowered && natureColumn[i] == raised)
            {
                effect = 1;
            }
            else if (raised != lowered && natureColumn[i] == lowered)
            {
                effect = -1;
            }
            detail.ivs.push_back({tr(Text::FieldIv, {tr(shortNames[i])}),
                numericValue(pokemon.iv(stats[i])), effect});
        }

        for (u8 i = 0; i < 4; ++i)
        {
            const pksm::Move move = pokemon.move(i);
            detail.moves.push_back(move == pksm::Move::None ? std::string("—")
                                                            : move.localize(language));
        }
        return detail;
    }
}
