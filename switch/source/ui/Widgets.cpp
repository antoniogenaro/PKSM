#include "ui/Widgets.hpp"
#include "ui/Theme.hpp"

#include "imgui_internal.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>

namespace pksm::switchui::widgets
{
    namespace
    {
        constexpr float PI = 3.14159265358979323846f;

        ImFont* fontFor(bool medium)
        {
            return medium ? theme::mediumFont() : ImGui::GetFont();
        }

        // Text at `position` at `size`, kept inside `clip` when given.
        void drawText(ImDrawList* drawList, const ImVec2& position, float size, ImU32 color,
            const char* text, const ImVec4* clip = nullptr, bool medium = false)
        {
            drawList->AddText(fontFor(medium), size, position, color, text, nullptr, 0.0f, clip);
        }

        ImVec2 textSize(const char* text, float size, bool medium = false)
        {
            ImGui::PushFont(medium ? theme::mediumFont() : nullptr, size);
            const ImVec2 result = ImGui::CalcTextSize(text);
            ImGui::PopFont();
            return result;
        }

        // A straight dashed line from `from` to `to`.
        void drawDashedLine(ImDrawList* drawList, const ImVec2& from, const ImVec2& to, ImU32 color,
            float thickness)
        {
            constexpr float dash = 9.0f;
            constexpr float gap  = 6.0f;
            const float dx       = to.x - from.x;
            const float dy       = to.y - from.y;
            const float length   = std::sqrt(dx * dx + dy * dy);
            if (length <= 0.0f)
            {
                return;
            }
            const float ux = dx / length;
            const float uy = dy / length;
            for (float start = 0.0f; start < length; start += dash + gap)
            {
                const float end = std::min(start + dash, length);
                drawList->AddLine(ImVec2(from.x + ux * start, from.y + uy * start),
                    ImVec2(from.x + ux * end, from.y + uy * end), color, thickness);
            }
        }

        void drawChevron(ImDrawList* drawList, const ImVec2& min, ImU32 color)
        {
            // The canvas arrow: 14 x 22, drawn with a 3 pixel stroke.
            const ImVec2 points[] = {ImVec2(min.x + 3.0f, min.y + 3.0f),
                ImVec2(min.x + 11.0f, min.y + 11.0f), ImVec2(min.x + 3.0f, min.y + 19.0f)};
            drawList->AddPolyline(points, 3, color, ImDrawFlags_None, 3.0f);
        }
    }

    void drawTextEllipsis(ImDrawList* drawList, ImFont* font, float size, const ImVec2& position,
        ImU32 color, const char* text, float maxWidth)
    {
        const char* end     = text + std::strlen(text);
        const auto widthTo  = [&](const char* stop)
        {
            return font->CalcTextSizeA(size, FLT_MAX, 0.0f, text, stop).x;
        };
        if (widthTo(end) <= maxWidth)
        {
            drawList->AddText(font, size, position, color, text, end);
            return;
        }

        // Drop whole characters from the end until the rest and the ellipsis fit.
        static constexpr char ellipsis[] = "\xE2\x80\xA6";
        const float ellipsisWidth = font->CalcTextSizeA(size, FLT_MAX, 0.0f, ellipsis).x;
        while (end > text)
        {
            do
            {
                --end;
            } while (end > text && (static_cast<unsigned char>(*end) & 0xC0) == 0x80);
            if (widthTo(end) + ellipsisWidth <= maxWidth)
            {
                break;
            }
        }
        while (end > text && end[-1] == ' ')
        {
            --end;
        }
        drawList->AddText(font, size, position, color, text, end);
        drawList->AddText(
            font, size, ImVec2(position.x + widthTo(end), position.y), color, ellipsis);
    }

    void text(const char* text, float size, unsigned color, bool medium)
    {
        ImGui::PushFont(medium ? theme::mediumFont() : nullptr, size);
        ImGui::PushStyleColor(ImGuiCol_Text, theme::vec(color));
        ImGui::TextUnformatted(text);
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }

    void drawPokeBall(ImDrawList* drawList, const ImVec2& center, float radius)
    {
        // Proportions of the 44 pixel ball on the canvas, whose radius is 20.
        const ImU32 white = theme::rgb(theme::TEXT);
        const ImU32 dark  = theme::rgb(theme::BACKGROUND);
        drawList->AddCircleFilled(center, radius, white, 40);
        drawList->PathArcTo(center, radius, PI, 2.0f * PI, 40);
        drawList->PathFillConvex(theme::rgb(theme::POKEBALL_RED));
        drawList->AddRectFilled(ImVec2(center.x - radius, center.y - radius * 0.1f),
            ImVec2(center.x + radius, center.y + radius * 0.1f), dark);
        drawList->AddCircle(center, radius, dark, 40, radius * 0.1f);
        drawList->AddCircleFilled(center, radius * 0.35f, white, 24);
        drawList->AddCircle(center, radius * 0.35f, dark, 24, radius * 0.2f);
    }

    void drawHeader(const char* subtitle)
    {
        constexpr float ballSize   = 44.0f;
        constexpr float ballGap    = 20.0f;
        constexpr float titleRow   = 56.0f;
        // The canvas pulls the subtitle 10 pixels up into the gap below the title row.
        constexpr float subtitleY  = titleRow + theme::SECTION_GAP - 10.0f;
        constexpr float subtitleH  = 22.0f;
        static constexpr char title[] = "PKSM";

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 origin  = ImGui::GetCursorScreenPos();
        const float width    = ImGui::GetContentRegionAvail().x;

        const ImVec2 titleSize = textSize(title, theme::FONT_TITLE, true);
        const float rowWidth   = ballSize * 2.0f + ballGap * 2.0f + titleSize.x;
        const float left       = origin.x + (width - rowWidth) * 0.5f;
        const float centerY    = origin.y + titleRow * 0.5f;
        drawPokeBall(drawList, ImVec2(left + ballSize * 0.5f, centerY), 20.0f);
        const ImVec2 titlePosition(left + ballSize + ballGap, centerY - titleSize.y * 0.5f);
        const ImU32 titleColor = theme::rgb(theme::TEXT);
        drawText(drawList, titlePosition, theme::FONT_TITLE, titleColor, title, nullptr, true);
        drawPokeBall(drawList,
            ImVec2(left + ballSize + ballGap * 2.0f + titleSize.x + ballSize * 0.5f, centerY),
            20.0f);

        if (subtitle && subtitle[0] != '\0')
        {
            const ImVec2 size = textSize(subtitle, theme::FONT_SUBTITLE, true);
            drawText(drawList,
                ImVec2(origin.x + (width - size.x) * 0.5f,
                    origin.y + subtitleY + (subtitleH - size.y) * 0.5f),
                theme::FONT_SUBTITLE, theme::rgb(theme::TEXT_SOFT), subtitle, nullptr, true);
        }
        ImGui::Dummy(ImVec2(width, subtitleY + subtitleH));
    }

    void drawButtonLegend(std::initializer_list<ButtonHint> hints)
    {
        constexpr float hintGap   = 32.0f;
        constexpr float faceGap   = 8.0f;
        constexpr float faceSize  = theme::LEGEND_HEIGHT;
        constexpr float facePad   = 7.0f; // Around names longer than one letter, such as "L / R".

        // The window's own padding: the style's may differ from what the window was begun with.
        const ImGuiWindow* window = ImGui::GetCurrentWindowRead();
        const ImVec2 windowPos    = window->Pos;
        const ImVec2 windowSize   = window->Size;
        const ImVec2 padding      = window->WindowPadding;
        const float top           = windowPos.y + windowSize.y - padding.y - faceSize;

        const auto faceWidth = [&](const ButtonHint& hint)
        {
            return std::max(faceSize, textSize(hint.button, theme::FONT_BODY, true).x +
                                      facePad * 2.0f);
        };
        float total = 0.0f;
        for (const ButtonHint& hint : hints)
        {
            total += (total > 0.0f ? hintGap : 0.0f) + faceWidth(hint) + faceGap +
                     textSize(hint.label, theme::FONT_BODY, true).x;
        }

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        float x              = windowPos.x + windowSize.x - padding.x - total;
        for (const ButtonHint& hint : hints)
        {
            const float width       = faceWidth(hint);
            const ImVec2 buttonSize = textSize(hint.button, theme::FONT_BODY, true);
            const ImVec2 labelSize  = textSize(hint.label, theme::FONT_BODY, true);
            drawList->AddRectFilled(ImVec2(x, top), ImVec2(x + width, top + faceSize),
                theme::rgb(theme::BUTTON_FACE), faceSize * 0.5f);
            const ImVec2 buttonPosition(
                x + (width - buttonSize.x) * 0.5f, top + (faceSize - buttonSize.y) * 0.5f);
            const ImU32 buttonColor = theme::rgb(theme::BACKGROUND);
            drawText(drawList, buttonPosition, theme::FONT_BODY, buttonColor, hint.button, nullptr,
                true);
            x += width + faceGap;
            drawText(drawList, ImVec2(x, top + (faceSize - labelSize.y) * 0.5f), theme::FONT_BODY,
                theme::rgb(theme::TEXT_SOFT), hint.label, nullptr, true);
            x += labelSize.x + hintGap;
        }
    }

    bool beginPanel(const char* id, const ImVec2& size, const char* caption, ImGuiWindowFlags flags)
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::vec(theme::PANEL));
        ImGui::PushStyleColor(ImGuiCol_Border, theme::vec(theme::BORDER));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, theme::PANEL_RADIUS);
        ImGui::PushStyleVar(
            ImGuiStyleVar_WindowPadding, ImVec2(theme::PANEL_PADDING, theme::PANEL_PADDING));
        const bool open = ImGui::BeginChild(
            id, size, ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding, flags);
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
        if (caption && caption[0] != '\0')
        {
            text(caption, theme::FONT_CAPTION, theme::TEXT_MUTED, true);
        }
        return open;
    }

    void endPanel()
    {
        ImGui::EndChild();
    }

    void drawSelectionOutline(
        ImDrawList* drawList, const ImVec2& min, const ImVec2& max, float rounding)
    {
        // A 4 pixel line 2 pixels outside the item, so its centre is 4 pixels out.
        constexpr float offset    = 4.0f;
        constexpr float thickness = 4.0f;
        drawList->AddRect(ImVec2(min.x - offset, min.y - offset),
            ImVec2(max.x + offset, max.y + offset), theme::rgb(theme::ACCENT), rounding + offset,
            ImDrawFlags_None, thickness);
    }

    namespace icons
    {
        namespace
        {
            // Maps the coordinates of a canvas SVG, `viewBox` units wide and drawn `glyph`
            // canvas pixels wide in the 48 pixel icon box, into the box at `min` of `size`.
            struct Pen
            {
                Pen(const ImVec2& min, float size, float viewBox, float glyph)
                {
                    const float boxScale = size / theme::ICON_BOX_SIZE;
                    unit                 = glyph * boxScale / viewBox;
                    const float offset   = (size - glyph * boxScale) * 0.5f;
                    origin               = ImVec2(min.x + offset, min.y + offset);
                }

                ImVec2 at(float x, float y) const
                {
                    return ImVec2(origin.x + x * unit, origin.y + y * unit);
                }

                float operator()(float length) const { return length * unit; }

                ImVec2 origin;
                float unit;
            };

            // The 26 pixel line icons share one 26 unit view box and a 2.4 unit line.
            Pen linePen(const ImVec2& min, float size)
            {
                return Pen(min, size, 26.0f, 26.0f);
            }

            constexpr float LINE = 2.4f;

            void roundedRect(ImDrawList* drawList, const Pen& pen, float x, float y, float width,
                float height, float radius, ImU32 color, float line)
            {
                drawList->AddRect(pen.at(x, y), pen.at(x + width, y + height), color, pen(radius),
                    ImDrawFlags_None, pen(line));
            }
        }

        void boxes(ImDrawList* drawList, const ImVec2& min, float size, ImU32 color)
        {
            const Pen pen = linePen(min, size);
            for (const float y : {3.0f, 15.0f})
            {
                for (const float x : {3.0f, 15.0f})
                {
                    roundedRect(drawList, pen, x, y, 8.0f, 8.0f, 2.0f, color, LINE);
                }
            }
        }

        void bank(ImDrawList* drawList, const ImVec2& min, float size, ImU32 color)
        {
            // A stack of coins: the top, the sides and two lower rims.
            const Pen pen = linePen(min, size);
            drawList->AddEllipse(pen.at(13.0f, 6.0f), ImVec2(pen(9.0f), pen(3.5f)), color, 0.0f,
                32, pen(LINE));
            drawList->AddLine(pen.at(4.0f, 6.0f), pen.at(4.0f, 20.0f), color, pen(LINE));
            drawList->AddLine(pen.at(22.0f, 6.0f), pen.at(22.0f, 20.0f), color, pen(LINE));
            for (const float y : {13.0f, 20.0f})
            {
                drawList->PathEllipticalArcTo(
                    pen.at(13.0f, y), ImVec2(pen(9.0f), pen(3.5f)), 0.0f, 0.0f, PI, 20);
                drawList->PathStroke(color, ImDrawFlags_None, pen(LINE));
            }
        }

        void bag(ImDrawList* drawList, const ImVec2& min, float size, ImU32 color)
        {
            const Pen pen = linePen(min, size);
            const ImVec2 body[] = {pen.at(6.0f, 9.0f), pen.at(20.0f, 9.0f), pen.at(21.5f, 23.0f),
                pen.at(4.5f, 23.0f)};
            drawList->AddPolyline(body, 4, color, ImDrawFlags_Closed, pen(LINE));
            // The handle: up from the bag, over a half circle and down again.
            drawList->PathLineTo(pen.at(9.0f, 9.0f));
            drawList->PathArcTo(pen.at(13.0f, 7.0f), pen(4.0f), PI, 2.0f * PI, 16);
            drawList->PathLineTo(pen.at(17.0f, 9.0f));
            drawList->PathStroke(color, ImDrawFlags_None, pen(LINE));
        }

        void gift(ImDrawList* drawList, const ImVec2& min, float size, ImU32 color)
        {
            const Pen pen = linePen(min, size);
            roundedRect(drawList, pen, 3.0f, 11.0f, 20.0f, 12.0f, 2.0f, color, LINE);
            roundedRect(drawList, pen, 2.0f, 7.0f, 22.0f, 4.0f, 1.5f, color, LINE);
            drawList->AddLine(pen.at(13.0f, 7.0f), pen.at(13.0f, 23.0f), color, pen(LINE));
            // The two loops of the bow, as in the canvas path.
            for (const float side : {-1.0f, 1.0f})
            {
                drawList->PathLineTo(pen.at(13.0f, 7.0f));
                drawList->PathBezierCubicCurveTo(pen.at(13.0f + side, 3.0f),
                    pen.at(13.0f + side * 6.0f, 3.0f), pen.at(13.0f + side * 6.0f, 5.5f));
                drawList->PathBezierCubicCurveTo(pen.at(13.0f + side * 6.0f, 8.0f),
                    pen.at(13.0f + side * 2.0f, 7.0f), pen.at(13.0f, 7.0f));
                drawList->PathStroke(color, ImDrawFlags_None, pen(LINE));
            }
        }

        void trainer(ImDrawList* drawList, const ImVec2& min, float size, ImU32 color)
        {
            const Pen pen = linePen(min, size);
            drawList->AddCircle(pen.at(13.0f, 8.0f), pen(4.5f), color, 24, pen(LINE));
            drawList->PathLineTo(pen.at(4.0f, 23.0f));
            drawList->PathBezierCubicCurveTo(
                pen.at(4.0f, 18.0f), pen.at(8.0f, 15.0f), pen.at(13.0f, 15.0f));
            drawList->PathBezierCubicCurveTo(
                pen.at(18.0f, 15.0f), pen.at(22.0f, 18.0f), pen.at(22.0f, 23.0f));
            drawList->PathStroke(color, ImDrawFlags_None, pen(LINE));
        }

        void more(ImDrawList* drawList, const ImVec2& min, float size, ImU32 color)
        {
            const Pen pen = linePen(min, size);
            for (const float x : {5.0f, 13.0f, 21.0f})
            {
                drawList->AddCircleFilled(pen.at(x, 13.0f), pen(2.0f + LINE * 0.5f), color, 12);
            }
        }

        void backup(ImDrawList* drawList, const ImVec2& min, float size, ImU32 color)
        {
            // A 34 unit view box drawn 26 pixels wide, with a 3 unit line.
            const Pen pen(min, size, 34.0f, 26.0f);
            roundedRect(drawList, pen, 11.0f, 11.0f, 20.0f, 20.0f, 4.0f, color, 3.0f);
            drawList->PathLineTo(pen.at(23.0f, 5.0f));
            drawList->PathLineTo(pen.at(8.0f, 5.0f));
            drawList->PathArcTo(pen.at(8.0f, 9.0f), pen(4.0f), PI * 1.5f, PI, 8);
            drawList->PathLineTo(pen.at(4.0f, 24.0f));
            drawList->PathStroke(color, ImDrawFlags_None, pen(3.0f));
        }
    }

    bool drawListRow(const char* id, const ListRow& row, bool selected)
    {
        constexpr float iconLeft   = 12.0f;
        constexpr float textGap    = 16.0f;
        constexpr float lineGap    = 3.0f;
        constexpr float arrowWidth = 14.0f;
        constexpr float arrowRight = 16.0f;

        ImGui::PushID(id);
        const ImVec2 min  = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x;
        const bool touched =
            ImGui::InvisibleButton("row", ImVec2(width, theme::LIST_ROW_HEIGHT));
        ImGui::PopID();
        const ImVec2 max(min.x + width, min.y + theme::LIST_ROW_HEIGHT);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(min, max,
            theme::rgb(selected ? theme::CARD_SELECTED : theme::CARD), theme::ROW_RADIUS);
        if (selected)
        {
            drawSelectionOutline(drawList, min, max, theme::ROW_RADIUS);
        }

        const bool highlighted = selected && row.enabled;
        const ImVec2 iconMin(
            min.x + iconLeft, min.y + (theme::LIST_ROW_HEIGHT - theme::ICON_BOX_SIZE) * 0.5f);
        drawList->AddRectFilled(iconMin,
            ImVec2(iconMin.x + theme::ICON_BOX_SIZE, iconMin.y + theme::ICON_BOX_SIZE),
            theme::rgb(highlighted ? theme::ACCENT : theme::BORDER), theme::ICON_BOX_RADIUS);
        if (row.icon)
        {
            row.icon(drawList, iconMin, theme::ICON_BOX_SIZE,
                theme::rgb(highlighted ? theme::BACKGROUND
                                       : (row.enabled ? theme::TEXT : theme::TEXT_MUTED)));
        }

        // The lines are centred together in the row and stop before the arrow.
        const float textLeft  = iconMin.x + theme::ICON_BOX_SIZE + textGap;
        const float textRight = max.x - arrowRight - arrowWidth - textGap;
        struct Line
        {
            const char* text;
            float size;
            unsigned color;
            bool medium;
        };
        const Line lines[] = {
            {row.title, theme::FONT_ROW_TITLE, row.enabled ? theme::TEXT : theme::TEXT_MUTED, true},
            {row.subtitle, theme::FONT_SMALL, theme::TEXT_MUTED, false},
            {row.detail, theme::FONT_PATH, theme::TEXT_PATH, false},
        };
        float height = 0.0f;
        for (const Line& line : lines)
        {
            if (line.text && line.text[0] != '\0')
            {
                height += (height > 0.0f ? lineGap : 0.0f) +
                          textSize(line.text, line.size, line.medium).y;
            }
        }
        float y = min.y + (theme::LIST_ROW_HEIGHT - height) * 0.5f;
        for (const Line& line : lines)
        {
            if (!line.text || line.text[0] == '\0')
            {
                continue;
            }
            drawTextEllipsis(drawList, fontFor(line.medium), line.size, ImVec2(textLeft, y),
                theme::rgb(line.color), line.text, textRight - textLeft);
            y += textSize(line.text, line.size, line.medium).y + lineGap;
        }

        drawChevron(drawList,
            ImVec2(max.x - arrowRight - arrowWidth, min.y + (theme::LIST_ROW_HEIGHT - 22.0f) * 0.5f),
            theme::rgb(theme::TEXT_MUTED));
        return touched;
    }

    bool drawDashedButton(
        const char* id, const char* label, const ImVec2& size, bool stacked, float rounding)
    {
        constexpr float thickness = 3.0f;
        constexpr float plusGap   = 12.0f;

        const ImVec2 min        = ImGui::GetCursorScreenPos();
        const ImVec2 available  = ImGui::GetContentRegionAvail();
        const ImVec2 buttonSize(size.x > 0.0f ? size.x : available.x + size.x,
            size.y > 0.0f ? size.y : available.y + size.y);
        const bool touched      = ImGui::InvisibleButton(id, buttonSize);
        const ImVec2 max(min.x + buttonSize.x, min.y + buttonSize.y);

        // Straight dashes along each side, and whole arcs at the corners.
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImU32 color    = theme::rgb(theme::DASHED);
        const float inset    = thickness * 0.5f;
        const float l        = min.x + inset;
        const float t        = min.y + inset;
        const float r        = max.x - inset;
        const float b        = max.y - inset;
        const float radius   = std::min(rounding, std::min(r - l, b - t) * 0.5f);
        drawDashedLine(drawList, ImVec2(l + radius, t), ImVec2(r - radius, t), color, thickness);
        drawDashedLine(drawList, ImVec2(r, t + radius), ImVec2(r, b - radius), color, thickness);
        drawDashedLine(drawList, ImVec2(r - radius, b), ImVec2(l + radius, b), color, thickness);
        drawDashedLine(drawList, ImVec2(l, b - radius), ImVec2(l, t + radius), color, thickness);
        const struct
        {
            ImVec2 center;
            float from;
        } corners[] = {
            {ImVec2(l + radius, t + radius), PI},
            {ImVec2(r - radius, t + radius), PI * 1.5f},
            {ImVec2(r - radius, b - radius), 0.0f},
            {ImVec2(l + radius, b - radius), PI * 0.5f},
        };
        for (const auto& corner : corners)
        {
            drawList->PathArcTo(corner.center, radius, corner.from, corner.from + PI * 0.5f, 12);
            drawList->PathStroke(color, ImDrawFlags_None, thickness);
        }

        const float plusSizePx  = stacked ? 36.0f : 28.0f;
        const ImVec2 plusSize   = textSize("+", plusSizePx);
        const ImVec2 labelSize  = textSize(label, theme::FONT_BODY, true);
        const ImU32 textColor   = theme::rgb(theme::TEXT_SOFT);
        const float centerX     = (min.x + max.x) * 0.5f;
        const float centerY     = (min.y + max.y) * 0.5f;
        if (stacked)
        {
            const float gap    = 10.0f;
            const float top    = centerY - (plusSize.y + gap + labelSize.y) * 0.5f;
            drawText(drawList, ImVec2(centerX - plusSize.x * 0.5f, top), plusSizePx, textColor, "+");
            drawText(drawList, ImVec2(centerX - labelSize.x * 0.5f, top + plusSize.y + gap),
                theme::FONT_BODY, textColor, label, nullptr, true);
        }
        else
        {
            const float left = centerX - (plusSize.x + plusGap + labelSize.x) * 0.5f;
            drawText(drawList, ImVec2(left, centerY - plusSize.y * 0.5f), plusSizePx, textColor, "+");
            drawText(drawList, ImVec2(left + plusSize.x + plusGap, centerY - labelSize.y * 0.5f),
                theme::FONT_BODY, textColor, label, nullptr, true);
        }
        return touched;
    }

    void drawStatus(
        ImDrawList* drawList, const ImVec2& bottomLeft, const char* text, unsigned dotColor)
    {
        constexpr float dotSize = 8.0f;
        constexpr float gap     = 6.0f;
        const ImVec2 size       = textSize(text, theme::FONT_PATH);
        const float height      = std::max(size.y, dotSize);
        const float top         = bottomLeft.y - height;
        drawList->AddCircleFilled(ImVec2(bottomLeft.x + dotSize * 0.5f, top + height * 0.5f),
            dotSize * 0.5f, theme::rgb(dotColor), 12);
        drawText(drawList, ImVec2(bottomLeft.x + dotSize + gap, top + (height - size.y) * 0.5f),
            theme::FONT_PATH, theme::rgb(theme::TEXT_MUTED), text);
    }
}
