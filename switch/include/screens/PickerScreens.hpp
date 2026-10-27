#pragma once

#include "SwitchUi.hpp"
#include "app/Screen.hpp"
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace pksm::switchui
{
    // Called with the chosen index after the picker has closed itself.
    using PickHandler = std::function<void(size_t index)>;

    // A long list of options in a dialog over the screen below, COLUMNS to a row and ROWS rows
    // to a page. The D-pad moves one option or one row, L/R a page.
    class OptionPickerScreen : public Screen
    {
    public:
        static constexpr size_t COLUMNS = SwitchUi::PICKER_COLUMNS;
        static constexpr size_t ROWS    = SwitchUi::PICKER_ROWS;

        OptionPickerScreen(std::string title, std::vector<std::string> options, size_t current,
            std::vector<size_t> spriteIndices, PickHandler onPick);

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;
        bool isOverlay() const override { return true; }

    private:
        std::string title_;
        std::vector<std::string> options_;
        std::vector<size_t> spriteIndices_;
        size_t current_;
        size_t index_;
        // The first row shown; it follows the selection.
        size_t firstRow_ = 0;
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
        size_t row_    = 0;
        size_t column_ = 0;
        PickHandler onPick_;
    };
}
