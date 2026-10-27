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
            stack_.back()->draw(ui_);
            SDL_Delay(16);
        }
    }
}
