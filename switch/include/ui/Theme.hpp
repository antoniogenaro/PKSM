#pragma once

#include "imgui.h"

#include <string>

// The look of the Switch app, taken from the "PKSM Switch" design canvas: its colours, corner
// radii, text sizes and spacing. Screens use these names instead of their own values.
namespace pksm::switchui::theme
{
    // A colour written as 0xRRGGBB, like the canvas does.
    constexpr ImU32 rgb(unsigned hex, unsigned alpha = 255)
    {
        return IM_COL32((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, alpha);
    }

    // The same colour for the style and colour stacks, which take ImVec4.
    constexpr ImVec4 vec(unsigned hex, float alpha = 1.0f)
    {
        return ImVec4(static_cast<float>((hex >> 16) & 0xFF) / 255.0f,
            static_cast<float>((hex >> 8) & 0xFF) / 255.0f, static_cast<float>(hex & 0xFF) / 255.0f,
            alpha);
    }

    // Colours, as 0xRRGGBB.
    inline constexpr unsigned BACKGROUND    = 0x12141C;
    inline constexpr unsigned PANEL         = 0x181B26; // Panels that group cards or rows.
    inline constexpr unsigned CARD          = 0x1E2230; // A card or row inside a panel.
    inline constexpr unsigned CARD_SELECTED = 0x283048;
    inline constexpr unsigned BORDER        = 0x2E3448; // Panel borders, dividers, icon boxes.
    inline constexpr unsigned DASHED        = 0x4A5170; // The dashed outline of "import" buttons.
    inline constexpr unsigned TEXT          = 0xF2F4F8;
    inline constexpr unsigned TEXT_SOFT     = 0xC9CFE0; // Subtitles and the button legend.
    inline constexpr unsigned TEXT_MUTED    = 0xA9B0C2; // Captions and secondary lines.
    inline constexpr unsigned TEXT_PATH     = 0x98A1B8; // File paths.
    inline constexpr unsigned BUTTON_FACE   = 0xE6E9F2; // The circles of the button legend.
    inline constexpr unsigned ACCENT        = 0x00C3E3; // The selection.
    inline constexpr unsigned POSITIVE      = 0x4ADE80; // "Has saves".
    inline constexpr unsigned NEUTRAL       = 0x7C8499; // "No saves".
    inline constexpr unsigned ERROR_TEXT    = 0xFF7359;
    inline constexpr unsigned POKEBALL_RED  = 0xE23D28;

    // Corner radii.
    inline constexpr float PANEL_RADIUS    = 20.0f;
    inline constexpr float ROW_RADIUS      = 16.0f;
    inline constexpr float CARD_RADIUS     = 14.0f;
    inline constexpr float ICON_BOX_RADIUS = 10.0f;

    // Text sizes in pixels.
    inline constexpr float FONT_TITLE     = 40.0f; // "PKSM" in the header.
    inline constexpr float FONT_SUBTITLE  = 20.0f; // The line under the header.
    inline constexpr float FONT_HEADING   = 22.0f; // The name of the open game.
    inline constexpr float FONT_ROW_TITLE = 18.0f;
    inline constexpr float FONT_BODY      = 16.0f; // The default size.
    inline constexpr float FONT_SMALL     = 14.0f;
    inline constexpr float FONT_CAPTION   = 13.0f; // Panel captions such as "GEN IX · ESCARLATA".
    inline constexpr float FONT_PATH      = 12.0f;

    // Layout.
    inline constexpr float SCREEN_PADDING_X = 48.0f;
    inline constexpr float SCREEN_PADDING_Y = 24.0f;
    inline constexpr float SECTION_GAP      = 16.0f; // Between header, content and legend.
    inline constexpr float PANEL_GAP        = 20.0f; // Between panels side by side.
    inline constexpr float PANEL_PADDING    = 12.0f;
    inline constexpr float ITEM_GAP         = 8.0f;  // Between rows or cards in a panel.
    inline constexpr float SIDE_PANEL_WIDTH = 300.0f;
    inline constexpr float LIST_ROW_HEIGHT  = 72.0f;
    inline constexpr float ICON_BOX_SIZE    = 48.0f;
    inline constexpr float LEGEND_HEIGHT    = 26.0f;

    // Where the UI fonts are in romfs. The canvas uses a bold weight for titles, captions and
    // names; Medium stands in for it.
    inline constexpr char FONT_REGULAR_PATH[] = "romfs:/fonts/Roboto.ttf";
    inline constexpr char FONT_MEDIUM_PATH[]  = "romfs:/fonts/Roboto-Medium.ttf";

    // Sets the colours, radii and spacing above on `style`.
    void applyStyle(ImGuiStyle& style);

    // Loads both UI fonts into the font atlas of the current ImGui context; the regular one
    // becomes the default. On failure, returns false and sets `failedPath` to the font that
    // could not be read.
    bool loadFonts(const char*& failedPath);

    // The heavier font for titles, captions and names. Draw it with PushFont(mediumFont(), size)
    // or pass it to ImDrawList::AddText.
    ImFont* mediumFont();
}
