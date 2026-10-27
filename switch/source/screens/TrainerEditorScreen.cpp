#include "screens/TrainerEditorScreen.hpp"

#include "SwitchUi.hpp"
#include "app/App.hpp"
#include "app/Session.hpp"
#include "domain/GameProfile.hpp"
#include "sav/Sav.hpp"
#include "ui/Text.hpp"
#include <cstdint>

namespace pksm::switchui
{
    namespace
    {
        // Stepped with L/R only; A does nothing on these rows.
        EditableField counter(Text key, unsigned int max, unsigned int step,
            std::function<unsigned int()> get, std::function<void(unsigned int)> set)
        {
            const char* label       = tr(key);
            const std::string value = std::to_string(get());
            EditableField field =
                numberField(Field{label, value}, label, 0, max, std::move(get), std::move(set));
            field.step  = step;
            field.input = EditableField::NumberInput::None;
            return field;
        }
    }

    TrainerEditorScreen::TrainerEditorScreen(Session& session, std::string game, std::string path)
        : session_(session), game_(std::move(game)), path_(std::move(path))
    {
    }

    std::vector<EditableField> TrainerEditorScreen::fields()
    {
        pksm::Sav& save            = session_.save();
        const GameProfile& profile = session_.profile();

        std::vector<EditableField> fields;
        fields.push_back(textField(Field{tr(Text::TrainerName), save.otName()}, tr(Text::TrainerNameTitle),
            profile.trainerNameLengthFor(save.language()), [&save] { return save.otName(); },
            [&save](const std::string& name) { save.otName(name); }));
        fields.push_back(counter(Text::TrainerTid, UINT16_MAX, 1, [&save] { return save.TID(); },
            [&save](unsigned int value) { save.TID(static_cast<u16>(value)); }));
        fields.push_back(counter(Text::TrainerSid, UINT16_MAX, 1, [&save] { return save.SID(); },
            [&save](unsigned int value) { save.SID(static_cast<u16>(value)); }));
        fields.push_back(counter(Text::TrainerMoney, profile.maxMoney, 1000, [&save] { return save.money(); },
            [&save](unsigned int value) { save.money(value); }));
        if (profile.hasBattlePoints)
        {
            fields.push_back(counter(Text::TrainerBattlePoints, 9999, 100, [&save] { return save.BP(); },
                [&save](unsigned int value) { save.BP(value); }));
        }
        fields.push_back(counter(Text::TrainerHours, UINT16_MAX, 1,
            [&save] { return save.playedHours(); },
            [&save](unsigned int value) { save.playedHours(static_cast<u16>(value)); }));
        EditableField minutes = counter(Text::TrainerMinutes, 59, 1,
            [&save] { return save.playedMinutes(); },
            [&save](unsigned int value) { save.playedMinutes(static_cast<u8>(value)); });
        minutes.wrap = true;
        fields.push_back(std::move(minutes));
        EditableField seconds = counter(Text::TrainerSeconds, 59, 1,
            [&save] { return save.playedSeconds(); },
            [&save](unsigned int value) { save.playedSeconds(static_cast<u8>(value)); });
        seconds.wrap = true;
        fields.push_back(std::move(seconds));
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
            case Action::Up:
                selected_ = selected_ == 0 ? rows.size() - 1 : selected_ - 1;
                break;
            case Action::Down:
                selected_ = (selected_ + 1) % rows.size();
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
                    session_.exportWithMessage(message_);
                }
                break;
            case Action::Confirm:
                openField(rows[selected_], app, message_, onChanged);
                break;
            case Action::Left:
            case Action::Right:
                if (stepField(rows[selected_], action == Action::Right ? 1 : -1))
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
        ui.drawTrainer(game_, path_, displayFields(fields()), selected_, session_.dirty(), message_);
    }
}
