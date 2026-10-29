#include "screens/TrainerEditorScreen.hpp"

#include "SwitchUi.hpp"
#include "app/App.hpp"
#include "app/Session.hpp"
#include "domain/FieldValues.hpp"
#include "domain/GameProfile.hpp"
#include "sav/Sav.hpp"
#include "screens/SaveSummary.hpp"
#include "ui/Text.hpp"
#include <cstdint>
#include <iterator>

namespace pksm::switchui
{
    namespace
    {
        // A number stepped with ZL/ZR by `step` and typed with the keyboard on A. `shown` is how
        // the card shows the value; plain digits without it.
        EditableField counter(Text key, unsigned int max, unsigned int step,
            std::function<unsigned int()> get, std::function<void(unsigned int)> set,
            const std::string& shown = {})
        {
            const char* label       = tr(key);
            const std::string value = shown.empty() ? std::to_string(get()) : shown;
            EditableField field =
                numberField(Field{label, value}, label, 0, max, std::move(get), std::move(set));
            field.step = step;
            return field;
        }

        // "4.521.141 ₽" or "₽4,521,141", as the language writes money.
        std::string moneyText(unsigned int money)
        {
            return tr(Text::MoneyValue, {groupThousands(money, tr(Text::ThousandsSeparator))});
        }

        // Play time is kept in hours, minutes and seconds; the card edits hours and minutes
        // together and leaves the seconds alone.
        constexpr unsigned int MAX_PLAY_MINUTES = UINT16_MAX * 60u + 59u;

        // The languages a save can be set to, in the games' own order.
        constexpr unsigned int LANGUAGES[] = {
            static_cast<unsigned int>(pksm::Language::JPN),
            static_cast<unsigned int>(pksm::Language::ENG),
            static_cast<unsigned int>(pksm::Language::FRE),
            static_cast<unsigned int>(pksm::Language::ITA),
            static_cast<unsigned int>(pksm::Language::GER),
            static_cast<unsigned int>(pksm::Language::SPA),
            static_cast<unsigned int>(pksm::Language::KOR),
            static_cast<unsigned int>(pksm::Language::CHS),
            static_cast<unsigned int>(pksm::Language::CHT),
        };

        std::string languageName(unsigned int language)
        {
            switch (static_cast<pksm::Language>(language))
            {
                case pksm::Language::JPN:
                    return tr(Text::LanguageJapanese);
                case pksm::Language::ENG:
                    return tr(Text::LanguageEnglish);
                case pksm::Language::FRE:
                    return tr(Text::LanguageFrench);
                case pksm::Language::ITA:
                    return tr(Text::LanguageItalian);
                case pksm::Language::GER:
                    return tr(Text::LanguageGerman);
                case pksm::Language::SPA:
                    return tr(Text::LanguageSpanish);
                case pksm::Language::KOR:
                    return tr(Text::LanguageKorean);
                case pksm::Language::CHS:
                    return tr(Text::LanguageChineseSimplified);
                case pksm::Language::CHT:
                    return tr(Text::LanguageChineseTraditional);
                default:
                    return tr(Text::LanguageUnknown);
            }
        }

        constexpr size_t COLUMNS = SwitchUi::TRAINER_COLUMNS;
    }

    TrainerEditorScreen::TrainerEditorScreen(Session& session, std::string game, std::string path)
        : session_(session), game_(std::move(game)), path_(std::move(path))
    {
    }

    std::vector<EditableField> TrainerEditorScreen::fields()
    {
        pksm::Sav& save            = session_.save();
        const GameProfile& profile = session_.profile();

        // Filled row by row into the grid, in the canvas order: name and play time, the two
        // IDs, gender and language, then money and battle points.
        std::vector<EditableField> fields;
        fields.push_back(textField(Field{tr(Text::TrainerName), save.otName()}, tr(Text::TrainerNameTitle),
            profile.trainerNameLengthFor(save.language()), [&save] { return save.otName(); },
            [&save](const std::string& name) { save.otName(name); }));
        const auto playMinutes = [&save]
        { return static_cast<unsigned int>(save.playedHours()) * 60u + save.playedMinutes(); };
        EditableField playTime = counter(Text::TrainerPlayTime, MAX_PLAY_MINUTES, 60, playMinutes,
            [&save](unsigned int value)
            {
                save.playedHours(static_cast<u16>(value / 60));
                save.playedMinutes(static_cast<u8>(value % 60));
            },
            formatPlayTime(playMinutes()));
        playTime.title = tr(Text::TrainerPlayTimeTitle);
        playTime.input = EditableField::NumberInput::PlayTime;
        fields.push_back(std::move(playTime));
        const pksm::Generation generation = save.generation();
        const bool shownSplit = generation == pksm::Generation::SEVEN ||
                                generation == pksm::Generation::LGPE ||
                                generation == pksm::Generation::EIGHT ||
                                generation == pksm::Generation::NINE;
        if (shownSplit)
        {
            // Edit the IDs the way the game shows them; both change the two stored values.
            const auto shown = [&save] { return showTrainerIds(save.TID(), save.SID()); };
            const auto store = [&save](ShownTrainerIds ids)
            {
                unsigned int tid = 0;
                unsigned int sid = 0;
                storeTrainerIds(ids, tid, sid);
                save.TID(static_cast<u16>(tid));
                save.SID(static_cast<u16>(sid));
            };
            fields.push_back(counter(Text::TrainerTid, MAX_SHOWN_TID, 1,
                [shown] { return shown().tid; },
                [shown, store](unsigned int value) { store({value, shown().sid}); },
                padDigits(shown().tid, 6)));
            fields.push_back(counter(Text::TrainerSid, MAX_SHOWN_SID, 1,
                [shown] { return shown().sid; },
                [shown, store](unsigned int value) { store({shown().tid, value}); },
                padDigits(shown().sid, 4)));
        }
        else
        {
            fields.push_back(counter(Text::TrainerTid, UINT16_MAX, 1, [&save] { return save.TID(); },
                [&save](unsigned int value) { save.TID(static_cast<u16>(value)); }));
            fields.push_back(counter(Text::TrainerSid, UINT16_MAX, 1, [&save] { return save.SID(); },
                [&save](unsigned int value) { save.SID(static_cast<u16>(value)); }));
        }

        const auto genderName = [](unsigned int value)
        {
            return std::string(tr(value == u8(pksm::Gender::Female) ? Text::TrainerFemale
                                                                    : Text::TrainerMale));
        };
        const unsigned int gender = u8(save.gender());
        fields.push_back(choiceField(Field{tr(Text::TrainerGender), genderName(gender)},
            tr(Text::TrainerGender),
            [] { return std::vector<unsigned int>{u8(pksm::Gender::Male), u8(pksm::Gender::Female)}; },
            genderName, [&save] { return static_cast<unsigned int>(u8(save.gender())); },
            [&save](unsigned int value) { save.gender(pksm::Gender{static_cast<u8>(value)}); }));

        const unsigned int language = static_cast<unsigned int>(save.language());
        const Field languageDisplay{tr(Text::TrainerLanguage), languageName(language)};
        // Generation III saves only tell Japanese from the rest, and cannot change it.
        if (save.generation() == pksm::Generation::THREE)
        {
            fields.push_back(readOnlyField(languageDisplay));
        }
        else
        {
            fields.push_back(choiceField(languageDisplay, tr(Text::TrainerLanguage),
                [] { return std::vector<unsigned int>(std::begin(LANGUAGES), std::end(LANGUAGES)); },
                languageName, [&save] { return static_cast<unsigned int>(save.language()); },
                [&save](unsigned int value) { save.language(static_cast<pksm::Language>(value)); }));
        }

        fields.push_back(counter(Text::TrainerMoney, profile.maxMoney, 1000, [&save] { return save.money(); },
            [&save](unsigned int value) { save.money(value); }, moneyText(save.money())));
        if (profile.hasBattlePoints)
        {
            fields.push_back(counter(Text::TrainerBattlePoints, 9999, 100, [&save] { return save.BP(); },
                [&save](unsigned int value) { save.BP(value); }));
        }
        return fields;
    }

    void TrainerEditorScreen::update(App& app, Action action)
    {
        const std::vector<EditableField> rows = fields();
        const auto onChanged = [this]
        {
            session_.markDirty();
            message_.clear();
        };
        switch (action)
        {
            case Action::Quit:
                if (session_.dirty())
                {
                    message_ = tr(Text::UnsavedBeforeQuit);
                }
                else
                {
                    app.quit();
                }
                break;
            // The D-pad moves around the grid and stops at its edges.
            case Action::Up:
                if (selected_ >= COLUMNS)
                {
                    selected_ -= COLUMNS;
                }
                break;
            case Action::Down:
                if (selected_ + COLUMNS < rows.size())
                {
                    selected_ += COLUMNS;
                }
                break;
            case Action::Left:
                if (selected_ % COLUMNS != 0)
                {
                    --selected_;
                }
                break;
            case Action::Right:
                if (selected_ % COLUMNS != COLUMNS - 1 && selected_ + 1 < rows.size())
                {
                    ++selected_;
                }
                break;
            case Action::Back:
                if (session_.dirty())
                {
                    message_ = tr(Text::UnsavedBeforeBack);
                }
                else
                {
                    app.pop();
                }
                break;
            case Action::Discard:
                app.closeSession();
                break;
            case Action::Save:
                if (session_.dirty())
                {
                    session_.saveWithMessage(message_);
                }
                break;
            case Action::Confirm:
                openField(rows[selected_], app, message_, onChanged);
                break;
            case Action::PreviousTab:
            case Action::NextTab:
                if (stepField(rows[selected_], action == Action::NextTab ? 1 : -1))
                {
                    onChanged();
                }
                break;
            default:
                break;
        }
    }

    void TrainerEditorScreen::draw(SwitchUi& ui)
    {
        ui.drawTrainer(summarizeSave(session_, path_), displayFields(fields()), selected_,
            sessionMessage(session_, message_));
    }
}
