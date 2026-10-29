#include "screens/SaveSummary.hpp"

#include "app/App.hpp"
#include "app/Session.hpp"
#include "domain/FieldValues.hpp"
#include "domain/GameProfile.hpp"
#include "platform/SaveStorage.hpp"
#include "sav/Sav.hpp"
#include "screens/PickerScreens.hpp"
#include "ui/Text.hpp"
#include <memory>
#include <vector>

namespace pksm::switchui
{
    SaveSummary summarizeSave(Session& session, const std::string& path)
    {
        pksm::Sav& save            = session.save();
        const GameProfile& profile = session.profile();
        const size_t slash         = path.find_last_of('/');

        SaveSummary summary;
        summary.game    = GameCard{tr(profile.name), profile.mascotSpecies, 0, profile.artColor};
        summary.group   = tr(profile.group);
        summary.file    = slash == std::string::npos ? path : path.substr(slash + 1);
        // From generation VII on the game pads the IDs with zeros: 006800 and 3144.
        const pksm::Generation generation = save.generation();
        const bool padded = generation == pksm::Generation::SEVEN ||
                            generation == pksm::Generation::LGPE ||
                            generation == pksm::Generation::EIGHT ||
                            generation == pksm::Generation::NINE;
        summary.details   = tr(Text::TrainerSummary,
              {save.otName(), padDigits(save.displayTID(), padded ? 6 : 0),
                  padDigits(save.displaySID(), padded ? 4 : 0)});
        if (const auto& console = session.consoleLocation())
        {
            summary.file = tr(Text::ConsoleSaveTitle);
            summary.path = tr(Text::ConsoleSaveOf, {console->user.name});
        }
        else
        {
            summary.path = storage::savePath(path);
        }
        return summary;
    }

    std::string sessionMessage(const Session& session, const std::string& message)
    {
        if (!message.empty())
        {
            return message;
        }
        return session.dirty() ? std::string(tr(Text::PendingChanges)) : std::string{};
    }

    void leaveAsking(App& app, Session& session, std::string& message)
    {
        if (!session.dirty())
        {
            app.pop();
            return;
        }
        constexpr size_t SAVE = 0;
        std::vector<std::string> options{tr(Text::LeaveSave), tr(Text::LeaveDiscard)};
        // The dialog closes before it calls back; the screen to leave is on top again then.
        app.push(std::make_unique<OptionPickerScreen>(tr(Text::LeaveTitle), std::move(options),
            SAVE, std::vector<size_t>{},
            [&app, &session, &message](size_t index)
            {
                std::string error;
                const bool done = index == SAVE ? session.saveWithMessage(message)
                                                : session.discardChanges(error);
                if (done)
                {
                    app.pop();
                }
                else if (index != SAVE)
                {
                    message = error;
                }
            }));
    }
}
