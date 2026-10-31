#include "screens/PickerScreens.hpp"

#include "SwitchUi.hpp"
#include "app/App.hpp"
#include "ui/Text.hpp"
#include <algorithm>
#include <iterator>

namespace pksm::switchui
{
    namespace
    {
        // The order each kind of list had when last closed, for the next time it opens.
        PickerOrder rememberedOrders[] = {
            PickerOrder::Number, // Other: not remembered.
            PickerOrder::Number, // Species
            PickerOrder::Name,   // Moves
            PickerOrder::Name    // Items
        };

        constexpr size_t SEARCH_LENGTH = 32;
    }

    OptionPickerScreen::OptionPickerScreen(std::string title, std::vector<std::string> options,
        size_t current, std::vector<size_t> spriteIndices, PickHandler onPick,
        std::vector<unsigned int> numbers, PickerKind kind)
        : title_(std::move(title)),
          options_(std::move(options)),
          spriteIndices_(std::move(spriteIndices)),
          numbers_(std::move(numbers)),
          kind_(kind),
          current_(options_.empty() ? 0 : std::min(current, options_.size() - 1)),
          order_(rememberedOrders[static_cast<size_t>(kind)]),
          onPick_(std::move(onPick))
    {
        // A short list keeps the order it was given.
        if (!searchable())
        {
            order_ = PickerOrder::Number;
            numbers_.clear();
        }
        // Open on the current option.
        refresh(current_);
    }

    void OptionPickerScreen::refresh(size_t option)
    {
        entries_ = pickerEntries(options_, numbers_, query_, order_);
        const auto found = std::find(entries_.begin(), entries_.end(), option);
        index_ = found == entries_.end() ? 0 : static_cast<size_t>(found - entries_.begin());
    }

    void OptionPickerScreen::setOrder(PickerOrder order)
    {
        order_ = order;
        if (kind_ != PickerKind::Other)
        {
            rememberedOrders[static_cast<size_t>(kind_)] = order;
        }
        refresh(selectedOption());
    }

    size_t OptionPickerScreen::selectedOption() const
    {
        return index_ < entries_.size() ? entries_[index_] : current_;
    }

    void OptionPickerScreen::update(App& app, Action action)
    {
        if (options_.empty())
        {
            app.pop();
            return;
        }
        const size_t count = entries_.size();
        const size_t last  = count > 0 ? count - 1 : 0;
        switch (action)
        {
            case Action::Confirm:
                // Nothing to pick while the search matches nothing.
                if (count > 0)
                {
                    const size_t picked = entries_[index_];
                    app.pop();
                    onPick_(picked);
                }
                break;
            case Action::Back:
            case Action::Quit:
                app.pop();
                break;
            case Action::Move: // Y: search.
                if (searchable())
                {
                    std::string typed;
                    if (app.ui().editText(
                            tr(Text::PickerSearchTitle, {title_}), query_, typed, SEARCH_LENGTH))
                    {
                        const size_t option = selectedOption();
                        query_              = typed;
                        refresh(option);
                    }
                }
                break;
            case Action::Alternate: // −: clear the search.
                if (searchable() && !query_.empty())
                {
                    const size_t option = selectedOption();
                    query_.clear();
                    refresh(option);
                }
                break;
            case Action::Discard: // X: the other order.
                if (searchable())
                {
                    setOrder(order_ == PickerOrder::Number ? PickerOrder::Name : PickerOrder::Number);
                }
                break;
            case Action::Left:
                index_ = index_ > 0 ? index_ - 1 : index_;
                break;
            case Action::Right:
                index_ = std::min(last, index_ + 1);
                break;
            case Action::Up:
                index_ = index_ >= COLUMNS ? index_ - COLUMNS : index_;
                break;
            case Action::Down:
                index_ = index_ + COLUMNS <= last ? index_ + COLUMNS : index_;
                break;
            case Action::PreviousPage:
                index_ = turnPage(index_, count, PER_PAGE, -1);
                break;
            case Action::NextPage:
                index_ = turnPage(index_, count, PER_PAGE, 1);
                break;
            default:
                break;
        }
    }

    void OptionPickerScreen::draw(SwitchUi& ui)
    {
        OptionPickerView view;
        view.title         = title_;
        view.current       = options_.empty() ? std::string{} : options_[current_];
        view.options       = &options_;
        view.spriteIndices = &spriteIndices_;
        view.entries       = &entries_;
        view.selected      = index_;
        view.searchable    = searchable();
        view.query         = query_;
        view.order         = order_;
        view.page          = index_ / PER_PAGE;
        view.pageCount     = pageCount(entries_.size(), PER_PAGE);
        ui.drawOptionPicker(view);

        index_ = entries_.empty() ? 0 : std::min(view.selected, entries_.size() - 1);
        if (view.orderRequest)
        {
            setOrder(order_ == PickerOrder::Number ? PickerOrder::Name : PickerOrder::Number);
        }
    }

    NaturePickerScreen::NaturePickerScreen(
        std::vector<std::string> natures, size_t current, PickHandler onPick)
        : natures_(std::move(natures)), current_(current), onPick_(std::move(onPick))
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
        ui.drawNaturePicker(natures_, current_, row_, column_);
    }

    UserPickerScreen::UserPickerScreen(
        std::vector<std::string> users, size_t current, PickHandler onPick)
        : users_(std::move(users)),
          current_(users_.empty() ? 0 : std::min(current, users_.size() - 1)),
          index_(current_),
          onPick_(std::move(onPick))
    {
    }

    void UserPickerScreen::update(App& app, Action action)
    {
        if (users_.empty())
        {
            app.pop();
            return;
        }
        const size_t last = users_.size() - 1;
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
            case Action::Left:
                index_ = index_ > 0 ? index_ - 1 : last;
                break;
            case Action::Right:
                index_ = index_ < last ? index_ + 1 : 0;
                break;
            case Action::Up:
                index_ = index_ >= COLUMNS ? index_ - COLUMNS : index_;
                break;
            case Action::Down:
                index_ = index_ + COLUMNS <= last ? index_ + COLUMNS : index_;
                break;
            default:
                break;
        }
    }

    void UserPickerScreen::draw(SwitchUi& ui)
    {
        ui.drawUserPicker(users_, current_, index_);
    }

    DatePickerScreen::DatePickerScreen(std::string title, const Date& current, YearRange years,
        std::function<void(const Date&)> onPick)
        : title_(std::move(title)),
          current_(formatDate(current)),
          years_(years),
          date_(clampDate(current, years)),
          onPick_(std::move(onPick))
    {
    }

    void DatePickerScreen::step(int direction)
    {
        date_ = stepDatePart(date_, PARTS[column_], direction, years_);
    }

    void DatePickerScreen::update(App& app, Action action)
    {
        constexpr size_t last = std::size(PARTS) - 1;
        switch (action)
        {
            case Action::Confirm:
            {
                const Date picked = date_;
                app.pop();
                onPick_(picked);
                break;
            }
            case Action::Back:
            case Action::Quit:
                app.pop();
                break;
            case Action::Left:
                column_ = column_ > 0 ? column_ - 1 : column_;
                break;
            case Action::Right:
                column_ = std::min(last, column_ + 1);
                break;
            case Action::Up:
                step(1);
                break;
            case Action::Down:
                step(-1);
                break;
            default:
                break;
        }
    }

    void DatePickerScreen::draw(SwitchUi& ui)
    {
        const auto shown = [](DatePart part, const Date& date)
        {
            switch (part)
            {
                case DatePart::Day:
                    return std::to_string(date.day());
                case DatePart::Month:
                    return monthName(date.month());
                case DatePart::Year:
                    return std::to_string(date.year());
            }
            return std::string{};
        };
        constexpr Text headers[] = {Text::DateDay, Text::DateMonth, Text::DateYear};

        DatePickerView view;
        view.title   = title_;
        view.current = current_;
        view.column  = column_;
        for (size_t i = 0; i < std::size(PARTS); ++i)
        {
            view.columns.push_back({tr(headers[i]), shown(PARTS[i], date_),
                shown(PARTS[i], stepDatePart(date_, PARTS[i], 1, years_)),
                shown(PARTS[i], stepDatePart(date_, PARTS[i], -1, years_))});
        }
        ui.drawDatePicker(view);

        // A touch selects a column, and on a value beside the current one also picks it.
        column_ = std::min(view.column, std::size(PARTS) - 1);
        if (view.stepRequest != 0)
        {
            step(view.stepRequest);
        }
    }
}
