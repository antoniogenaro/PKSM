#include "domain/FieldValues.hpp"

#include <algorithm>
#include <charconv>
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
            error = "Introduce un número.";
            return false;
        }
        unsigned int parsed = 0;
        const auto result   = std::from_chars(text.data(), text.data() + text.size(), parsed);
        if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || parsed > maximum)
        {
            error = "El número está fuera del rango permitido.";
            return false;
        }
        value = parsed;
        return true;
    }
}
