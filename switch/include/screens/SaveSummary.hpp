#pragma once

#include "SwitchUi.hpp"
#include <string>

namespace pksm::switchui
{
    class Session;

    // What the side panel shows of the save open in `session`; `path` is relative to the save
    // folder.
    SaveSummary summarizeSave(Session& session, const std::string& path);

    // `message` when there is one; otherwise a note about unsaved changes, if any.
    std::string sessionMessage(const Session& session, const std::string& message);
}
