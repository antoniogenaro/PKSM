#include "screens/SaveListScreen.hpp"

#include "app/App.hpp"
#include "app/SaveLibrary.hpp"
#include "app/Session.hpp"
#include "domain/GameProfile.hpp"
#include "platform/ConsoleSaves.hpp"
#include "platform/SaveStorage.hpp"
#include "screens/GameMenuScreen.hpp"
#include "ui/Text.hpp"
#include <algorithm>
#include <memory>

namespace pksm::switchui
{
    SaveListScreen::SaveListScreen(size_t game, std::vector<std::string> files,
        std::optional<ConsoleUser> user, bool consoleSave)
        : game_(game), user_(std::move(user)), files_(std::move(files))
    {
        const GameProfile& profile = gameProfiles()[game_];
        card_  = GameCard{tr(profile.name), profile.mascotSpecies, files_.size(), profile.artColor,
             consoleSave && user_.has_value()};
        group_ = tr(profile.group);
    }

    size_t SaveListScreen::rowCount() const
    {
        return files_.size() + (card_.consoleSave ? 1 : 0);
    }

    void SaveListScreen::onResume()
    {
        const size_t firstFile    = card_.consoleSave ? 1 : 0;
        const bool onConsole      = card_.consoleSave && selected_ == 0;
        const std::string current = !onConsole && selected_ - firstFile < files_.size()
                                        ? files_[selected_ - firstFile]
                                        : std::string{};
        SaveLibrary library = SaveLibrary::scan(user_ ? &*user_ : nullptr);
        storageError_       = std::move(library.error);
        files_              = std::move(library.filesByGame[game_]);
        card_.saveCount     = files_.size();
        card_.consoleSave   = library.consoleSaveByGame[game_] && user_.has_value();

        // Keep the same save selected when it is still there.
        const size_t first = card_.consoleSave ? 1 : 0;
        const auto found   = std::find(files_.begin(), files_.end(), current);
        if (onConsole && card_.consoleSave)
        {
            selected_ = 0;
        }
        else if (!current.empty() && found != files_.end())
        {
            selected_ = first + static_cast<size_t>(found - files_.begin());
        }
        else if (selected_ >= rowCount())
        {
            selected_ = rowCount() == 0 ? 0 : rowCount() - 1;
        }
    }

    void SaveListScreen::open(App& app)
    {
        const GameProfile& profile = gameProfiles()[game_];
        if (card_.consoleSave && selected_ == 0)
        {
            const console::SaveLocation location{
                profile.consoleTitleId, profile.consoleSaveFile, *user_, profile.backupFolder};
            if (auto session = Session::openConsole(location, loadError_))
            {
                app.push(std::make_unique<GameMenuScreen>(std::move(session), std::string{}));
            }
            else
            {
                showingLoadError_ = true;
            }
            return;
        }

        const std::string& file = files_[selected_ - (card_.consoleSave ? 1 : 0)];
        if (auto session = Session::open(storage::savePath(file), loadError_))
        {
            app.push(std::make_unique<GameMenuScreen>(std::move(session), file));
        }
        else
        {
            showingLoadError_ = true;
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
        const size_t rows = rowCount();
        if (rows == 0)
        {
            return;
        }

        if (action == Action::Down)
        {
            selected_ = (selected_ + 1) % rows;
        }
        else if (action == Action::Up)
        {
            selected_ = selected_ == 0 ? rows - 1 : selected_ - 1;
        }
        else if (action == Action::Confirm && selected_ < rows)
        {
            open(app);
        }
    }

    void SaveListScreen::draw(SwitchUi& ui)
    {
        ui.drawSaveList(card_, group_, user_ ? user_->name : std::string{}, files_, selected_,
            showingLoadError_ ? loadError_ : storageError_);
    }
}
