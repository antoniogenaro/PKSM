#include "screens/EditableField.hpp"

#include "app/App.hpp"
#include "screens/PickerScreens.hpp"
#include <algorithm>
#include <charconv>
#include <memory>
#include <system_error>

namespace pksm::switchui
{
    namespace
    {
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
            if (result.ec != std::errc{} || result.ptr != text.data() + text.size() ||
                parsed > maximum)
            {
                error = "El número está fuera del rango permitido.";
                return false;
            }
            value = parsed;
            return true;
        }

        unsigned int stepNumber(const EditableField& field, unsigned int value, int direction)
        {
            if (field.wrap)
            {
                if (direction > 0)
                {
                    return value >= field.max ? field.min : value + field.step;
                }
                return value <= field.min ? field.max : value - field.step;
            }
            if (direction > 0)
            {
                return value >= field.max || field.max - value < field.step ? field.max
                                                                             : value + field.step;
            }
            return value > field.min && value - field.min >= field.step ? value - field.step
                                                                        : field.min;
        }

        size_t indexOf(const std::vector<unsigned int>& values, unsigned int value)
        {
            const auto it = std::find(values.begin(), values.end(), value);
            return it == values.end() ? 0 : static_cast<size_t>(it - values.begin());
        }

        // Opens a picker over `values` that applies the chosen one through `field.set`.
        void pickValue(const EditableField& field, App& app, std::vector<unsigned int> values,
            std::vector<std::string> options, std::vector<size_t> sprites,
            std::function<void()> onChanged)
        {
            if (values.empty())
            {
                return;
            }
            const size_t current = indexOf(values, field.get());
            PickHandler onPick =
                [values, get = field.get, set = field.set, onChanged](size_t index)
            {
                const unsigned int next = values[index];
                if (next != get())
                {
                    set(next);
                    onChanged();
                }
            };
            if (field.picker == EditableField::ChoicePicker::NatureGrid)
            {
                app.push(std::make_unique<NaturePickerScreen>(
                    std::move(options), current, std::move(onPick)));
            }
            else
            {
                app.push(std::make_unique<OptionPickerScreen>(field.title, std::move(options),
                    current, std::move(sprites), std::move(onPick)));
            }
        }
    }

    EditableField readOnlyField(Field display)
    {
        EditableField field;
        field.display = std::move(display);
        return field;
    }

    EditableField toggleField(Field display, std::function<bool()> get, std::function<void(bool)> set)
    {
        EditableField field;
        field.display = std::move(display);
        field.kind    = EditableField::Kind::Toggle;
        field.getFlag = std::move(get);
        field.setFlag = std::move(set);
        return field;
    }

    EditableField textField(Field display, std::string title, size_t maxLength,
        std::function<std::string()> get, std::function<void(const std::string&)> set)
    {
        EditableField field;
        field.display   = std::move(display);
        field.kind      = EditableField::Kind::Text;
        field.title     = std::move(title);
        field.maxLength = maxLength;
        field.getText   = std::move(get);
        field.setText   = std::move(set);
        return field;
    }

    EditableField numberField(Field display, std::string title, unsigned int min,
        unsigned int max, std::function<unsigned int()> get, std::function<void(unsigned int)> set)
    {
        EditableField field;
        field.display  = std::move(display);
        field.kind     = EditableField::Kind::Number;
        field.title    = std::move(title);
        field.min      = min;
        field.max      = max;
        field.inputMax = max;
        field.get      = std::move(get);
        field.set      = std::move(set);
        return field;
    }

    EditableField choiceField(Field display, std::string title,
        std::function<std::vector<unsigned int>()> choices,
        std::function<std::string(unsigned int)> label, std::function<unsigned int()> get,
        std::function<void(unsigned int)> set)
    {
        EditableField field;
        field.display = std::move(display);
        field.kind    = EditableField::Kind::Choice;
        field.title   = std::move(title);
        field.choices = std::move(choices);
        field.label   = std::move(label);
        field.get     = std::move(get);
        field.set     = std::move(set);
        return field;
    }

    bool stepField(const EditableField& field, int direction)
    {
        switch (field.kind)
        {
            case EditableField::Kind::ReadOnly:
            case EditableField::Kind::Text:
                return false;
            case EditableField::Kind::Toggle:
                field.setFlag(!field.getFlag());
                return true;
            case EditableField::Kind::Number:
            {
                const unsigned int current = field.get();
                const unsigned int next    = stepNumber(field, current, direction);
                if (next == current)
                {
                    return false;
                }
                field.set(next);
                return true;
            }
            case EditableField::Kind::Choice:
            {
                const std::vector<unsigned int> values = field.choices();
                if (values.empty())
                {
                    return false;
                }
                const unsigned int current = field.get();
                const size_t index         = indexOf(values, current);
                const size_t nextIndex     = direction > 0
                                                 ? (index + 1) % values.size()
                                                 : (index == 0 ? values.size() - 1 : index - 1);
                const unsigned int next = values[nextIndex];
                if (next == current)
                {
                    return false;
                }
                field.set(next);
                return true;
            }
        }
        return false;
    }

    void openField(const EditableField& field, App& app, std::string& message,
        std::function<void()> onChanged)
    {
        switch (field.kind)
        {
            case EditableField::Kind::ReadOnly:
                return;
            case EditableField::Kind::Toggle:
                if (stepField(field, 1))
                {
                    onChanged();
                }
                return;
            case EditableField::Kind::Text:
            {
                const std::string current = field.getText();
                std::string result;
                if (app.ui().editText(field.title, current, result, field.maxLength) &&
                    result != current)
                {
                    field.setText(result);
                    onChanged();
                }
                return;
            }
            case EditableField::Kind::Number:
                switch (field.input)
                {
                    case EditableField::NumberInput::None:
                        return;
                    case EditableField::NumberInput::Keyboard:
                    {
                        const unsigned int current = field.get();
                        std::string input;
                        unsigned int value = 0;
                        if (!app.ui().editText(field.title, std::to_string(current), input, 10) ||
                            !parseNumber(input, field.inputMax, value, message))
                        {
                            return;
                        }
                        value = std::clamp(value, field.min, field.max);
                        if (value != current)
                        {
                            field.set(value);
                            onChanged();
                        }
                        return;
                    }
                    case EditableField::NumberInput::List:
                    {
                        std::vector<unsigned int> values;
                        std::vector<std::string> options;
                        values.reserve(static_cast<size_t>(field.max - field.min) + 1);
                        options.reserve(values.capacity());
                        for (unsigned int value = field.min; value <= field.max; ++value)
                        {
                            values.push_back(value);
                            options.push_back(field.label ? field.label(value) : std::to_string(value));
                        }
                        pickValue(field, app, std::move(values), std::move(options), {},
                            std::move(onChanged));
                        return;
                    }
                }
                return;
            case EditableField::Kind::Choice:
            {
                std::vector<unsigned int> values = field.choices();
                std::vector<std::string> options;
                std::vector<size_t> sprites;
                options.reserve(values.size());
                for (const unsigned int value : values)
                {
                    options.push_back(field.label(value));
                    if (field.sprite)
                    {
                        sprites.push_back(field.sprite(value));
                    }
                }
                pickValue(field, app, std::move(values), std::move(options), std::move(sprites),
                    std::move(onChanged));
                return;
            }
        }
    }

    std::vector<Field> displayFields(const std::vector<EditableField>& fields)
    {
        std::vector<Field> display;
        display.reserve(fields.size());
        for (const auto& field : fields)
        {
            display.push_back(field.display);
        }
        return display;
    }
}
