#include "screens/PokemonFields.hpp"

#include "SwitchUi.hpp"
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
        constexpr std::array<Text, 12> monthNames = {Text::January, Text::February, Text::March,
            Text::April, Text::May, Text::June, Text::July, Text::August, Text::September,
            Text::October, Text::November, Text::December};

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

        // Day, month or year of a date, picked from a list.
        EditableField dateField(const std::string& label, unsigned int min, unsigned int max,
            std::function<unsigned int()> get, std::function<void(unsigned int)> set)
        {
            Field shown         = display(label, numericValue(get()));
            EditableField field =
                numberField(std::move(shown), label, min, max, std::move(get), std::move(set));
            field.input = EditableField::NumberInput::List;
            return field;
        }

        // The day, month and year rows of the met or egg date.
        void addDateFields(std::vector<EditableField>& fields, Text dayLabel, Text monthLabel,
            Text yearLabel, std::function<Date()> get, std::function<void(const Date&)> set)
        {
            fields.push_back(dateField(tr(dayLabel), 1, 31, [get] { return get().day(); },
                [get, set](unsigned int value)
                {
                    Date date = get();
                    date.day(static_cast<u8>(value));
                    set(date);
                }));
            EditableField month = dateField(tr(monthLabel), 1, 12, [get] { return get().month(); },
                [get, set](unsigned int value)
                {
                    Date date = get();
                    date.month(static_cast<u8>(value));
                    set(date);
                });
            month.label = [](unsigned int value) { return std::string(tr(monthNames[value - 1])); };
            fields.push_back(std::move(month));
            fields.push_back(dateField(tr(yearLabel), 0, 9999, [get] { return get().year(); },
                [get, set](unsigned int value)
                {
                    Date date = get();
                    date.year(value);
                    set(date);
                }));
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
            if (relearn && pokemon.generation() < pksm::Generation::SIX)
            {
                return readOnlyField(display(label, tr(Text::RelearnUnavailable)));
            }
            return choiceField(
                display(label, withId(move.localize(language), static_cast<unsigned int>(move))),
                tr(Text::MoveTitle), [&save] { return idsOf(save.availableMoves()); },
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

    std::vector<EditableField> pokemonFields(pksm::PKX& pokemon, pksm::Sav& save)
    {
        const pksm::Language language = pokemon.language();
        const pksm::Generation generation = pokemon.generation();
        std::vector<EditableField> fields;

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
        else
        {
            fields.push_back(readOnlyField(display(tr(Text::FieldNature), tr(Text::NotAvailable))));
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

        fields.push_back(toggleField(
            display(tr(Text::FieldShiny), pokemon.shiny() ? tr(Text::Yes) : tr(Text::No), true, pokemon.shiny()),
            [&pokemon] { return pokemon.shiny(); }, [&pokemon](bool value) { pokemon.shiny(value); }));
        fields.push_back(toggleField(
            display(tr(Text::FieldPokerus), pokemon.pkrsDays() > 0 ? tr(Text::Yes) : tr(Text::No), true, pokemon.pkrsDays() > 0),
            [&pokemon] { return pokemon.pkrs() != 0; },
            [&pokemon](bool value) { pokemon.pkrs(value ? 0xF4 : 0); }));

        fields.push_back(textField(display(tr(Text::FieldOriginalTrainer), pokemon.otName()),
            tr(Text::FieldOriginalTrainer),
            generation >= pksm::Generation::SIX ? 12 : language == pksm::Language::JPN ? 5 : 7,
            [&pokemon] { return pokemon.otName(); },
            [&pokemon](const std::string& name) { pokemon.otName(name); }));

        const unsigned int gender = static_cast<unsigned int>(pokemon.gender());
        fields.push_back(choiceField(
            display(tr(Text::FieldGender),
                tr(gender < genderNames.size() ? genderNames[gender] : Text::GenderUnknown)),
            tr(Text::FieldGender), [] { return range(0, genderNames.size() - 1); },
            [](unsigned int id)
            { return std::string(tr(id < genderNames.size() ? genderNames[id] : Text::GenderUnknown)); },
            [&pokemon] { return static_cast<unsigned int>(pokemon.gender()); },
            [&pokemon](unsigned int id) { pokemon.gender(pksm::Gender{static_cast<u8>(id)}); }));

        fields.push_back(counterField(tr(Text::FieldFriendship), tr(Text::FriendshipTitle), 255,
            [&pokemon] { return static_cast<unsigned int>(pokemon.currentFriendship()); },
            [&pokemon](unsigned int value) { pokemon.currentFriendship(static_cast<u8>(value)); }));

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

        const Field formDisplay = display(tr(Text::FieldForm),
            withId(i18n::form(language, pokemon.version(), pokemon.species(),
                       static_cast<u8>(pokemon.alternativeForm())),
                pokemon.alternativeForm()));
        const unsigned int formCount =
            pokemon.species() == pksm::Species::None ? 0 : save.formCount(pokemon.species());
        if (formCount < 2)
        {
            fields.push_back(readOnlyField(formDisplay));
        }
        else
        {
            fields.push_back(choiceField(formDisplay, tr(Text::FieldForm),
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

        fields.push_back(toggleField(display(tr(Text::FieldEgg), pokemon.egg() ? tr(Text::Yes) : tr(Text::No), true, pokemon.egg()),
            [&pokemon] { return pokemon.egg(); }, [&pokemon](bool value) { pokemon.egg(value); }));
        fields.push_back(toggleField(display(tr(Text::FieldFatefulEncounter),
                                         pokemon.fatefulEncounter() ? tr(Text::Yes) : tr(Text::No), true,
                                         pokemon.fatefulEncounter()),
            [&pokemon] { return pokemon.fatefulEncounter(); },
            [&pokemon](bool value) { pokemon.fatefulEncounter(value); }));

        fields.push_back(counterField(tr(Text::FieldMetLevel), tr(Text::FieldMetLevel), 100,
            [&pokemon] { return static_cast<unsigned int>(pokemon.metLevel()); },
            [&pokemon](unsigned int value) { pokemon.metLevel(static_cast<u8>(value)); }));
        addDateFields(fields, Text::FieldMetDay, Text::FieldMetMonth, Text::FieldMetYear,
            [&pokemon] { return pokemon.metDate(); },
            [&pokemon](const Date& date) { pokemon.metDate(date); });
        fields.push_back(numberField(
            display(tr(Text::FieldMetLocation),
                tr(Text::LocationValue, {numericValue(pokemon.metLocation())})),
            tr(Text::FieldMetLocation), 0, UINT16_MAX,
            [&pokemon] { return static_cast<unsigned int>(pokemon.metLocation()); },
            [&pokemon](unsigned int value) { pokemon.metLocation(static_cast<u16>(value)); }));

        if (generation >= pksm::Generation::FOUR)
        {
            addDateFields(fields, Text::FieldEggDay, Text::FieldEggMonth, Text::FieldEggYear,
                [&pokemon] { return pokemon.eggDate(); },
                [&pokemon](const Date& date) { pokemon.eggDate(date); });
            fields.push_back(numberField(
                display(tr(Text::FieldEggLocation),
                    tr(Text::LocationValue, {numericValue(pokemon.eggLocation())})),
                tr(Text::FieldEggLocation), 0, UINT16_MAX,
                [&pokemon] { return static_cast<unsigned int>(pokemon.eggLocation()); },
                [&pokemon](unsigned int value) { pokemon.eggLocation(static_cast<u16>(value)); }));
        }
        if (generation == pksm::Generation::EIGHT)
        {
            auto& pk8 = static_cast<pksm::PK8&>(pokemon);
            fields.push_back(counterField(tr(Text::FieldEnjoyment), tr(Text::FieldEnjoyment), 255,
                [&pk8] { return static_cast<unsigned int>(pk8.enjoyment()); },
                [&pk8](unsigned int value) { pk8.enjoyment(static_cast<u8>(value)); }));
            fields.push_back(counterField(tr(Text::FieldFullness), tr(Text::FieldFullness), 255,
                [&pk8] { return static_cast<unsigned int>(pk8.fullness()); },
                [&pk8](unsigned int value) { pk8.fullness(static_cast<u8>(value)); }));
        }

        for (size_t i = 0; i < stats.size(); ++i)
        {
            const pksm::Stat stat   = stats[i];
            const std::string label = tr(Text::FieldIv, {tr(statNames[i])});
            EditableField iv        = counterField(label, label, pokemon.maxIV(),
                       [&pokemon, stat] { return static_cast<unsigned int>(pokemon.iv(stat)); },
                       [&pokemon, stat](unsigned int value) { pokemon.iv(stat, static_cast<u8>(value)); });
            iv.wrap = true;
            fields.push_back(std::move(iv));
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
            fields.push_back(std::move(ev));
        }
        for (size_t i = 0; i < 4; ++i)
        {
            fields.push_back(moveField(pokemon, save, i, false));
        }
        for (size_t i = 0; i < 4; ++i)
        {
            fields.push_back(moveField(pokemon, save, i, true));
        }
        return fields;
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

    std::string pokemonSummary(const pksm::PKX& pokemon)
    {
        if (pokemon.species() == pksm::Species::None)
        {
            return tr(Text::Empty);
        }
        return tr(Text::PokemonSummary,
            {pokemon.species().localize(pokemon.language()), numericValue(pokemon.level())});
    }

    std::vector<Field> boxFields(pksm::Sav& save, u8 box)
    {
        std::vector<Field> fields;
        fields.reserve(30);
        for (u8 slot = 0; slot < 30; ++slot)
        {
            const auto pokemon = save.pkm(box, slot);
            fields.push_back({tr(Text::BoxSlot, {numericValue(slot + 1)}),
                pokemon ? pokemonSummary(*pokemon) : tr(Text::SlotReadFailed),
                pokemon ? static_cast<size_t>(pokemon->species()) : 0});
        }
        return fields;
    }

    std::vector<Field> partyFields(pksm::Sav& save)
    {
        const u8 count = std::min<u8>(6, static_cast<u8>(save.partyCount() + 1));
        std::vector<Field> fields;
        fields.reserve(count);
        for (u8 slot = 0; slot < count; ++slot)
        {
            const auto pokemon = save.pkm(slot);
            fields.push_back({tr(Text::PartySlot, {numericValue(slot + 1)}),
                pokemon ? pokemonSummary(*pokemon) : tr(Text::SlotReadFailed),
                pokemon ? static_cast<size_t>(pokemon->species()) : 0});
        }
        return fields;
    }
}
