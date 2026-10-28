#pragma once

#include "utils/DateTime.hpp"
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

// How editor values move with L/R and how typed numbers are read. Pure functions, so they
// can be tested on a PC.
namespace pksm::switchui
{
    struct StepRule
    {
        unsigned int min  = 0;
        unsigned int max  = 0;
        unsigned int step = 1;
        // Past either end, wrap around instead of stopping.
        bool wrap = false;
    };

    // The value after one press of L (direction < 0) or R (direction > 0). Without wrap, a
    // value outside min..max moves to the nearest end.
    unsigned int stepNumber(const StepRule& rule, unsigned int value, int direction);

    // The next of `choices` after `current`, wrapping at both ends. A current value that is
    // not among the choices counts as the first one. `choices` must not be empty.
    unsigned int nextChoice(
        const std::vector<unsigned int>& choices, unsigned int current, int direction);

    // Position of `value` in `values`, or 0 when it is not there.
    size_t indexOf(const std::vector<unsigned int>& values, unsigned int value);

    // Reads a whole decimal number no larger than `maximum`. On failure `error` says why.
    bool parseNumber(
        const std::string& text, unsigned int maximum, unsigned int& value, std::string& error);

    // From generation VII on, games show the two 16-bit trainer IDs as one 32-bit number split
    // in decimal: its last six digits are the trainer ID and the rest the secret ID.
    inline constexpr unsigned int MAX_SHOWN_TID = 999999;
    inline constexpr unsigned int MAX_SHOWN_SID = 4294;

    struct ShownTrainerIds
    {
        unsigned int tid;
        unsigned int sid;
    };

    // The IDs as the game shows them, from the stored `tid` and `sid`.
    ShownTrainerIds showTrainerIds(unsigned int tid, unsigned int sid);

    // The stored IDs for shown ones. `shown` is clamped, so the result always fits 32 bits.
    void storeTrainerIds(ShownTrainerIds shown, unsigned int& tid, unsigned int& sid);

    // `value` with leading zeros up to `digits` digits, such as "006800".
    std::string padDigits(unsigned int value, size_t digits);

    // `value` with `separator` between each group of three digits, such as "4.521.141".
    std::string groupThousands(unsigned int value, std::string_view separator);

    // A play time of `minutes` as "hh:mm", with at least two digits of hours: "258:25", "03:07".
    std::string formatPlayTime(unsigned int minutes);

    // Reads a play time typed as "h:mm" (minutes 0 to 59) or as whole hours, into minutes no
    // larger than `maximum`. On failure `error` says why.
    bool parsePlayTime(
        const std::string& text, unsigned int maximum, unsigned int& minutes, std::string& error);

    // Dates are edited a part at a time, within a range of years: the games store the year as
    // an offset from 2000 in one byte.
    enum class DatePart
    {
        Day,
        Month,
        Year
    };

    struct YearRange
    {
        unsigned int min = 2000;
        unsigned int max = 2099;
    };

    // Days in `month` (1 to 12) of `year`, counting leap years.
    unsigned int daysInMonth(unsigned int year, unsigned int month);

    // `date` with its year in `years`, its month in 1..12 and its day in that month.
    Date clampDate(const Date& date, YearRange years);

    // One `part` of `date` one step forward (direction > 0) or back, wrapping within that
    // part's range; the day then shrinks to fit the month. The result is clamped first.
    Date stepDatePart(const Date& date, DatePart part, int direction, YearRange years);

    // `date` moved by one day forward (direction > 0) or back, across months and years. It
    // stops at the first and last day of `years`. The result is clamped first.
    Date stepDay(const Date& date, int direction, YearRange years);

    // The name of `month` (1 to 12) in the current language.
    std::string monthName(unsigned int month);

    // `date` as the current language writes it, such as "28 de Octubre de 2026".
    std::string formatDate(const Date& date);
}
