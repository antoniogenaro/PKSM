#pragma once

#include "enums/GameVersion.hpp"
#include "platform/ConsoleSaves.hpp"
#include <memory>
#include <optional>
#include <string>

namespace pksm
{
    class Sav;
}

namespace pksm::switchui
{
    struct GameProfile;

    // An open save: the parsed data, where it came from (a file on the SD card or a user's
    // save on the console), its game and whether it has changes that were not saved yet.
    class Session
    {
    public:
        // Reads and parses the file. Returns nullptr, with a message in `error`, when it
        // cannot be read or is not a supported game.
        static std::unique_ptr<Session> open(const std::string& path, std::string& error);

        // Same for a user's save on the console.
        static std::unique_ptr<Session> openConsole(
            const console::SaveLocation& location, std::string& error);

        // The game a file is a save of, without preparing it for editing. Nothing when it
        // cannot be read or is not a save PKSM recognizes, supported here or not.
        static std::optional<pksm::GameVersion> identify(const std::string& path);

        ~Session();
        Session(const Session&)            = delete;
        Session& operator=(const Session&) = delete;

        pksm::Sav& save() { return *save_; }
        const GameProfile& profile() const { return *profile_; }
        // The SD file it was read from; empty for a console save.
        const std::string& sourcePath() const { return sourcePath_; }
        // Where it was read from on the console; nothing for an SD file.
        const std::optional<console::SaveLocation>& consoleLocation() const { return console_; }

        bool dirty() const { return dirty_; }
        void markDirty() { dirty_ = true; }

        // An SD file gets a copy next to it; the original is never replaced. A console save is
        // replaced, after a backup of it is made on the SD card. `outputPath` is the copy or
        // the backup. Clears the dirty flag on success.
        bool exportChanges(std::string& outputPath, std::string& error);

        // Same, leaving in `message` what to show: where it was saved, or the error.
        bool exportWithMessage(std::string& message);

    private:
        Session(std::unique_ptr<pksm::Sav> save, const GameProfile& profile, std::string path,
            std::optional<console::SaveLocation> console);

        static std::unique_ptr<Session> parse(std::shared_ptr<u8[]> data, size_t size,
            std::string path, std::optional<console::SaveLocation> console, std::string& error);

        std::unique_ptr<pksm::Sav> save_;
        const GameProfile* profile_;
        std::string sourcePath_;
        std::optional<console::SaveLocation> console_;
        bool dirty_ = false;
    };
}
