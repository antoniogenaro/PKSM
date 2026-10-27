#pragma once

#include <cstddef>
#include <string>
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
}
