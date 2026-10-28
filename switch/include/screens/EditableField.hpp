#pragma once

#include "SwitchUi.hpp"
#include "domain/FieldValues.hpp"
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace pksm::switchui
{
    class App;

    // One editable row of an editor: what it shows and how L/R and A change it. Editors build
    // their rows with the helpers below and hand them to stepField() and openField().
    struct EditableField
    {
        enum class Kind
        {
            ReadOnly,
            // Any of L/R/A flips it.
            Toggle,
            // A opens the system keyboard.
            Text,
            // L/R step between min and max; A opens `input`.
            Number,
            // L/R cycle through choices(); A opens a picker.
            Choice,
            // L/R move one day; A opens the date picker.
            Date
        };

        enum class NumberInput
        {
            None,
            Keyboard,
            // A picker listing every value from min to max, named by `label` when set.
            List,
            // The keyboard, for a play time in minutes typed as "hh:mm".
            PlayTime
        };

        enum class ChoicePicker
        {
            List,
            NatureGrid
        };

        Field display;
        Kind kind = Kind::ReadOnly;
        // Header of the keyboard or picker.
        std::string title;

        std::function<bool()> getFlag;
        std::function<void(bool)> setFlag;

        std::function<std::string()> getText;
        std::function<void(const std::string&)> setText;
        size_t maxLength = 0;

        // Number and Choice values.
        std::function<unsigned int()> get;
        std::function<void(unsigned int)> set;
        unsigned int min  = 0;
        unsigned int max  = 0;
        unsigned int step = 1;
        // Past either end, wrap around instead of stopping.
        bool wrap               = false;
        NumberInput input       = NumberInput::Keyboard;
        // Largest number the keyboard accepts; the result is then clamped to min..max.
        unsigned int inputMax   = 0;
        std::function<std::string(unsigned int)> label;

        std::function<std::vector<unsigned int>()> choices;
        // Poké Ball sprite for each choice, when the picker shows them.
        std::function<size_t(unsigned int)> sprite;
        ChoicePicker picker = ChoicePicker::List;

        std::function<::Date()> getDate;
        std::function<void(const ::Date&)> setDate;
        YearRange years;
    };

    EditableField readOnlyField(Field display);
    EditableField toggleField(Field display, std::function<bool()> get, std::function<void(bool)> set);
    EditableField textField(Field display, std::string title, size_t maxLength,
        std::function<std::string()> get, std::function<void(const std::string&)> set);
    // A number edited with the keyboard. Set `input`, `step`, `wrap` or `inputMax` afterwards
    // for anything else.
    EditableField numberField(Field display, std::string title, unsigned int min,
        unsigned int max, std::function<unsigned int()> get, std::function<void(unsigned int)> set);
    EditableField choiceField(Field display, std::string title,
        std::function<std::vector<unsigned int>()> choices,
        std::function<std::string(unsigned int)> label, std::function<unsigned int()> get,
        std::function<void(unsigned int)> set);
    // A whole date, shown with formatDate() and picked with the date picker.
    EditableField dateField(std::string label, std::string title, YearRange years,
        std::function<::Date()> get, std::function<void(const ::Date&)> set);

    // Applies L (-1) or R (1). Returns whether the value changed.
    bool stepField(const EditableField& field, int direction);

    // Applies A. The keyboard answers at once; a picker answers later, from its own screen.
    // `onChanged` runs whenever the value really changed. `message` gets input errors, and
    // must outlive any picker this opens.
    void openField(const EditableField& field, App& app, std::string& message,
        std::function<void()> onChanged);

    std::vector<Field> displayFields(const std::vector<EditableField>& fields);
}
