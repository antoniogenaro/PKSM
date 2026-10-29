#include "platform/ConsoleUsers.hpp"

#include <cstring>

namespace pksm::switchui
{
    namespace
    {
        // Opens the account service for the length of a call.
        class AccountService
        {
        public:
            AccountService()
                : open_(R_SUCCEEDED(accountInitialize(AccountServiceType_Application)) ||
                        R_SUCCEEDED(accountInitialize(AccountServiceType_System)))
            {
            }
            ~AccountService()
            {
                if (open_)
                {
                    accountExit();
                }
            }
            AccountService(const AccountService&)            = delete;
            AccountService& operator=(const AccountService&) = delete;

            bool open() const { return open_; }

        private:
            bool open_;
        };

        std::string nickname(AccountUid uid)
        {
            std::string name;
            AccountProfile profile;
            if (R_SUCCEEDED(accountGetProfile(&profile, uid)))
            {
                AccountProfileBase base{};
                if (R_SUCCEEDED(accountProfileGet(&profile, nullptr, &base)))
                {
                    name.assign(base.nickname, strnlen(base.nickname, sizeof(base.nickname)));
                }
                accountProfileClose(&profile);
            }
            return name;
        }

        bool sameUser(const AccountUid& a, const AccountUid& b)
        {
            return a.uid[0] == b.uid[0] && a.uid[1] == b.uid[1];
        }
    }

    std::vector<ConsoleUser> listConsoleUsers()
    {
        std::vector<ConsoleUser> users;
        const AccountService service;
        if (!service.open())
        {
            return users;
        }
        AccountUid uids[ACC_USER_LIST_SIZE]{};
        s32 count = 0;
        if (R_FAILED(accountListAllUsers(uids, ACC_USER_LIST_SIZE, &count)))
        {
            return users;
        }
        for (s32 i = 0; i < count && i < ACC_USER_LIST_SIZE; ++i)
        {
            if (accountUidIsValid(&uids[i]))
            {
                users.push_back(ConsoleUser{uids[i], nickname(uids[i])});
            }
        }
        return users;
    }

    size_t startingUserIndex(const std::vector<ConsoleUser>& users)
    {
        const AccountService service;
        if (!service.open())
        {
            return 0;
        }
        AccountUid uid{};
        bool found = R_SUCCEEDED(accountGetPreselectedUser(&uid)) && accountUidIsValid(&uid);
        if (!found)
        {
            found = R_SUCCEEDED(accountGetLastOpenedUser(&uid)) && accountUidIsValid(&uid);
        }
        for (size_t i = 0; found && i < users.size(); ++i)
        {
            if (sameUser(users[i].uid, uid))
            {
                return i;
            }
        }
        return 0;
    }
}
