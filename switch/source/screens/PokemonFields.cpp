#include "screens/PokemonFields.hpp"

#include "SwitchUi.hpp"
#include "enums/Stat.hpp"
#include "pkx/PK8.hpp"
#include "sav/Sav.hpp"
#include "utils/i18n.hpp"
#include "utils/utils.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <string>
#include <system_error>
#include <vector>

namespace pksm::switchui
{
    namespace
    {
        constexpr std::array<pksm::Stat, 6> stats = {pksm::Stat::HP, pksm::Stat::ATK,
            pksm::Stat::DEF, pksm::Stat::SPATK, pksm::Stat::SPDEF, pksm::Stat::SPD};
        constexpr std::array<const char*, 6> statNames = {
            "PS", "Ataque", "Defensa", "Ataque Esp.", "Defensa Esp.", "Velocidad"};
        constexpr std::array<const char*, 3> genderNames = {"Macho", "Hembra", "Sin genero"};
        constexpr std::array<const char*, 12> monthNames = {"Enero", "Febrero", "Marzo",
            "Abril", "Mayo", "Junio", "Julio", "Agosto", "Septiembre", "Octubre",
            "Noviembre", "Diciembre"};
    }

    std::string numericValue(unsigned int value)
    {
        return std::to_string(value);
    }

    std::vector<EditorField> makeFields(const pksm::PKX& pokemon)
    {
        const pksm::Language language = pokemon.language();
        std::vector<EditorField> fields;
        const auto add = [&fields](FieldKind kind, size_t index, std::string label,
                           std::string value, bool checkbox = false, bool checked = false)
        {
            Field display{std::move(label), std::move(value)};
            display.checkbox = checkbox;
            display.checked = checked;
            fields.push_back({kind, index, std::move(display)});
        };

        const unsigned int species = static_cast<unsigned int>(pokemon.species());
        add(FieldKind::Species, 0, "Especie",
            pokemon.species() == pksm::Species::None
                ? "Vacío"
                : pokemon.species().localize(language) + " (#" + numericValue(species) + ")");
        add(FieldKind::Nickname, 0, "Apodo", pokemon.nickname());
        add(FieldKind::Level, 0, "Nivel", numericValue(pokemon.level()));
        add(FieldKind::Nature, 0, "Naturaleza",
            pokemon.generation() >= pksm::Generation::THREE
                ? pokemon.nature().localize(language)
                : "No disponible");

        const u8 abilitySlot = pokemon.abilityNumber() >> 1;
        add(FieldKind::Ability, 0, "Habilidad",
            pokemon.ability().localize(language) + " [" + numericValue(abilitySlot) + "]");
        add(FieldKind::Item, 0, "Objeto",
            i18n::item(language, pokemon.heldItem()) + " (#" +
                numericValue(pokemon.heldItem()) + ")");
        add(FieldKind::Shiny, 0, "Variocolor", pokemon.shiny() ? "Sí" : "No", true,
            pokemon.shiny());
        add(FieldKind::Pokerus, 0, "Pokérus", pokemon.pkrsDays() > 0 ? "Sí" : "No", true,
            pokemon.pkrsDays() > 0);
        add(FieldKind::OriginalTrainer, 0, "Entrenador original", pokemon.otName());

        const unsigned int gender = static_cast<unsigned int>(pokemon.gender());
        add(FieldKind::Gender, 0, "Género",
            gender < genderNames.size() ? genderNames[gender] : "Desconocido");
        add(FieldKind::Friendship, 0, "Amistad", numericValue(pokemon.currentFriendship()));
        add(FieldKind::Ball, 0, "Poké Ball",
            pokemon.ball().localize(language) + " (#" +
                numericValue(static_cast<unsigned int>(pokemon.ball())) + ")");
        fields.back().display.hasBallSprite = true;
        const unsigned int ballIndex = static_cast<unsigned int>(pokemon.ball());
        fields.back().display.ballSpriteIndex = ballIndex <= 25 ? ballIndex : 0;
        add(FieldKind::Form, 0, "Forma",
            i18n::form(language, pokemon.version(), pokemon.species(),
                static_cast<u8>(pokemon.alternativeForm())) +
                " (#" + numericValue(pokemon.alternativeForm()) + ")");
        add(FieldKind::Egg, 0, "Huevo", pokemon.egg() ? "Sí" : "No", true, pokemon.egg());
        add(FieldKind::FatefulEncounter, 0, "Encuentro fatídico",
            pokemon.fatefulEncounter() ? "Sí" : "No", true, pokemon.fatefulEncounter());
        add(FieldKind::MetLevel, 0, "Nivel de encuentro", numericValue(pokemon.metLevel()));
        const Date metDate = pokemon.metDate();
        add(FieldKind::MetDay, 0, "Día de encuentro", numericValue(metDate.day()));
        add(FieldKind::MetMonth, 0, "Mes de encuentro", numericValue(metDate.month()));
        add(FieldKind::MetYear, 0, "Año de encuentro", numericValue(metDate.year()));
        add(FieldKind::MetLocation, 0, "Lugar de encuentro",
            "#" + numericValue(pokemon.metLocation()));
        if (pokemon.generation() >= pksm::Generation::FOUR)
        {
            const Date eggDate = pokemon.eggDate();
            add(FieldKind::EggDay, 0, "Día de obtención del huevo",
                numericValue(eggDate.day()));
            add(FieldKind::EggMonth, 0, "Mes de obtención del huevo",
                numericValue(eggDate.month()));
            add(FieldKind::EggYear, 0, "Año de obtención del huevo",
                numericValue(eggDate.year()));
            add(FieldKind::EggLocation, 0, "Lugar de obtención del huevo",
                "#" + numericValue(pokemon.eggLocation()));
        }
        if (pokemon.generation() == pksm::Generation::EIGHT)
        {
            const auto& pk8 = static_cast<const pksm::PK8&>(pokemon);
            add(FieldKind::Enjoyment, 0, "Satisfacción", numericValue(pk8.enjoyment()));
            add(FieldKind::Fullness, 0, "Saciedad", numericValue(pk8.fullness()));
        }

        for (size_t i = 0; i < stats.size(); ++i)
        {
            add(FieldKind::IV, i, std::string("IV ") + statNames[i],
                numericValue(pokemon.iv(stats[i])));
        }
        for (size_t i = 0; i < stats.size(); ++i)
        {
            add(FieldKind::EV, i, std::string("EV ") + statNames[i],
                numericValue(pokemon.secondaryStatCalc(stats[i])));
        }
        for (size_t i = 0; i < 4; ++i)
        {
            const pksm::Move move = pokemon.move(static_cast<u8>(i));
            add(FieldKind::Move, i, "Movimiento " + numericValue(static_cast<unsigned int>(i + 1)),
                move.localize(language) + " (#" + numericValue(static_cast<unsigned int>(move)) +
                    ")");
        }
        for (size_t i = 0; i < 4; ++i)
        {
            const pksm::Move move = pokemon.relearnMove(static_cast<u8>(i));
            add(FieldKind::RelearnMove, i,
                "Movimiento recuerdo " + numericValue(static_cast<unsigned int>(i + 1)),
                pokemon.generation() >= pksm::Generation::SIX
                    ? move.localize(language) + " (#" +
                          numericValue(static_cast<unsigned int>(move)) + ")"
                    : "No disponible en esta generación");
        }
        return fields;
    }

    namespace
    {
        bool parseNumber(
            const std::string& text, unsigned int maximum, unsigned int& value, std::string& error)
        {
            if (text.empty())
            {
                error = "Introduce un número.";
                return false;
            }
            unsigned int parsed = 0;
            const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
            if (result.ec != std::errc{} || result.ptr != text.data() + text.size() ||
                parsed > maximum)
            {
                error = "El número está fuera del rango permitido.";
                return false;
            }
            value = parsed;
            return true;
        }

        bool editNumber(SwitchUi& ui, const std::string& title, unsigned int current,
            unsigned int maximum, unsigned int& value, std::string& error)
        {
            std::string input;
            if (!ui.editText(title, numericValue(current), input, 10))
            {
                return false;
            }
            return parseNumber(input, maximum, value, error);
        }

        template <typename T>
        bool cycleValue(const std::vector<T>& values, T current, int direction, T& next)
        {
            if (values.empty())
            {
                return false;
            }
            const auto currentIt = std::find(values.begin(), values.end(), current);
            const size_t currentIndex =
                currentIt == values.end() ? 0 : static_cast<size_t>(currentIt - values.begin());
            const size_t nextIndex = direction > 0
                ? (currentIndex + 1) % values.size()
                : (currentIndex == 0 ? values.size() - 1 : currentIndex - 1);
            next = values[nextIndex];
            return next != current;
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

        template <typename T, typename Label>
        bool selectValue(SwitchUi& ui, const std::string& title, const std::vector<T>& values,
            T current, Label label, T& next, const std::vector<size_t>& spriteIndices = {})
        {
            if (values.empty())
            {
                return false;
            }
            std::vector<std::string> options;
            options.reserve(values.size());
            for (const T& value : values)
            {
                options.push_back(label(value));
            }
            const auto currentIt = std::find(values.begin(), values.end(), current);
            const size_t currentIndex = currentIt == values.end()
                ? 0
                : static_cast<size_t>(currentIt - values.begin());
            size_t selected = currentIndex;
            if (!ui.selectOption(title, options, currentIndex, selected, spriteIndices))
            {
                return false;
            }
            next = values[selected];
            return next != current;
        }

        bool selectNumber(SwitchUi& ui, const std::string& title, unsigned int minimum,
            unsigned int maximum, unsigned int current, unsigned int& next)
        {
            std::vector<unsigned int> values;
            std::vector<std::string> options;
            values.reserve(static_cast<size_t>(maximum - minimum) + 1);
            options.reserve(values.capacity());
            for (unsigned int value = minimum; value <= maximum; ++value)
            {
                values.push_back(value);
                options.push_back(numericValue(value));
            }
            const size_t currentIndex = current >= minimum && current <= maximum
                ? static_cast<size_t>(current - minimum)
                : 0;
            size_t selected = currentIndex;
            if (!ui.selectOption(title, options, currentIndex, selected))
            {
                return false;
            }
            next = values[selected];
            return next != current;
        }
    }

    bool changeField(EditorField& field, pksm::PKX& pokemon, pksm::Sav& save, SwitchUi& ui,
        int direction, std::string& error)
    {
        const std::vector<pksm::Species> species(
            save.availableSpecies().begin(), save.availableSpecies().end());
        const std::vector<pksm::Move> moves(save.availableMoves().begin(),
            save.availableMoves().end());
        const std::vector<pksm::Ball> balls(
            save.availableBalls().begin(), save.availableBalls().end());

        switch (field.kind)
        {
            case FieldKind::Species:
            {
                pksm::Species next;
                if (direction == 0)
                {
                    if (!selectValue(ui, "Especie", species, pokemon.species(),
                            [&pokemon](pksm::Species value)
                            {
                                return value.localize(pokemon.language()) + " (#" +
                                       numericValue(static_cast<unsigned int>(value)) + ")";
                            },
                            next))
                    {
                        return false;
                    }
                }
                else if (!cycleValue(species, pokemon.species(), direction, next))
                {
                    return false;
                }
                return setSpecies(pokemon, next);
            }
            case FieldKind::Nickname:
            case FieldKind::OriginalTrainer:
            {
                if (direction != 0)
                {
                    return false;
                }
                const bool nickname = field.kind == FieldKind::Nickname;
                const std::string current = nickname ? pokemon.nickname() : pokemon.otName();
                std::string result;
                const size_t maxLength = nickname
                    ? (pokemon.generation() >= pksm::Generation::SIX ? 12 : 10)
                    : (pokemon.generation() >= pksm::Generation::SIX ? 12 :
                       pokemon.language() == pksm::Language::JPN ? 5 : 7);
                if (!ui.editText(nickname ? "Apodo" : "Entrenador original",
                        current, result, maxLength) ||
                    result == current)
                {
                    return false;
                }
                if (nickname)
                {
                    pokemon.nickname(result);
                    std::string speciesName = pokemon.species().localize(pokemon.language());
                    if (pokemon.generation() <= pksm::Generation::FOUR)
                    {
                        StringUtils::toUpper(speciesName);
                    }
                    pokemon.nicknamed(result != speciesName);
                }
                else
                {
                    pokemon.otName(result);
                }
                return true;
            }
            case FieldKind::Level:
            {
                const unsigned int minimum =
                    pokemon.generation() > pksm::Generation::THREE ? 1 : 2;
                unsigned int value = pokemon.level();
                if (direction == 0)
                {
                    if (!editNumber(ui, "Nivel", value, 100, value, error))
                    {
                        return false;
                    }
                    value = std::max(minimum, value);
                }
                else
                {
                    const unsigned int changed = direction > 0
                        ? std::min(100U, value + 1)
                        : (value > minimum ? value - 1 : value);
                    value = changed;
                }
                if (pokemon.level() == value)
                {
                    return false;
                }
                pokemon.level(static_cast<u8>(value));
                return true;
            }
            case FieldKind::Nature:
            {
                if (pokemon.generation() < pksm::Generation::THREE)
                {
                    return false;
                }
                std::vector<pksm::Nature> natures;
                natures.reserve(25);
                for (u8 value = 0; value < 25; ++value)
                {
                    natures.emplace_back(value);
                }
                if (direction == 0)
                {
                    std::vector<std::string> natureNames;
                    natureNames.reserve(natures.size());
                    for (const pksm::Nature nature : natures)
                    {
                        natureNames.push_back(nature.localize(pokemon.language()));
                    }
                    size_t selected = static_cast<unsigned int>(pokemon.nature());
                    if (!ui.selectNature(natureNames, selected, selected))
                    {
                        return false;
                    }
                    if (selected == static_cast<unsigned int>(pokemon.nature()))
                    {
                        return false;
                    }
                    pokemon.nature(pksm::Nature{static_cast<u8>(selected)});
                    return true;
                }
                unsigned int value = static_cast<unsigned int>(pokemon.nature());
                value = direction > 0 ? (value + 1) % 25 : (value == 0 ? 24 : value - 1);
                if (static_cast<unsigned int>(pokemon.nature()) == value)
                {
                    return false;
                }
                pokemon.nature(pksm::Nature{static_cast<u8>(value)});
                return true;
            }
            case FieldKind::Ability:
            {
                std::vector<u8> slots;
                for (u8 i = 0; i < 3; ++i)
                {
                    if (pokemon.abilities(i) != pksm::Ability::None)
                    {
                        slots.push_back(i);
                    }
                }
                if (slots.empty())
                {
                    return false;
                }
                u8 current = pokemon.abilityNumber() >> 1;
                u8 next = current;
                if (direction == 0)
                {
                    if (!selectValue(ui, "Habilidad", slots, current,
                            [&pokemon](u8 slot)
                            {
                                return pokemon.abilities(slot).localize(pokemon.language()) +
                                       " [Ranura " + numericValue(slot) + "]";
                            },
                            next))
                    {
                        return false;
                    }
                }
                else
                {
                    const auto currentIt = std::find(slots.begin(), slots.end(), current);
                    const size_t index = currentIt == slots.end()
                        ? 0
                        : static_cast<size_t>(currentIt - slots.begin());
                    const size_t nextIndex = direction > 0
                        ? (index + 1) % slots.size()
                        : (index == 0 ? slots.size() - 1 : index - 1);
                    next = slots[nextIndex];
                }
                if (next == current)
                {
                    return false;
                }
                pokemon.setAbility(next);
                return true;
            }
            case FieldKind::Item:
            {
                std::vector<u16> items{0};
                for (int item : save.availableItems())
                {
                    if (item > 0 && item <= UINT16_MAX)
                    {
                        items.push_back(static_cast<u16>(item));
                    }
                }
                std::sort(items.begin(), items.end());
                items.erase(std::unique(items.begin(), items.end()), items.end());
                u16 next = pokemon.heldItem();
                if (direction == 0)
                {
                    if (!selectValue(ui, "Objeto", items, pokemon.heldItem(),
                            [&pokemon](u16 item)
                            {
                                return i18n::item(pokemon.language(), item) + " (#" +
                                       numericValue(item) + ")";
                            },
                            next))
                    {
                        return false;
                    }
                }
                else if (!cycleValue(items, pokemon.heldItem(), direction, next))
                {
                    return false;
                }
                if (next == pokemon.heldItem())
                {
                    return false;
                }
                pokemon.heldItem(next);
                return true;
            }
            case FieldKind::Shiny:
                pokemon.shiny(!pokemon.shiny());
                return true;
            case FieldKind::Pokerus:
                pokemon.pkrs(pokemon.pkrs() == 0 ? 0xF4 : 0);
                return true;
            case FieldKind::Gender:
            {
                std::vector<pksm::Gender> genders;
                genders.reserve(genderNames.size());
                for (u8 value = 0; value < genderNames.size(); ++value)
                {
                    genders.emplace_back(value);
                }
                pksm::Gender next = pokemon.gender();
                if (direction == 0)
                {
                    if (!selectValue(ui, "Género", genders, pokemon.gender(),
                            [](pksm::Gender value)
                            {
                                const unsigned int id = static_cast<unsigned int>(value);
                                return id < genderNames.size() ? genderNames[id] :
                                                                 "Desconocido";
                            },
                            next))
                    {
                        return false;
                    }
                }
                else
                {
                    const unsigned int current = static_cast<unsigned int>(pokemon.gender());
                    const unsigned int nextId = direction > 0
                        ? (current + 1) % genderNames.size()
                        : (current == 0 ? genderNames.size() - 1 : current - 1);
                    next = pksm::Gender{static_cast<u8>(nextId)};
                }
                if (next == pokemon.gender())
                {
                    return false;
                }
                pokemon.gender(next);
                return true;
            }
            case FieldKind::Friendship:
            {
                unsigned int value = pokemon.currentFriendship();
                if (direction == 0)
                {
                    if (!editNumber(ui, "Amistad (0-255)", value, 255, value, error))
                    {
                        return false;
                    }
                }
                else
                {
                    value = direction > 0 ? std::min(255U, value + 1) :
                                           (value == 0 ? 0 : value - 1);
                }
                if (pokemon.currentFriendship() == value)
                {
                    return false;
                }
                pokemon.currentFriendship(static_cast<u8>(value));
                return true;
            }
            case FieldKind::Ball:
            {
                pksm::Ball next = pokemon.ball();
                if (direction == 0)
                {
                    std::vector<size_t> ballSpriteIndices;
                    ballSpriteIndices.reserve(balls.size());
                    for (const pksm::Ball ball : balls)
                    {
                        const unsigned int ballIndex = static_cast<unsigned int>(ball);
                        ballSpriteIndices.push_back(ballIndex <= 25 ? ballIndex : 0);
                    }
                    if (!selectValue(ui, "Poké Ball", balls, pokemon.ball(),
                            [&pokemon](pksm::Ball value)
                            {
                                return value.localize(pokemon.language()) + " (#" +
                                       numericValue(static_cast<unsigned int>(value)) + ")";
                            },
                            next, ballSpriteIndices))
                    {
                        return false;
                    }
                }
                else if (!cycleValue(balls, pokemon.ball(), direction, next))
                {
                    return false;
                }
                if (next == pokemon.ball())
                {
                    return false;
                }
                pokemon.ball(next);
                return true;
            }
            case FieldKind::Egg:
                pokemon.egg(!pokemon.egg());
                return true;
            case FieldKind::FatefulEncounter:
                pokemon.fatefulEncounter(!pokemon.fatefulEncounter());
                return true;
            case FieldKind::MetLevel:
            {
                unsigned int value = pokemon.metLevel();
                if (direction == 0)
                {
                    if (!editNumber(ui, "Nivel de encuentro", value, 100, value, error))
                    {
                        return false;
                    }
                }
                else
                {
                    value = direction > 0 ? std::min(100U, value + 1) :
                                           (value == 0 ? 0 : value - 1);
                }
                if (pokemon.metLevel() == value)
                {
                    return false;
                }
                pokemon.metLevel(static_cast<u8>(value));
                return true;
            }
            case FieldKind::MetDay:
            case FieldKind::MetMonth:
            case FieldKind::MetYear:
            case FieldKind::EggDay:
            case FieldKind::EggMonth:
            case FieldKind::EggYear:
            {
                const bool met = field.kind == FieldKind::MetDay ||
                                 field.kind == FieldKind::MetMonth ||
                                 field.kind == FieldKind::MetYear;
                Date date = met ? pokemon.metDate() : pokemon.eggDate();
                const Date originalDate = date;
                unsigned int value = field.kind == FieldKind::MetDay ||
                                             field.kind == FieldKind::EggDay
                    ? date.day()
                    : field.kind == FieldKind::MetMonth || field.kind == FieldKind::EggMonth
                    ? date.month()
                    : date.year();
                const unsigned int maximum = field.kind == FieldKind::MetDay ||
                                                     field.kind == FieldKind::EggDay
                    ? 31
                    : field.kind == FieldKind::MetMonth ||
                              field.kind == FieldKind::EggMonth
                    ? 12
                    : 9999;
                if (direction == 0)
                {
                    const bool month = field.kind == FieldKind::MetMonth ||
                                       field.kind == FieldKind::EggMonth;
                    unsigned int next = value;
                    const bool changed = month
                        ? [&]()
                          {
                              std::vector<unsigned int> months;
                              months.reserve(monthNames.size());
                              for (unsigned int monthValue = 1;
                                   monthValue <= monthNames.size(); ++monthValue)
                              {
                                  months.push_back(monthValue);
                              }
                              return selectValue(ui, field.display.label, months, value,
                                  [](unsigned int monthValue)
                                  {
                                      return monthNames[monthValue - 1];
                                  },
                                  next);
                          }()
                        : selectNumber(ui, field.display.label,
                              maximum == 9999 ? 0 : 1, maximum, value, next);
                    if (!changed)
                    {
                        return false;
                    }
                    value = next;
                }
                else
                {
                    const unsigned int minimum = maximum == 9999 ? 0 : 1;
                    value = direction > 0 ? std::min(maximum, value + 1) :
                                           (value <= minimum ? minimum : value - 1);
                }
                if (field.kind == FieldKind::MetDay || field.kind == FieldKind::EggDay)
                {
                    date.day(static_cast<u8>(value));
                }
                else if (field.kind == FieldKind::MetMonth ||
                         field.kind == FieldKind::EggMonth)
                {
                    date.month(static_cast<u8>(value));
                }
                else
                {
                    date.year(value);
                }
                if (date == originalDate)
                {
                    return false;
                }
                if (met)
                {
                    pokemon.metDate(date);
                }
                else
                {
                    pokemon.eggDate(date);
                }
                return true;
            }
            case FieldKind::MetLocation:
            case FieldKind::EggLocation:
            {
                const bool met = field.kind == FieldKind::MetLocation;
                const u16 current = met ? pokemon.metLocation() : pokemon.eggLocation();
                unsigned int value = current;
                if (direction == 0)
                {
                    if (!editNumber(ui, field.display.label, value, UINT16_MAX, value, error))
                    {
                        return false;
                    }
                }
                else
                {
                    value = direction > 0 ? std::min<unsigned int>(UINT16_MAX, value + 1) :
                                           (value == 0 ? 0 : value - 1);
                }
                if (current == value)
                {
                    return false;
                }
                if (met)
                {
                    pokemon.metLocation(static_cast<u16>(value));
                }
                else
                {
                    pokemon.eggLocation(static_cast<u16>(value));
                }
                return true;
            }
            case FieldKind::Enjoyment:
            case FieldKind::Fullness:
            {
                auto& pk8 = static_cast<pksm::PK8&>(pokemon);
                const bool enjoyment = field.kind == FieldKind::Enjoyment;
                unsigned int value = enjoyment ? pk8.enjoyment() : pk8.fullness();
                if (direction == 0)
                {
                    if (!editNumber(ui, field.display.label, value, 255, value, error))
                    {
                        return false;
                    }
                }
                else
                {
                    value = direction > 0 ? std::min(255U, value + 1) :
                                           (value == 0 ? 0 : value - 1);
                }
                if (value == (enjoyment ? pk8.enjoyment() : pk8.fullness()))
                {
                    return false;
                }
                if (enjoyment)
                {
                    pk8.enjoyment(static_cast<u8>(value));
                }
                else
                {
                    pk8.fullness(static_cast<u8>(value));
                }
                return true;
            }
            case FieldKind::Form:
            {
                if (pokemon.species() == pksm::Species::None)
                {
                    return false;
                }
                const unsigned int count = save.formCount(pokemon.species());
                if (count < 2)
                {
                    return false;
                }
                std::vector<unsigned int> forms;
                forms.reserve(count);
                for (unsigned int form = 0; form < count; ++form)
                {
                    forms.push_back(form);
                }
                unsigned int value = pokemon.alternativeForm();
                if (direction == 0)
                {
                    if (!selectValue(ui, "Forma", forms, value,
                            [&pokemon, &save](unsigned int form)
                            {
                                return i18n::form(pokemon.language(), save.version(),
                                           pokemon.species(), static_cast<u8>(form)) +
                                       " (#" + numericValue(form) + ")";
                            },
                            value))
                    {
                        return false;
                    }
                }
                else
                {
                    value = direction > 0 ? (value + 1) % count :
                                            (value == 0 ? count - 1 : value - 1);
                }
                if (pokemon.alternativeForm() == value)
                {
                    return false;
                }
                pokemon.alternativeForm(static_cast<u16>(value));
                return true;
            }
            case FieldKind::IV:
            {
                const pksm::Stat stat = stats[field.index];
                unsigned int value = pokemon.iv(stat);
                if (direction == 0)
                {
                    if (!editNumber(ui, field.display.label, value, pokemon.maxIV(), value, error))
                    {
                        return false;
                    }
                }
                else
                {
                    value = direction > 0
                        ? (value == pokemon.maxIV() ? 0 : value + 1)
                        : (value == 0 ? pokemon.maxIV() : value - 1);
                }
                if (pokemon.iv(stat) == value)
                {
                    return false;
                }
                pokemon.iv(stat, static_cast<u8>(value));
                return true;
            }
            case FieldKind::EV:
            {
                const pksm::Stat stat = stats[field.index];
                unsigned int total = 0;
                for (pksm::Stat other : stats)
                {
                    if (other != stat)
                    {
                        total += pokemon.secondaryStatCalc(other);
                    }
                }
                const unsigned int maxValue =
                    std::min<unsigned int>(pokemon.maxSecondaryStatCalc(),
                        pokemon.maxSecondaryStatCalcTotal() > total
                            ? pokemon.maxSecondaryStatCalcTotal() - total
                            : 0);
                unsigned int value = pokemon.secondaryStatCalc(stat);
                if (direction == 0)
                {
                    if (!editNumber(ui, field.display.label, value,
                            pokemon.maxSecondaryStatCalc(), value, error))
                    {
                        return false;
                    }
                    value = std::min(value, maxValue);
                }
                else if (direction > 0)
                {
                    value = value >= maxValue ? 0 : value + 1;
                }
                else
                {
                    value = value == 0 ? maxValue : value - 1;
                }
                if (pokemon.secondaryStatCalc(stat) == value)
                {
                    return false;
                }
                pokemon.secondaryStatCalc(stat, static_cast<u16>(value));
                return true;
            }
            case FieldKind::Move:
            case FieldKind::RelearnMove:
            {
                const bool relearn = field.kind == FieldKind::RelearnMove;
                if (relearn && pokemon.generation() < pksm::Generation::SIX)
                {
                    return false;
                }
                const u8 slot = static_cast<u8>(field.index);
                pksm::Move current = relearn ? pokemon.relearnMove(slot) : pokemon.move(slot);
                pksm::Move next = current;
                if (direction == 0)
                {
                    if (!selectValue(ui, "Movimiento", moves, current,
                            [&pokemon](pksm::Move value)
                            {
                                return value.localize(pokemon.language()) + " (#" +
                                       numericValue(static_cast<unsigned int>(value)) + ")";
                            },
                            next))
                    {
                        return false;
                    }
                }
                else if (!cycleValue(moves, current, direction, next))
                {
                    return false;
                }
                if (next == current)
                {
                    return false;
                }
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
                return true;
            }
        }
        return false;
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
            return "Vacío";
        }
        return pokemon.species().localize(pokemon.language()) + "  Nv. " +
               numericValue(pokemon.level());
    }

    std::vector<Field> boxFields(pksm::Sav& save, u8 box)
    {
        std::vector<Field> fields;
        fields.reserve(30);
        for (u8 slot = 0; slot < 30; ++slot)
        {
            const auto pokemon = save.pkm(box, slot);
            fields.push_back({"Espacio " + numericValue(slot + 1),
                pokemon ? pokemonSummary(*pokemon) : "Error al leer",
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
            fields.push_back({"Equipo " + numericValue(slot + 1),
                pokemon ? pokemonSummary(*pokemon) : "Error al leer",
                pokemon ? static_cast<size_t>(pokemon->species()) : 0});
        }
        return fields;
    }
}
