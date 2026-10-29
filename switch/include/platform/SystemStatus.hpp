#pragma once

#include <ctime>
#include <string>

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

    // Asks the console for the header's status. The user is read once; the time and battery
    // are read again at most once a second.
    class SystemStatus
    {
    public:
        SystemStatus();
        ~SystemStatus();
        SystemStatus(const SystemStatus&)            = delete;
        SystemStatus& operator=(const SystemStatus&) = delete;

        const HeaderStatus& current();

    private:
        HeaderStatus status_;
        std::time_t updated_ = 0;
        bool power_          = false;
    };
}
