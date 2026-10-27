#include "screens/PickerScreens.hpp"

#include "SwitchUi.hpp"
#include "app/App.hpp"
#include <algorithm>

namespace pksm::switchui
{
    OptionPickerScreen::OptionPickerScreen(std::string title, std::vector<std::string> options,
        size_t current, std::vector<size_t> spriteIndices, PickHandler onPick)
        : title_(std::move(title)),
          options_(std::move(options)),
          spriteIndices_(std::move(spriteIndices)),
          index_(options_.empty() ? 0 : std::min(current, options_.size() - 1)),
          onPick_(std::move(onPick))
    {
    }

    void OptionPickerScreen::update(App& app, Action action)
    {
        if (options_.empty())
        {
            app.pop();
            return;
        }
        const size_t last = options_.size() - 1;
        switch (action)
        {
            case Action::Confirm:
                app.pop();
                onPick_(index_);
                break;
            case Action::Back:
            case Action::Quit:
                app.pop();
                break;
            case Action::Up:
                index_ = index_ > 0 ? index_ - 1 : index_;
                break;
            case Action::Down:
                index_ = std::min(last, index_ + 1);
                break;
            case Action::Left:
                index_ = index_ > 10 ? index_ - 10 : 0;
                break;
            case Action::Right:
                index_ = std::min(last, index_ + 10);
                break;
            case Action::PreviousPage:
                index_ = index_ > 100 ? index_ - 100 : 0;
                break;
            case Action::NextPage:
                index_ = std::min(last, index_ + 100);
                break;
            default:
                break;
        }
    }

    void OptionPickerScreen::draw(SwitchUi& ui)
    {
        const size_t lastStart =
            options_.size() > VISIBLE_ROWS ? options_.size() - VISIBLE_ROWS : 0;
        const size_t first =
            std::min(index_ > VISIBLE_ROWS / 2 ? index_ - VISIBLE_ROWS / 2 : 0, lastStart);
        ui.drawOptionPicker(title_, options_, spriteIndices_, first, VISIBLE_ROWS, index_);
    }

    NaturePickerScreen::NaturePickerScreen(
        std::vector<std::string> natures, size_t current, PickHandler onPick)
        : natures_(std::move(natures)), onPick_(std::move(onPick))
    {
        if (current < 25)
        {
            row_    = current / 5;
            column_ = current % 5;
        }
    }

    void NaturePickerScreen::update(App& app, Action action)
    {
        if (natures_.size() != 25)
        {
            app.pop();
            return;
        }
        switch (action)
        {
            case Action::Confirm:
                app.pop();
                onPick_(row_ * 5 + column_);
                break;
            case Action::Back:
            case Action::Quit:
                app.pop();
                break;
            case Action::Up:
                row_ = row_ > 0 ? row_ - 1 : row_;
                break;
            case Action::Down:
                row_ = row_ < 4 ? row_ + 1 : row_;
                break;
            case Action::Left:
                column_ = column_ > 0 ? column_ - 1 : column_;
                break;
            case Action::Right:
                column_ = column_ < 4 ? column_ + 1 : column_;
                break;
            default:
                break;
        }
    }

    void NaturePickerScreen::draw(SwitchUi& ui)
    {
        ui.drawNaturePicker(natures_, row_, column_);
    }
}
