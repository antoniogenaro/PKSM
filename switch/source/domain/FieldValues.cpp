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

    unsigned int daysInMonth(unsigned int year, unsigned int month)
    {
        constexpr unsigned int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (month < 1 || month > 12)
        {
            return 31;
        }
        const bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
        return month == 2 && leap ? 29 : days[month - 1];
    }

    Date clampDate(const Date& date, YearRange years)
    {
        const unsigned int year  = std::clamp<unsigned int>(date.year(), years.min, years.max);
        const unsigned int month = std::clamp<unsigned int>(date.month(), 1, 12);
        const unsigned int day   = std::clamp<unsigned int>(date.day(), 1, daysInMonth(year, month));
        return Date{static_cast<u8>(day), static_cast<u8>(month), year};
    }

    Date stepDatePart(const Date& date, DatePart part, int direction, YearRange years)
    {
        Date next = clampDate(date, years);
        switch (part)
        {
            case DatePart::Day:
                next.day(static_cast<u8>(stepNumber(
                    {1, daysInMonth(next.year(), next.month()), 1, true}, next.day(), direction)));
                return next;
            case DatePart::Month:
                next.month(static_cast<u8>(stepNumber({1, 12, 1, true}, next.month(), direction)));
                break;
            case DatePart::Year:
                next.year(stepNumber({years.min, years.max, 1, true}, next.year(), direction));
                break;
        }
        return clampDate(next, years);
    }

    Date stepDay(const Date& date, int direction, YearRange years)
    {
        Date next = clampDate(date, years);
        if (direction > 0)
        {
            if (next.day() < daysInMonth(next.year(), next.month()))
            {
                next.day(next.day() + 1);
            }
            else if (next.month() < 12)
            {
                next = Date{1, static_cast<u8>(next.month() + 1), next.year()};
            }
            else if (next.year() < years.max)
            {
                next = Date{1, 1, next.year() + 1};
            }
        }
        else if (direction < 0)
        {
            if (next.day() > 1)
            {
                next.day(next.day() - 1);
            }
            else if (next.month() > 1)
            {
                const u8 month = static_cast<u8>(next.month() - 1);
                next = Date{static_cast<u8>(daysInMonth(next.year(), month)), month, next.year()};
            }
            else if (next.year() > years.min)
            {
                next = Date{31, 12, next.year() - 1};
            }
        }
        return next;
    }

    std::string monthName(unsigned int month)
    {
        constexpr Text names[] = {Text::January, Text::February, Text::March, Text::April,
            Text::May, Text::June, Text::July, Text::August, Text::September, Text::October,
            Text::November, Text::December};
        return month >= 1 && month <= 12 ? tr(names[month - 1]) : std::to_string(month);
    }

    std::string formatDate(const Date& date)
    {
        return tr(Text::DateValue, {std::to_string(date.day()), monthName(date.month()),
                                       std::to_string(date.year())});
    }
}
