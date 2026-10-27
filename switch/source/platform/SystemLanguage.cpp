#include "platform/SystemLanguage.hpp"

#include <switch.h>

namespace pksm::switchui
{
    TextLanguage systemTextLanguage()
    {
        if (R_FAILED(setInitialize()))
        {
            return TextLanguage::English;
        }
        u64 code              = 0;
        SetLanguage language  = SetLanguage_ENUS;
        const bool known      = R_SUCCEEDED(setGetSystemLanguage(&code)) &&
                                R_SUCCEEDED(setMakeLanguage(code, &language));
        setExit();
        if (known && (language == SetLanguage_ES || language == SetLanguage_ES419))
        {
            return TextLanguage::Spanish;
        }
        return TextLanguage::English;
    }
}
