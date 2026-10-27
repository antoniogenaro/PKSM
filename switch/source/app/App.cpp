#include "app/App.hpp"

#include "SwitchUi.hpp"
#include <algorithm>
#include <switch.h>

namespace pksm::switchui
{
    void App::push(std::unique_ptr<Screen> screen)
    {
        pendingPushes_.push_back(std::move(screen));
    }

    void App::pop()
    {
        ++pendingPops_;
    }

    void App::closeSession()
    {
        for (size_t i = stack_.size(); i > 0; --i)
        {
            if (stack_[i - 1]->ownsSession())
            {
                pendingPops_ = std::max(pendingPops_, stack_.size() - (i - 1));
                return;
            }
        }
    }

    void App::quit()
    {
        quitRequested_ = true;
    }

    void App::applyPending()
    {
        const bool popped = pendingPops_ > 0;
        for (; pendingPops_ > 0 && !stack_.empty(); --pendingPops_)
        {
            stack_.pop_back();
        }
        pendingPops_ = 0;

        const bool pushed = !pendingPushes_.empty();
        for (auto& screen : pendingPushes_)
        {
            stack_.push_back(std::move(screen));
        }
        pendingPushes_.clear();

        if (popped && !pushed && !stack_.empty())
        {
            stack_.back()->onResume();
        }
    }

    void App::run()
    {
        applyPending();
        while (appletMainLoop() && !quitRequested_ && !stack_.empty())
        {
            const Action action = ui_.pollAction();
            stack_.back()->update(*this, action);
            applyPending();
            if (quitRequested_ || stack_.empty())
            {
                break;
            }
            // The top screen, and when it is an overlay the screens below it down to the first
            // that is not, drawn from the bottom up.
            size_t first = stack_.size() - 1;
            while (first > 0 && stack_[first]->isOverlay())
            {
                --first;
            }
            ui_.beginScreens();
            for (size_t i = first; i < stack_.size(); ++i)
            {
                stack_[i]->draw(ui_);
            }
            ui_.endScreens();
            SDL_Delay(16);
        }
    }
}
