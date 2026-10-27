#pragma once

#include "app/Screen.hpp"
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace pksm::switchui
{
    // Called with the chosen index after the picker has closed itself.
    using PickHandler = std::function<void(size_t index)>;

    // A long list of options. Up/Down move one, Left/Right ten and L/R a hundred.
    class OptionPickerScreen : public Screen
    {
    public:
        OptionPickerScreen(std::string title, std::vector<std::string> options, size_t current,
            std::vector<size_t> spriteIndices, PickHandler onPick);

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;

    private:
        static constexpr size_t VISIBLE_ROWS = 9;

        std::string title_;
        std::vector<std::string> options_;
        std::vector<size_t> spriteIndices_;
        size_t index_;
        PickHandler onPick_;
    };

    // The 25 natures as a grid of raised against lowered stat.
    class NaturePickerScreen : public Screen
    {
    public:
        NaturePickerScreen(std::vector<std::string> natures, size_t current, PickHandler onPick);

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;

    private:
        std::vector<std::string> natures_;
        size_t row_    = 0;
        size_t column_ = 0;
        PickHandler onPick_;
    };
}
