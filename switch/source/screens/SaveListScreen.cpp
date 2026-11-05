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
        std::optional<ConsoleUser> user, std::vector<ConsoleTitle> consoleTitles)
        : game_(game), user_(std::move(user)), consoleTitles_(std::move(consoleTitles)),
          files_(std::move(files))
    {
        const GameProfile& profile = gameProfiles()[game_];
        card_  = GameCard{tr(profile.name), profile.mascotSpecies, files_.size(), profile.artColor,
             consoleCount() > 0};
        group_ = tr(profile.group);
    }

    size_t SaveListScreen::consoleCount() const
    {
        return user_ ? consoleTitles_.size() : 0;
    }

    size_t SaveListScreen::rowCount() const
    {
        return consoleCount() + files_.size();
    }

    void SaveListScreen::onResume()
    {
        const size_t firstFile = consoleCount();
        const uint64_t currentTitle =
            selected_ < firstFile ? consoleTitles_[selected_].id : uint64_t{0};
        const std::string current = selected_ >= firstFile && selected_ - firstFile < files_.size()
                                        ? files_[selected_ - firstFile]
                                        : std::string{};
        SaveLibrary library = SaveLibrary::scan(user_ ? &*user_ : nullptr);
        storageError_       = std::move(library.error);
        files_              = std::move(library.filesByGame[game_]);
        card_.saveCount     = files_.size();
        consoleTitles_      = std::move(library.consoleTitlesByGame[game_]);
        card_.consoleSave   = consoleCount() > 0;

        // Keep the same save selected when it is still there.
        const size_t first    = consoleCount();
        const auto foundTitle = std::find_if(consoleTitles_.begin(), consoleTitles_.end(),
            [currentTitle](const ConsoleTitle& title) { return title.id == currentTitle; });
        const auto found      = std::find(files_.begin(), files_.end(), current);
        if (currentTitle != 0 && foundTitle != consoleTitles_.end())
        {
            selected_ = static_cast<size_t>(foundTitle - consoleTitles_.begin());
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
        if (selected_ < consoleCount())
        {
            // Each language gets a backup folder of its own, such as "FireRed ES".
            const ConsoleTitle& title = consoleTitles_[selected_];
            std::string backupFolder  = profile.backupFolder;
            if (title.language[0] != '\0')
            {
                backupFolder += std::string(" ") + title.language;
            }
            const console::SaveLocation location{
                title.id, profile.consoleSaveFile, *user_, std::move(backupFolder)};
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

        const std::string& file = files_[selected_ - consoleCount()];
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
        std::vector<std::string> consoleLabels;
        for (size_t i = 0; i < consoleCount(); ++i)
        {
            consoleLabels.emplace_back(consoleTitles_[i].language);
        }
        ui.drawSaveList(card_, group_, user_ ? user_->name : std::string{}, consoleLabels, files_,
            selected_, showingLoadError_ ? loadError_ : storageError_);
    }
}
