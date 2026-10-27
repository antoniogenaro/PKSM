#include "screens/TrainerEditorScreen.hpp"

#include "SwitchUi.hpp"
#include "app/App.hpp"
#include "app/Session.hpp"
#include "domain/GameProfile.hpp"
#include "sav/Sav.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>

namespace pksm::switchui
{
    TrainerEditorScreen::TrainerEditorScreen(Session& session, std::string game, std::string path)
        : session_(session), game_(std::move(game)), path_(std::move(path))
    {
    }

    size_t TrainerEditorScreen::rowCount() const
    {
        return session_.profile().hasBattlePoints ? 8 : 7;
    }

    TrainerEditorScreen::Row TrainerEditorScreen::rowAt(size_t index) const
    {
        // Games without BP skip that row.
        if (!session_.profile().hasBattlePoints && index >= static_cast<size_t>(Row::BP))
        {
            ++index;
        }
        return static_cast<Row>(index);
    }

    void TrainerEditorScreen::update(App& app, Action action)
    {
        pksm::Sav& save = session_.save();
        switch (action)
        {
            case Action::Quit:
                if (session_.dirty())
                {
                    message_ = "Exporta con Y o descarta con X antes de salir.";
                }
                else
                {
                    app.quit();
                }
                break;
            case Action::Up:
                selected_ = selected_ == 0 ? rowCount() - 1 : selected_ - 1;
                break;
            case Action::Down:
                selected_ = (selected_ + 1) % rowCount();
                break;
            case Action::Back:
                if (session_.dirty())
                {
                    message_ = "Exporta con Y o descarta con X antes de volver.";
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
                if (rowAt(selected_) == Row::Name)
                {
                    std::string name;
                    const size_t maxLength =
                        session_.profile().trainerNameLengthFor(save.language());
                    if (app.ui().editTrainerName(save.otName(), name, maxLength) &&
                        name != save.otName())
                    {
                        save.otName(name);
                        session_.markDirty();
                        message_.clear();
                    }
                }
                break;
            case Action::Left:
            case Action::Right:
                if (step(rowAt(selected_), action == Action::Right))
                {
                    session_.markDirty();
                    message_.clear();
                }
                break;
            default:
                break;
        }
    }

    bool TrainerEditorScreen::step(Row row, bool increase)
    {
        pksm::Sav& save = session_.save();
        switch (row)
        {
            case Row::Name:
                return false;
            case Row::TID:
                if ((increase && save.TID() < UINT16_MAX) || (!increase && save.TID() > 0))
                {
                    save.TID(static_cast<u16>(increase ? save.TID() + 1 : save.TID() - 1));
                    return true;
                }
                return false;
            case Row::SID:
                if ((increase && save.SID() < UINT16_MAX) || (!increase && save.SID() > 0))
                {
                    save.SID(static_cast<u16>(increase ? save.SID() + 1 : save.SID() - 1));
                    return true;
                }
                return false;
            case Row::Money:
            {
                const u32 current = save.money();
                const u32 next    = increase ? std::min(session_.profile().maxMoney, current + 1000)
                                             : (current > 1000 ? current - 1000 : 0);
                if (next == current)
                {
                    return false;
                }
                save.money(next);
                return true;
            }
            case Row::BP:
            {
                const u32 current = save.BP();
                const u32 next    = increase ? std::min<u32>(9999, current + 100)
                                             : (current > 100 ? current - 100 : 0);
                if (next == current)
                {
                    return false;
                }
                save.BP(next);
                return true;
            }
            case Row::Hours:
                if ((increase && save.playedHours() < UINT16_MAX) ||
                    (!increase && save.playedHours() > 0))
                {
                    save.playedHours(static_cast<u16>(
                        increase ? save.playedHours() + 1 : save.playedHours() - 1));
                    return true;
                }
                return false;
            case Row::Minutes:
            {
                const u8 current = save.playedMinutes();
                const u8 next    = increase ? static_cast<u8>((current + 1) % 60)
                                            : static_cast<u8>(current == 0 ? 59 : current - 1);
                if (next == current)
                {
                    return false;
                }
                save.playedMinutes(next);
                return true;
            }
            case Row::Seconds:
            {
                const u8 current = save.playedSeconds();
                const u8 next    = increase ? static_cast<u8>((current + 1) % 60)
                                            : static_cast<u8>(current == 0 ? 59 : current - 1);
                if (next == current)
                {
                    return false;
                }
                save.playedSeconds(next);
                return true;
            }
        }
        return false;
    }

    void TrainerEditorScreen::draw(SwitchUi& ui)
    {
        pksm::Sav& save = session_.save();
        std::vector<Field> fields{
            {"Nombre", save.otName()},
            {"TID", std::to_string(save.TID())},
            {"SID", std::to_string(save.SID())},
            {"Dinero", std::to_string(save.money())},
        };
        if (session_.profile().hasBattlePoints)
        {
            fields.push_back({"BP", std::to_string(save.BP())});
        }
        fields.push_back({"Horas jugadas", std::to_string(save.playedHours())});
        fields.push_back({"Minutos jugados", std::to_string(save.playedMinutes())});
        fields.push_back({"Segundos jugados", std::to_string(save.playedSeconds())});
        ui.drawTrainer(game_, path_, fields, selected_, session_.dirty(), message_);
    }
}
