#pragma once

#include <ctime>
#include <string>
#include <utility>

namespace pksm::switchui
{
    // What the header shows on the right: the console user, the time and the battery.
    struct HeaderStatus
    {
        std::string user;  // Empty when the console could not say who is playing.
        std::string time;  // "HH:MM".
        int battery = -1;  // Percent, or -1 when unknown.
        bool charging = false;
    };

    // Asks the console for the header's time and battery, at most once a second. The user is
    // the one the app works with, set with setUser().
    class SystemStatus
    {
    public:
        SystemStatus();
        ~SystemStatus();
        SystemStatus(const SystemStatus&)            = delete;
        SystemStatus& operator=(const SystemStatus&) = delete;

        void setUser(std::string name) { status_.user = std::move(name); }
        const HeaderStatus& current();

    private:
        HeaderStatus status_;
        std::time_t updated_ = 0;
        bool power_          = false;
    };
}
