#include "platform/SystemStatus.hpp"

#include <switch.h>
#include <cstdio>
#include <cstring>

namespace pksm::switchui
{
    namespace
    {
        // The nickname of the user the app was started for, or of the last one who played.
        std::string consoleUserName()
        {
            if (R_FAILED(accountInitialize(AccountServiceType_Application)) &&
                R_FAILED(accountInitialize(AccountServiceType_System)))
            {
                return {};
            }
            std::string name;
            AccountUid uid{};
            bool found = R_SUCCEEDED(accountGetPreselectedUser(&uid)) && accountUidIsValid(&uid);
            if (!found)
            {
                found = R_SUCCEEDED(accountGetLastOpenedUser(&uid)) && accountUidIsValid(&uid);
            }
            AccountProfile profile;
            if (found && R_SUCCEEDED(accountGetProfile(&profile, uid)))
            {
                AccountProfileBase base{};
                if (R_SUCCEEDED(accountProfileGet(&profile, nullptr, &base)))
                {
                    name.assign(base.nickname, strnlen(base.nickname, sizeof(base.nickname)));
                }
                accountProfileClose(&profile);
            }
            accountExit();
            return name;
        }
    }

    SystemStatus::SystemStatus()
    {
        status_.user = consoleUserName();
        power_       = R_SUCCEEDED(psmInitialize());
    }

    SystemStatus::~SystemStatus()
    {
        if (power_)
        {
            psmExit();
        }
    }

    const HeaderStatus& SystemStatus::current()
    {
        const std::time_t now = std::time(nullptr);
        if (now == updated_)
        {
            return status_;
        }
        updated_ = now;

        char time[8] = "--:--";
        if (const std::tm* local = std::localtime(&now))
        {
            std::snprintf(time, sizeof(time), "%02d:%02d", local->tm_hour, local->tm_min);
        }
        status_.time = time;

        u32 percent         = 0;
        PsmChargerType type = PsmChargerType_Unconnected;
        status_.battery =
            power_ && R_SUCCEEDED(psmGetBatteryChargePercentage(&percent)) ? static_cast<int>(percent)
                                                                            : -1;
        status_.charging = power_ && R_SUCCEEDED(psmGetChargerType(&type)) &&
                           type != PsmChargerType_Unconnected;
        return status_;
    }
}
