#include "screens/SaveListScreen.hpp"

#include "SwitchUi.hpp"
#include "app/App.hpp"
#include "app/Session.hpp"
#include "domain/GameProfile.hpp"
#include "domain/SaveCache.hpp"
#include "platform/SaveStorage.hpp"
#include "screens/GameMenuScreen.hpp"

namespace pksm::switchui
{
    SaveListScreen::SaveListScreen()
    {
        std::vector<std::string> files = storage::prepareSaveDirectory(storageError_)
                                             ? storage::listSaveFiles(storageError_)
                                             : std::vector<std::string>{};
        SaveCache cache = SaveCache::parse(storage::readSaveCache());
        files_.reserve(files.size());
        gameNames_.reserve(files.size());
        gameMascots_.reserve(files.size());
        for (const auto& file : files)
        {
            if (const GameProfile* profile = identify(cache, file))
            {
                gameNames_.emplace_back(tr(profile->name));
                gameMascots_.push_back(profile->mascotSpecies);
                files_.push_back(file);
            }
        }
        cache.keepOnly(files);
        if (cache.changed())
        {
            storage::writeSaveCache(cache.serialize());
        }
    }

    const GameProfile* SaveListScreen::identify(SaveCache& cache, const std::string& file)
    {
        const std::string path = storage::savePath(file);
        SaveCache::Stamp stamp;
        const bool stamped = storage::fileStamp(path, stamp.size, stamp.modified);

        std::optional<int> version = stamped ? cache.lookup(file, stamp) : std::nullopt;
        if (!version)
        {
            const auto identified = Session::identify(path);
            version = identified ? static_cast<int>(static_cast<u8>(*identified))
                                 : SaveCache::UNRECOGNIZED;
            if (stamped)
            {
                cache.store(file, stamp, *version);
            }
        }
        if (*version < 0 || *version > UINT8_MAX)
        {
            return nullptr;
        }
        return findGameProfile(pksm::GameVersion{static_cast<u8>(*version)});
    }

    void SaveListScreen::update(App& app, Action action)
    {
        if (action == Action::Quit)
        {
            app.quit();
            return;
        }
        if (showingLoadError_)
        {
            if (action == Action::Confirm || action == Action::Back)
            {
                showingLoadError_ = false;
            }
            return;
        }
        if (files_.empty())
        {
            return;
        }

        if (action == Action::Down)
        {
            selected_ = (selected_ + 1) % files_.size();
        }
        else if (action == Action::Up)
        {
            selected_ = selected_ == 0 ? files_.size() - 1 : selected_ - 1;
        }
        else if (action == Action::Confirm)
        {
            auto session = Session::open(storage::savePath(files_[selected_]), loadError_);
            if (session)
            {
                app.push(std::make_unique<GameMenuScreen>(std::move(session), files_[selected_]));
            }
            else
            {
                showingLoadError_ = true;
            }
        }
    }

    void SaveListScreen::draw(SwitchUi& ui)
    {
        ui.drawFiles(files_, gameNames_, gameMascots_, selected_,
            showingLoadError_ ? loadError_ : storageError_);
    }
}
