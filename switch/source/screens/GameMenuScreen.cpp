#include "screens/GameMenuScreen.hpp"

#include "SwitchUi.hpp"
#include "app/App.hpp"
#include "app/Session.hpp"
#include "domain/EventLibrary.hpp"
#include "domain/GameProfile.hpp"
#include "screens/BankScreen.hpp"
#include "sav/Sav.hpp"
#include "screens/BagScreen.hpp"
#include "screens/BoxBrowserScreen.hpp"
#include "screens/EventsScreen.hpp"
#include "screens/SaveSummary.hpp"
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
        constexpr size_t BANK_ITEM           = 1;
        constexpr size_t BAG_ITEM            = 2;
        constexpr size_t EVENTS_ITEM         = 3;
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
                    session_->saveWithMessage(message_);
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
        else if (selected_ == BANK_ITEM)
        {
            if (session_->save().maxBoxes() == 0)
            {
                message_ = tr(Text::NoBoxes);
                return;
            }
            std::string error;
            PokemonBank* bank = session_->bank(error);
            if (!bank)
            {
                message_ = error;
                return;
            }
            app.push(std::make_unique<BankScreen>(*session_, *bank));
        }
        else if (selected_ == BAG_ITEM)
        {
            if (session_->save().pouches().empty())
            {
                message_ = tr(Text::OptionUnavailable);
                return;
            }
            app.push(std::make_unique<BagScreen>(*session_, path_));
        }
        else if (selected_ == EVENTS_ITEM)
        {
            // Only Sword and Shield cards are packed.
            if (session_->save().generation() != pksm::Generation::EIGHT ||
                session_->save().maxBoxes() == 0)
            {
                message_ = tr(Text::OptionUnavailable);
                return;
            }
            std::string error;
            auto library = EventLibrary::load(error);
            if (!library)
            {
                message_ = error;
                return;
            }
            app.push(std::make_unique<EventsScreen>(*session_, std::move(library)));
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
        ui.drawGameMenu(
            summarizeSave(*session_, path_), selected_, sessionMessage(*session_, message_));
    }

    void GameMenuScreen::onResume()
    {
        message_.clear();
    }
}
