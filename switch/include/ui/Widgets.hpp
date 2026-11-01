#pragma once

#include "imgui.h"
#include "platform/SystemStatus.hpp"
#include "ui/Icons.hpp"

#include <initializer_list>
#include <string>
#include <vector>

// The building blocks every screen of the design canvas shares: the PKSM header, panels, list
// rows, dashed "import" buttons and the button legend. They draw into the current ImGui window
// with the colours and sizes of ui/Theme.hpp.
namespace pksm::switchui::widgets
{
    // Draws `text` at the cursor at `size` pixels in `color` (0xRRGGBB), in the bold font
    // when `bold`, then moves the cursor below it.
    void text(const char* text, float size, unsigned color, bool bold = false);

    // Draws `text` at `position`; when it is wider than `maxWidth`, cuts it short and ends it
    // with "…" so it fits.
    void drawTextEllipsis(ImDrawList* drawList, ImFont* font, float size, const ImVec2& position,
        ImU32 color, const char* text, float maxWidth);

    // A button on the header bar, such as "Pokédex": an icon in a white circle and a label.
    struct HeaderButton
    {
        const char* label;
        icons::Icon icon;
    };

    // The red bar across the top of the screen: a Poké Ball, "PKSM" and `title` on the left,
    // and `status` (the user, the time and the battery) on the right, over a black strip, with
    // `button` before the user when there is one. It ignores the window padding and leaves the
    // cursor under the strip. Returns true when the button is touched.
    bool drawHeader(
        const char* title, const HeaderStatus& status, const HeaderButton* button = nullptr);

    // One button of the legend: the button's name, such as "A", "L / R" or "+", and what it does.
    struct ButtonHint
    {
        const char* button;
        const char* label;
    };

    // The button legend along the bottom right of the current window. It does not move the cursor,
    // so content above should leave theme::LEGEND_HEIGHT free. Returns how wide it is.
    float drawButtonLegend(std::initializer_list<ButtonHint> hints);
    // The same, for a legend whose buttons depend on the screen's state.
    float drawButtonLegend(const std::vector<ButtonHint>& hints);

    // `text` in `color` (0xRRGGBB) along the bottom left of the current window, on the legend's
    // line, cut short with "…" past `maxWidth`.
    void drawFooterMessage(const std::string& text, unsigned color, float maxWidth);

    // A rounded panel of `size` (0 or negative fills, as for BeginChild) with an optional
    // `caption` such as "GEN IX · ESCARLATA". Always call endPanel() afterwards.
    bool beginPanel(const char* id, const ImVec2& size, const char* caption = nullptr,
        ImGuiWindowFlags flags = ImGuiWindowFlags_None);
    void endPanel();

    // The accent outline around a selected card or row, 2 pixels outside it. Leave at least
    // 6 pixels around the item free, or the window clips the outline.
    void drawSelectionOutline(ImDrawList* drawList, const ImVec2& min, const ImVec2& max,
        float rounding);

    // A row of a list panel: an icon box, a title, an optional subtitle and an optional detail
    // line in a smaller font (used for file paths), and an arrow on the right.
    struct ListRow
    {
        const char* title;
        const char* subtitle = nullptr;
        const char* detail   = nullptr;
        icons::Icon icon     = icons::Icon::None;
        bool enabled         = true;
    };

    // A card of `size` for one editable value, as on the canvas trainer screen: `label` and a
    // pencil when `editable` at the top, the value large at the bottom. Returns true when touched.
    bool drawFieldCard(const char* id, const char* label, const char* value, const ImVec2& size,
        bool selected, bool editable);

    // Draws `row` across the available width. Returns true when touched.
    bool drawListRow(const char* id, const ListRow& row, bool selected);

    // A dashed outline of the rectangle from `min` to `max`, inside it, in `color` (0xRRGGBB).
    void drawDashedRect(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, float rounding,
        float thickness, unsigned color, float dash = 9.0f, float gap = 6.0f);

    // A transparent button with a dashed outline and a "+" before `label`, such as "Import save".
    // With `stacked`, the "+" sits above the label, as on the game grid. Returns true when touched.
    bool drawDashedButton(const char* id, const char* label, const ImVec2& size, bool stacked = false,
        float rounding = 16.0f);

    // A small dot of `dotColor` (0xRRGGBB) and `text` after it, as in "● Has saves", with its
    // bottom left corner at `bottomLeft`. Without `filled`, the dot is a ring, as in "○ No saves".
    // It does not move the cursor.
    void drawStatus(ImDrawList* drawList, const ImVec2& bottomLeft, const char* text,
        unsigned dotColor, bool filled = true);
}
