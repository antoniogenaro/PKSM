#include "domain/FieldValues.hpp"

#include "ui/Text.hpp"
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <system_error>

namespace pksm::switchui
{
    unsigned int stepNumber(const StepRule& rule, unsigned int value, int direction)
    {
        if (rule.wrap)
        {
            if (direction > 0)
            {
                return value >= rule.max ? rule.min : value + rule.step;
            }
            return value <= rule.min ? rule.max : value - rule.step;
        }
        if (direction > 0)
        {
            return value >= rule.max || rule.max - value < rule.step ? rule.max
                                                                     : value + rule.step;
        }
        return value > rule.min && value - rule.min >= rule.step ? value - rule.step : rule.min;
    }

    size_t indexOf(const std::vector<unsigned int>& values, unsigned int value)
    {
        const auto it = std::find(values.begin(), values.end(), value);
        return it == values.end() ? 0 : static_cast<size_t>(it - values.begin());
    }

    unsigned int nextChoice(
        const std::vector<unsigned int>& choices, unsigned int current, int direction)
    {
        const size_t index = indexOf(choices, current);
        const size_t next  = direction > 0 ? (index + 1) % choices.size()
                                           : (index == 0 ? choices.size() - 1 : index - 1);
        return choices[next];
    }

    bool parseNumber(
        const std::string& text, unsigned int maximum, unsigned int& value, std::string& error)
    {
        if (text.empty())
        {
            error = tr(Text::EnterNumber);
            return false;
        }
        unsigned int parsed = 0;
        const auto result   = std::from_chars(text.data(), text.data() + text.size(), parsed);
        if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || parsed > maximum)
        {
            error = tr(Text::NumberOutOfRange);
            return false;
        }
        value = parsed;
        return true;
    }

    ShownTrainerIds showTrainerIds(unsigned int tid, unsigned int sid)
    {
        const uint32_t full = (static_cast<uint32_t>(sid & 0xFFFF) << 16) | (tid & 0xFFFF);
        return {full % 1000000, full / 1000000};
    }

    void storeTrainerIds(ShownTrainerIds shown, unsigned int& tid, unsigned int& sid)
    {
        const uint64_t wanted = static_cast<uint64_t>(std::min(shown.sid, MAX_SHOWN_SID)) * 1000000 +
                                std::min(shown.tid, MAX_SHOWN_TID);
        const uint32_t full   = static_cast<uint32_t>(std::min<uint64_t>(wanted, UINT32_MAX));
        tid                   = full & 0xFFFF;
        sid                   = full >> 16;
    }

    std::string padDigits(unsigned int value, size_t digits)
    {
        std::string text = std::to_string(value);
        if (text.size() < digits)
        {
            text.insert(0, digits - text.size(), '0');
        }
        return text;
    }

    std::string groupThousands(unsigned int value, std::string_view separator)
    {
        const std::string digits = std::to_string(value);
        std::string result;
        for (size_t i = 0; i < digits.size(); ++i)
        {
            if (i != 0 && (digits.size() - i) % 3 == 0)
            {
                result += separator;
            }
            result += digits[i];
        }
        return result;
    }

    std::string formatPlayTime(unsigned int minutes)
    {
        const unsigned int hours = minutes / 60;
        const unsigned int rest  = minutes % 60;
        return (hours < 10 ? "0" : "") + std::to_string(hours) + (rest < 10 ? ":0" : ":") +
               std::to_string(rest);
    }

    bool parsePlayTime(
        const std::string& text, unsigned int maximum, unsigned int& minutes, std::string& error)
    {
        if (text.empty())
        {
            error = tr(Text::InvalidPlayTime);
            return false;
        }
        const size_t colon = text.find(':');
        const std::string hoursText =
            colon == std::string::npos ? text : text.substr(0, colon);
        const std::string minutesText =
            colon == std::string::npos ? std::string("0") : text.substr(colon + 1);

        const auto readWhole = [](const std::string& part, unsigned int& out)
        {
            const auto result = std::from_chars(part.data(), part.data() + part.size(), out);
            return !part.empty() && result.ec == std::errc{} &&
                   result.ptr == part.data() + part.size();
        };
        unsigned int hours = 0;
        unsigned int rest  = 0;
        if (!readWhole(hoursText, hours) || !readWhole(minutesText, rest) || rest > 59 ||
            (colon != std::string::npos && minutesText.size() != 2))
        {
            error = tr(Text::InvalidPlayTime);
            return false;
        }
        if (hours > maximum / 60 || hours * 60 + rest > maximum)
        {
            error = tr(Text::NumberOutOfRange);
            return false;
        }
        minutes = hours * 60 + rest;
        return true;
    }
}
