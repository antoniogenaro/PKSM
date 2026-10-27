#include "screens/SaveListScreen.hpp"

#include "SwitchUi.hpp"
#include "app/App.hpp"
#include "app/Session.hpp"
#include "domain/GameProfile.hpp"
#include "platform/SaveStorage.hpp"
#include "screens/GameMenuScreen.hpp"

namespace pksm::switchui
{
    SaveListScreen::SaveListScreen()
    {
        std::vector<std::string> files = storage::prepareSaveDirectory(storageError_)
                                             ? storage::listSaveFiles(storageError_)
                                             : std::vector<std::string>{};
        files_.reserve(files.size());
        gameNames_.reserve(files.size());
        gameMascots_.reserve(files.size());
        for (auto& file : files)
        {
            std::string identifyError;
            if (const auto identified = Session::open(storage::savePath(file), identifyError))
            {
                gameNames_.emplace_back(identified->profile().name);
                gameMascots_.push_back(identified->profile().mascotSpecies);
                files_.push_back(std::move(file));
            }
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
