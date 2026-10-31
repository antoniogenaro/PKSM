#pragma once

#include "SwitchUi.hpp"
#include "app/Screen.hpp"
#include "domain/PickerList.hpp"
#include "domain/FieldValues.hpp"
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace pksm::switchui
{
    // Called with the chosen index after the picker has closed itself.
    using PickHandler = std::function<void(size_t index)>;

    // A long list of options in a dialog over the screen below, COLUMNS to a row and ROWS rows
    // to a page. The D-pad moves one option or one row, L/R a page. A list longer than a page
    // also has a search (Y opens the keyboard, − clears it) and an order (X switches between
    // number and name); its page is shown.
    class OptionPickerScreen : public Screen
    {
    public:
        static constexpr size_t COLUMNS  = SwitchUi::PICKER_COLUMNS;
        static constexpr size_t ROWS     = SwitchUi::PICKER_ROWS;
        static constexpr size_t PER_PAGE = COLUMNS * ROWS;

        // `numbers` holds each option's number for the number order; without them that order
        // is the order given.
        OptionPickerScreen(std::string title, std::vector<std::string> options, size_t current,
            std::vector<size_t> spriteIndices, PickHandler onPick,
            std::vector<unsigned int> numbers = {}, PickerKind kind = PickerKind::Other);

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;
        bool isOverlay() const override { return true; }

    private:
        bool searchable() const { return options_.size() > PER_PAGE; }
        // Lists the options again after the search or the order changed, keeping `option`
        // selected when it is still listed.
        void refresh(size_t option);
        void setOrder(PickerOrder order);
        // The option selected, as a position in `options_`.
        size_t selectedOption() const;

        std::string title_;
        std::vector<std::string> options_;
        std::vector<size_t> spriteIndices_;
        std::vector<unsigned int> numbers_;
        PickerKind kind_;
        size_t current_;
        std::string query_;
        PickerOrder order_;
        // The options shown, as positions in `options_`.
        std::vector<size_t> entries_;
        // The selection, as a position in `entries_`.
        size_t index_ = 0;
        PickHandler onPick_;
    };

    // The 25 natures as a grid of raised against lowered stat, in a dialog over the screen below.
    class NaturePickerScreen : public Screen
    {
    public:
        NaturePickerScreen(std::vector<std::string> natures, size_t current, PickHandler onPick);

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;
        bool isOverlay() const override { return true; }

    private:
        std::vector<std::string> natures_;
        size_t current_;
        size_t row_    = 0;
        size_t column_ = 0;
        PickHandler onPick_;
    };

    // The console users as cards in a dialog over the screen below, four to a row. `onPick`
    // gets the chosen user after the picker has closed itself, only on A.
    class UserPickerScreen : public Screen
    {
    public:
        static constexpr size_t COLUMNS = 4;

        UserPickerScreen(std::vector<std::string> users, size_t current, PickHandler onPick);

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;
        bool isOverlay() const override { return true; }

    private:
        std::vector<std::string> users_;
        size_t current_;
        size_t index_;
        PickHandler onPick_;
    };

    // A date in a dialog over the screen below, as day, month and year columns. Left/Right pick
    // the column and Up/Down change it; the day always fits the month and the year stays in
    // `years`. `onPick` gets the date after the picker has closed itself, only on A.
    class DatePickerScreen : public Screen
    {
    public:
        DatePickerScreen(std::string title, const Date& current, YearRange years,
            std::function<void(const Date&)> onPick);

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;
        bool isOverlay() const override { return true; }

    private:
        static constexpr DatePart PARTS[] = {DatePart::Day, DatePart::Month, DatePart::Year};

        void step(int direction);

        std::string title_;
        std::string current_;
        YearRange years_;
        Date date_;
        size_t column_ = 0;
        std::function<void(const Date&)> onPick_;
    };
}
