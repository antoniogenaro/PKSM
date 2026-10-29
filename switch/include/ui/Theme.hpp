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
    inline constexpr unsigned BACKGROUND    = 0xF5F5F3;
    inline constexpr unsigned PANEL         = 0xFFFFFF; // Panels that group cards or rows.
    inline constexpr unsigned CARD          = 0xF2F2F0; // A card or row inside a panel.
    inline constexpr unsigned CARD_SELECTED = 0xF2F2F0; // The accent outline marks the selection.
    inline constexpr unsigned CONTROL       = 0xE8E8E5; // Pills and value buttons on a card.
    inline constexpr unsigned ART           = 0xE4E4E1; // Behind a sprite or an item picture.
    inline constexpr unsigned EMPTY_SLOT    = 0xFAFAF8; // An empty box slot.
    inline constexpr unsigned BORDER        = 0xE2E2DF; // Panel borders, dividers, icon boxes.
    inline constexpr unsigned DASHED        = 0x9A9A98; // Dashed outlines and "no saves" rings.
    inline constexpr unsigned TEXT          = 0x141414;
    inline constexpr unsigned TEXT_SOFT     = 0x2A2A28; // The button legend and pill labels.
    inline constexpr unsigned TEXT_MUTED    = 0x5E5E5C; // Captions and secondary lines.
    inline constexpr unsigned TEXT_PATH     = 0x6B6B69; // File paths.
    inline constexpr unsigned ON_DARK       = 0xFFFFFF; // Text and icons on the accent or black.
    inline constexpr unsigned BUTTON_FACE   = 0x141414; // The circles of the button legend.
    inline constexpr unsigned ACCENT        = 0xD71920; // The selection.
    inline constexpr unsigned CAPTION       = 0xC2161C; // Panel captions such as "GEN IX · …".
    inline constexpr unsigned RAISED        = 0xC2161C; // A stat the nature raises.
    inline constexpr unsigned LOWERED       = 0x2F6FD6; // A stat the nature lowers.
    inline constexpr unsigned GOLD          = 0x8A6100; // Neutral natures and switches that are on.
    inline constexpr unsigned POSITIVE      = 0xD71920; // "Has saves".
    inline constexpr unsigned NEUTRAL       = 0x9A9A98; // "No saves".
    inline constexpr unsigned ERROR_TEXT    = 0xC2161C;
    inline constexpr unsigned POKEBALL_RED  = 0xE23D28;
    inline constexpr unsigned HEADER        = 0xD71920; // The header bar.
    inline constexpr unsigned HEADER_EDGE   = 0x141414; // The strip under the header bar.

    // Corner radii.
    inline constexpr float PANEL_RADIUS    = 20.0f;
    inline constexpr float ROW_RADIUS      = 16.0f;
    inline constexpr float CARD_RADIUS     = 14.0f;
    inline constexpr float ICON_BOX_RADIUS = 10.0f;

    // Text sizes in pixels.
    inline constexpr float FONT_TITLE     = 28.0f; // "PKSM" in the header.
    inline constexpr float FONT_SUBTITLE  = 18.0f; // The screen's name in the header.
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
    inline constexpr float HEADER_HEIGHT    = 64.0f; // The red bar, across the whole screen.
    inline constexpr float HEADER_EDGE_SIZE = 6.0f;  // The black strip under it.

    // Where the UI fonts are in romfs. The canvas uses a bold weight for titles, captions and
    // names; Roboto Bold covers both of its bold weights, 700 and 800.
    inline constexpr char FONT_REGULAR_PATH[] = "romfs:/fonts/Roboto.ttf";
    inline constexpr char FONT_BOLD_PATH[]    = "romfs:/fonts/Roboto-Bold.ttf";

    // Sets the colours, radii and spacing above on `style`.
    void applyStyle(ImGuiStyle& style);

    // Loads both UI fonts into the font atlas of the current ImGui context; the regular one
    // becomes the default. On failure, returns false and sets `failedPath` to the font that
    // could not be read.
    bool loadFonts(const char*& failedPath);

    // The heavier font for titles, captions and names. Draw it with PushFont(boldFont(), size)
    // or pass it to ImDrawList::AddText.
    ImFont* boldFont();
}
