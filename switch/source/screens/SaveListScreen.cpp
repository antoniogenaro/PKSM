#include "screens/SaveListScreen.hpp"

#include "app/App.hpp"
#include "app/SaveLibrary.hpp"
#include "app/Session.hpp"
#include "domain/GameProfile.hpp"
#include "platform/SaveStorage.hpp"
#include "screens/GameMenuScreen.hpp"
#include "ui/Text.hpp"
#include <algorithm>
#include <memory>

namespace pksm::switchui
{
    SaveListScreen::SaveListScreen(size_t game, std::vector<std::string> files)
        : game_(game), files_(std::move(files))
    {
        const GameProfile& profile = gameProfiles()[game_];
        card_   = GameCard{tr(profile.name), profile.mascotSpecies, files_.size(), profile.artColor};
        group_  = tr(profile.group);
    }

    void SaveListScreen::onResume()
    {
        const std::string current = selected_ < files_.size() ? files_[selected_] : std::string{};
        SaveLibrary library       = SaveLibrary::scan();
        storageError_             = std::move(library.error);
        files_                    = std::move(library.filesByGame[game_]);
        card_.saveCount           = files_.size();
        // Keep the same file selected when it is still there.
        const auto found = std::find(files_.begin(), files_.end(), current);
        if (found != files_.end())
        {
            selected_ = static_cast<size_t>(found - files_.begin());
        }
        else if (selected_ >= files_.size())
        {
            selected_ = files_.empty() ? 0 : files_.size() - 1;
        }
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
        if (action == Action::Back)
        {
            app.pop();
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
        ui.drawSaveList(
            card_, group_, files_, selected_, showingLoadError_ ? loadError_ : storageError_);
    }
}
