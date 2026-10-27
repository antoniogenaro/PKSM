#pragma once

namespace pksm::switchui
{
    class App;
    class SwitchUi;
    enum class Action;

    // One screen of the app. Only the top screen of the App's stack gets input and is drawn.
    class Screen
    {
    public:
        virtual ~Screen() = default;

        // Reacts to one input action. Stack changes requested through `app` take effect after
        // this call returns, so a screen may close itself here.
        virtual void update(App& app, Action action) = 0;
        virtual void draw(SwitchUi& ui) = 0;

        // Called when the screens above this one have closed.
        virtual void onResume() {}

        // The screen that owns the open save. App::closeSession() returns below it.
        virtual bool ownsSession() const { return false; }

        // A dialog drawn over the screen below it, which stays visible but gets no input.
        virtual bool isOverlay() const { return false; }
    };
}
