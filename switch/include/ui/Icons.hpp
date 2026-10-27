#pragma once

#include "imgui.h"

#include <SDL.h>

// The line icons of the design canvas. Each is an SVG file in switch/icons, drawn white, which
// romfs holds in romfs:/icons. They are turned into textures at the pixel size they are drawn
// at, so they stay sharp, and tinted when drawn.
namespace pksm::switchui::icons
{
    enum class Icon
    {
        None,
        Boxes,   // Four squares: the boxes.
        Bank,    // A stack of coins.
        Bag,     // The bag and its items.
        Gift,    // Mystery Gifts and events.
        Trainer, // A person.
        More,    // Three dots: other tools.
        Backup,  // Two overlapping squares: a backup copy.
        Count
    };

    // Reads every icon file. Returns false, with `failedPath` set to the file, when one cannot be
    // read or is not an SVG. `renderer` makes the textures and must outlive unload().
    bool load(SDL_Renderer* renderer, const char*& failedPath);

    // Frees the textures and the parsed files. Call it before destroying the renderer.
    void unload();

    // Draws `icon` in `color`, centred in the icon box at `min` that is `boxSize` wide. As on the
    // canvas, an icon fills 26 pixels of a 48 pixel box.
    void draw(ImDrawList* drawList, Icon icon, const ImVec2& min, float boxSize, ImU32 color);
}
