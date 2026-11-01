#include "screens/GameListScreen.hpp"

#include "app/App.hpp"
#include "domain/GameProfile.hpp"
#include "screens/PickerScreens.hpp"
#include "screens/PokedexScreen.hpp"
#include "screens/SaveListScreen.hpp"
#include "ui/Text.hpp"
#include <memory>
#include <optional>

namespace pksm::switchui
{
    GameListScreen::GameListScreen(std::vector<ConsoleUser> users, size_t user)
        : grid_(gameProfiles()), users_(std::move(users)), user_(user < users_.size() ? user : 0)
    {
        refresh();
        // Start on the first game that has saves, if any.
        for (size_t i = 0; i < games_.size(); ++i)
        {
            if (games_[i].consoleSave || games_[i].saveCount > 0)
            {
                selected_ = i;
                break;
            }
        }
    }

    const ConsoleUser* GameListScreen::user() const
    {
        return user_ < users_.size() ? &users_[user_] : nullptr;
    }

    void GameListScreen::refresh()
    {
        library_            = SaveLibrary::scan(user());
        const auto profiles = gameProfiles();
        games_.clear();
        games_.reserve(profiles.size());
        for (size_t i = 0; i < profiles.size(); ++i)
        {
            games_.push_back(GameCard{tr(profiles[i].name), profiles[i].mascotSpecies,
                library_.filesByGame[i].size(), profiles[i].artColor,
                library_.consoleSaveByGame[i]});
        }
    }

    void GameListScreen::onResume()
    {
        refresh();
    }

    void GameListScreen::update(App& app, Action action)
    {
        if (pokedexRequested_)
        {
            pokedexRequested_ = false;
            action            = Action::Alternate;
        }
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
            case Action::Discard: // X
                if (!users_.empty())
                {
                    std::vector<std::string> names;
                    names.reserve(users_.size());
                    for (const auto& user : users_)
                    {
                        names.push_back(user.name);
                    }
                    // The games are counted again in onResume(), once the picker has closed.
                    app.push(std::make_unique<UserPickerScreen>(std::move(names), user_,
                        [this, &app](size_t index)
                        {
                            user_ = index;
                            app.ui().setUserName(users_[user_].name);
                        }));
                }
                break;
            case Action::Alternate: // −
                app.push(std::make_unique<PokedexScreen>());
                break;
            case Action::Confirm:
                if (selected_ < games_.size())
                {
                    std::optional<ConsoleUser> owner;
                    if (user())
                    {
                        owner = *user();
                    }
                    app.push(std::make_unique<SaveListScreen>(selected_,
                        library_.filesByGame[selected_], std::move(owner),
                        games_[selected_].consoleSave));
                }
                break;
            default:
                break;
        }
    }

    void GameListScreen::draw(SwitchUi& ui)
    {
        if (ui.drawGameList(grid_, games_, selected_, library_.error, !users_.empty()))
        {
            pokedexRequested_ = true;
        }
    }
}
