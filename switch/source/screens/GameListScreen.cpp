#include "screens/GameListScreen.hpp"

#include "app/App.hpp"
#include "domain/GameProfile.hpp"
#include "screens/SaveListScreen.hpp"
#include "ui/Text.hpp"
#include <memory>

namespace pksm::switchui
{
    GameListScreen::GameListScreen() : grid_(gameProfiles())
    {
        refresh();
        // Start on the first game that has saves, if any.
        for (size_t i = 0; i < games_.size(); ++i)
        {
            if (games_[i].saveCount > 0)
            {
                selected_ = i;
                break;
            }
        }
    }

    void GameListScreen::refresh()
    {
        library_            = SaveLibrary::scan();
        const auto profiles = gameProfiles();
        games_.clear();
        games_.reserve(profiles.size());
        for (size_t i = 0; i < profiles.size(); ++i)
        {
            games_.push_back(GameCard{tr(profiles[i].name), profiles[i].mascotSpecies,
                library_.filesByGame[i].size()});
        }
    }

    void GameListScreen::onResume()
    {
        refresh();
    }

    void GameListScreen::update(App& app, Action action)
    {
        switch (action)
        {
            case Action::Quit:
            case Action::Back:
                app.quit();
                break;
            case Action::Left:
                selected_ = grid_.left(selected_);
                break;
            case Action::Right:
                selected_ = grid_.right(selected_);
                break;
            case Action::Up:
                selected_ = grid_.up(selected_);
                break;
            case Action::Down:
                selected_ = grid_.down(selected_);
                break;
            case Action::Confirm:
                if (selected_ < games_.size())
                {
                    app.push(std::make_unique<SaveListScreen>(
                        selected_, library_.filesByGame[selected_]));
                }
                break;
            default:
                break;
        }
    }

    void GameListScreen::draw(SwitchUi& ui)
    {
        ui.drawGameList(grid_, games_, selected_, library_.error);
    }
}
