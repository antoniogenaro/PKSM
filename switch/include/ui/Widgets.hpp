#pragma once

#include "imgui.h"

#include <initializer_list>

// The building blocks every screen of the design canvas shares: the PKSM header, panels, list
// rows, dashed "import" buttons and the button legend. They draw into the current ImGui window
// with the colours and sizes of ui/Theme.hpp.
namespace pksm::switchui::widgets
{
    // Draws `text` at the cursor at `size` pixels in `color` (0xRRGGBB), in the medium font
    // when `medium`, then moves the cursor below it.
    void text(const char* text, float size, unsigned color, bool medium = false);

    // A Poké Ball of `radius` centred on `center`.
    void drawPokeBall(ImDrawList* drawList, const ImVec2& center, float radius);

    // "PKSM" between two Poké Balls, centred, with `subtitle` under it. Uses the full width.
    void drawHeader(const char* subtitle);

    // One button of the legend: the button's name, such as "A", "L / R" or "+", and what it does.
    struct ButtonHint
    {
        const char* button;
        const char* label;
    };

    // The button legend along the bottom right of the current window. It does not move the cursor,
    // so content above should leave theme::LEGEND_HEIGHT free.
    void drawButtonLegend(std::initializer_list<ButtonHint> hints);

    // A rounded panel of `size` (0 or negative fills, as for BeginChild) with an optional
    // `caption` such as "GEN IX · ESCARLATA". Always call endPanel() afterwards.
    bool beginPanel(const char* id, const ImVec2& size, const char* caption = nullptr,
        ImGuiWindowFlags flags = ImGuiWindowFlags_None);
    void endPanel();

    // The accent outline around a selected card or row, 2 pixels outside it. Leave at least
    // 6 pixels around the item free, or the window clips the outline.
    void drawSelectionOutline(ImDrawList* drawList, const ImVec2& min, const ImVec2& max,
        float rounding);

    // Draws an icon inside the square that starts at `min` and is `size` wide, in `color`.
    using IconPainter = void (*)(ImDrawList* drawList, const ImVec2& min, float size, ImU32 color);

    // A row of a list panel: an icon box, a title, an optional subtitle and an optional detail
    // line in a smaller font (used for file paths), and an arrow on the right.
    struct ListRow
    {
        const char* title;
        const char* subtitle = nullptr;
        const char* detail   = nullptr;
        IconPainter icon     = nullptr;
        bool enabled         = true;
    };

    // Draws `row` across the available width. Returns true when touched.
    bool drawListRow(const char* id, const ListRow& row, bool selected);

    // A transparent button with a dashed outline and a "+" before `label`, such as "Import save".
    // With `stacked`, the "+" sits above the label, as on the game grid. Returns true when touched.
    bool drawDashedButton(const char* id, const char* label, const ImVec2& size, bool stacked = false,
        float rounding = 16.0f);

    // A small dot of `dotColor` (0xRRGGBB) and `text` after it, as in "● Has saves", with its
    // bottom left corner at `bottomLeft`. It does not move the cursor.
    void drawStatus(
        ImDrawList* drawList, const ImVec2& bottomLeft, const char* text, unsigned dotColor);
}
