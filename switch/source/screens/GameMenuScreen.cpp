#include "screens/GameMenuScreen.hpp"

#include "SwitchUi.hpp"
#include "app/App.hpp"
#include "app/Session.hpp"
#include "domain/GameProfile.hpp"
#include "platform/SaveStorage.hpp"
#include "sav/Sav.hpp"
#include "screens/BoxBrowserScreen.hpp"
#include "screens/TrainerEditorScreen.hpp"
#include "utils/i18n.hpp"
#include "ui/Text.hpp"

namespace pksm::switchui
{
    namespace
    {
        // The menu is a list; the order matches SwitchUi::drawGameMenu.
        constexpr size_t MENU_OPTIONS        = 6;
        constexpr size_t POKEMON_EDITOR_ITEM = 0;
        constexpr size_t TRAINER_EDITOR_ITEM = 4;
    }

    GameMenuScreen::GameMenuScreen(std::unique_ptr<Session> session, std::string path)
        : session_(std::move(session)), path_(std::move(path))
    {
        i18n::init(session_->save().language());
    }

    GameMenuScreen::~GameMenuScreen() = default;

    void GameMenuScreen::update(App& app, Action action)
    {
        switch (action)
        {
            case Action::Quit:
                if (session_->dirty())
                {
                    message_ = tr(Text::UnsavedBeforeQuit);
                }
                else
                {
                    app.quit();
                }
                break;
            case Action::Back:
                if (session_->dirty())
                {
                    message_ = tr(Text::UnsavedBeforeBack);
                }
                else
                {
                    app.pop();
                }
                break;
            case Action::Discard:
                app.pop();
                break;
            case Action::Save:
                if (session_->dirty())
                {
                    session_->exportWithMessage(message_);
                }
                break;
            case Action::Up:
                selected_ = selected_ == 0 ? MENU_OPTIONS - 1 : selected_ - 1;
                message_.clear();
                break;
            case Action::Down:
                selected_ = (selected_ + 1) % MENU_OPTIONS;
                message_.clear();
                break;
            case Action::Confirm:
                open(app);
                break;
            default:
                break;
        }
    }

    void GameMenuScreen::open(App& app)
    {
        const char* game = tr(session_->profile().name);
        if (selected_ == POKEMON_EDITOR_ITEM)
        {
            if (session_->save().maxBoxes() == 0)
            {
                message_ = tr(Text::NoBoxes);
                return;
            }
            app.push(std::make_unique<BoxBrowserScreen>(*session_, game, path_));
        }
        else if (selected_ == TRAINER_EDITOR_ITEM)
        {
            app.push(std::make_unique<TrainerEditorScreen>(*session_, game, path_));
        }
        else
        {
            message_ = tr(Text::OptionUnavailable);
        }
    }

    void GameMenuScreen::draw(SwitchUi& ui)
    {
        pksm::Sav& save            = session_->save();
        const GameProfile& profile = session_->profile();
        const std::string details  = tr(Text::TrainerSummary, {save.otName(),
             std::to_string(save.displayTID()), std::to_string(save.displaySID())});
        const size_t slash         = path_.find_last_of('/');
        const std::string file =
            slash == std::string::npos ? path_ : path_.substr(slash + 1);
        ui.drawGameMenu(GameCard{tr(profile.name), profile.mascotSpecies, 0, profile.artColor},
            tr(profile.group), file, details, std::string(storage::SAVE_DIRECTORY) + "/" + path_,
            selected_, message_);
    }

    void GameMenuScreen::onResume()
    {
        message_.clear();
    }
}
