#pragma once

#include "SwitchUi.hpp"
#include <string>

namespace pksm::switchui
{
    class App;
    class Session;

    // What the side panel shows of the save open in `session`; `path` is relative to the save
    // folder.
    SaveSummary summarizeSave(Session& session, const std::string& path);

    // `message` when there is one; otherwise a note about unsaved changes, if any.
    std::string sessionMessage(const Session& session, const std::string& message);

    // Leaves the screen on top. With unsaved changes it first asks whether to save or discard
    // them, and leaves once that is done; B in the dialog stays. Errors go to `message`.
    void leaveAsking(App& app, Session& session, std::string& message);
}
