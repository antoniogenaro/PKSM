#pragma once

#include <memory>
#include <string>

namespace pksm
{
    class Sav;
}

namespace pksm::switchui
{
    struct GameProfile;

    // An open save: the parsed data, where it came from, its game and whether it has
    // changes that were not exported yet.
    class Session
    {
    public:
        // Reads and parses the file. Returns nullptr, with a message in `error`, when it
        // cannot be read or is not a supported game.
        static std::unique_ptr<Session> open(const std::string& path, std::string& error);

        ~Session();
        Session(const Session&)            = delete;
        Session& operator=(const Session&) = delete;

        pksm::Sav& save() { return *save_; }
        const GameProfile& profile() const { return *profile_; }
        const std::string& sourcePath() const { return sourcePath_; }

        bool dirty() const { return dirty_; }
        void markDirty() { dirty_ = true; }

        // Writes a copy next to the source file; the original is never replaced. Clears
        // the dirty flag on success.
        bool exportChanges(std::string& outputPath, std::string& error);

    private:
        Session(std::unique_ptr<pksm::Sav> save, const GameProfile& profile, std::string path);

        std::unique_ptr<pksm::Sav> save_;
        const GameProfile* profile_;
        std::string sourcePath_;
        bool dirty_ = false;
    };
}
