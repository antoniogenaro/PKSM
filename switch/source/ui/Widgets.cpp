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

        ImFont* fontFor(bool bold)
        {
            return bold ? theme::boldFont() : ImGui::GetFont();
        }

        // Text at `position` at `size`, kept inside `clip` when given.
        void drawText(ImDrawList* drawList, const ImVec2& position, float size, ImU32 color,
            const char* text, const ImVec4* clip = nullptr, bool bold = false)
        {
            drawList->AddText(fontFor(bold), size, position, color, text, nullptr, 0.0f, clip);
        }

        ImVec2 textSize(const char* text, float size, bool bold = false)
        {
            ImGui::PushFont(bold ? theme::boldFont() : nullptr, size);
            const ImVec2 result = ImGui::CalcTextSize(text);
            ImGui::PopFont();
            return result;
        }

        // The box the visible strokes of `text` cover at `size`, as (left, top, right, bottom)
        // from where AddText starts it. Empty for text with nothing to draw.
        ImVec4 inkBounds(ImFont* font, float size, const char* text)
        {
            ImFontBaked* baked = font->GetFontBaked(size);
            ImVec4 bounds(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
            float x = 0.0f;
            for (const char* cursor = text; *cursor != '\0';)
            {
                unsigned int character = 0;
                cursor += ImTextCharFromUtf8(&character, cursor, nullptr);
                const ImFontGlyph* glyph =
                    baked ? baked->FindGlyph(static_cast<ImWchar>(character)) : nullptr;
                if (!glyph)
                {
                    continue;
                }
                if (glyph->Visible)
                {
                    bounds.x = std::min(bounds.x, x + glyph->X0);
                    bounds.y = std::min(bounds.y, glyph->Y0);
                    bounds.z = std::max(bounds.z, x + glyph->X1);
                    bounds.w = std::max(bounds.w, glyph->Y1);
                }
                x += glyph->AdvanceX;
            }
            return bounds.x <= bounds.z ? bounds : ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
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

    void text(const char* text, float size, unsigned color, bool bold)
    {
        ImGui::PushFont(bold ? theme::boldFont() : nullptr, size);
        ImGui::PushStyleColor(ImGuiCol_Text, theme::vec(color));
        ImGui::TextUnformatted(text);
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }

    namespace
    {
        // The top of `text`'s capitals at `size`, for a line whose capitals are centred on
        // `centerY`: text lines leave room for descenders, so centring the line sits it high.
        float capitalsTop(float size, float centerY)
        {
            const ImVec4 capitals = inkBounds(theme::boldFont(), size, "H");
            return std::round(centerY - (capitals.y + capitals.w) * 0.5f);
        }

        // The canvas header ball: a white ball with a black outline and a ringed button,
        // 34 pixels across, drawn from its 52 pixel source.
        void drawHeaderBall(ImDrawList* drawList, const ImVec2& center)
        {
            constexpr float scale = 34.0f / 52.0f;
            const ImU32 white     = theme::rgb(theme::ON_DARK);
            const ImU32 black     = theme::rgb(theme::HEADER_EDGE);
            drawList->AddCircleFilled(center, (22.0f + 3.0f) * scale, black, 40);
            drawList->AddCircleFilled(center, (22.0f - 3.0f) * scale, white, 40);
            drawList->AddCircleFilled(center, (9.0f + 1.5f) * scale, black, 24);
            drawList->AddCircleFilled(center, (9.0f - 1.5f) * scale, white, 24);
        }

        // The canvas battery: a 30 x 16 outline with a nub, filled as far as `percent`.
        void drawBattery(ImDrawList* drawList, const ImVec2& min, int percent)
        {
            const ImU32 white = theme::rgb(theme::ON_DARK);
            drawList->AddRect(ImVec2(min.x + 1.0f, min.y + 1.0f), ImVec2(min.x + 26.0f, min.y + 15.0f),
                white, 3.0f, ImDrawFlags_None, 2.0f);
            drawList->AddRectFilled(ImVec2(min.x + 27.0f, min.y + 5.0f),
                ImVec2(min.x + 29.5f, min.y + 11.0f), white, 1.0f);
            const float fill = 19.0f * static_cast<float>(std::clamp(percent, 0, 100)) / 100.0f;
            if (fill >= 1.0f)
            {
                drawList->AddRectFilled(ImVec2(min.x + 4.0f, min.y + 4.0f),
                    ImVec2(min.x + 4.0f + fill, min.y + 12.0f), white, 1.0f);
            }
        }
    }

    void drawHeader(const char* title, const HeaderStatus& status)
    {
        constexpr float gap        = 16.0f; // Between the parts on the left.
        constexpr float statusGap  = 24.0f; // Between the parts on the right.
        constexpr float ballSize   = 34.0f;
        constexpr float spacing    = 2.0f;  // Between the letters of "PKSM".
        constexpr float avatarSize = 32.0f;
        constexpr float avatarGap  = 10.0f;
        constexpr int avatarIcon   = 18;
        constexpr float batteryW   = 30.0f;
        constexpr float batteryH   = 16.0f;
        constexpr float batteryGap = 8.0f;
        static constexpr char name[] = "PKSM";

        const ImGuiWindow* window = ImGui::GetCurrentWindowRead();
        const ImVec2 origin       = window->Pos;
        const float width         = window->Size.x;
        const float bandBottom    = origin.y + theme::HEADER_HEIGHT;
        const float centerY       = origin.y + theme::HEADER_HEIGHT * 0.5f;
        const ImU32 white         = theme::rgb(theme::ON_DARK);
        ImFont* bold              = theme::boldFont();

        // The bar spans the screen, over the window's padding.
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->PushClipRect(origin, ImVec2(origin.x + width, origin.y + window->Size.y), false);
        drawList->AddRectFilled(origin, ImVec2(origin.x + width, bandBottom), theme::rgb(theme::HEADER));
        drawList->AddRectFilled(ImVec2(origin.x, bandBottom),
            ImVec2(origin.x + width, bandBottom + theme::HEADER_EDGE_SIZE),
            theme::rgb(theme::HEADER_EDGE));

        // Left: the ball, "PKSM", a divider and the screen's name.
        float x = origin.x + theme::SCREEN_PADDING_X;
        drawHeaderBall(drawList, ImVec2(x + ballSize * 0.5f, centerY));
        x += ballSize + gap;
        const float nameY = capitalsTop(theme::FONT_TITLE, centerY);
        for (const char* letter = name; *letter != '\0'; ++letter)
        {
            const char glyph[2] = {*letter, '\0'};
            drawText(drawList, ImVec2(x, nameY), theme::FONT_TITLE, white, glyph, nullptr, true);
            x += textSize(glyph, theme::FONT_TITLE, true).x + spacing;
        }
        x += gap - spacing;
        drawList->AddRectFilled(ImVec2(x, centerY - 12.0f), ImVec2(x + 2.0f, centerY + 12.0f),
            theme::rgb(theme::ON_DARK, 128));
        x += 2.0f + gap;
        const float titleLeft = x;

        // Right, from the edge inwards: the battery, the time and the user.
        float right       = origin.x + width - theme::SCREEN_PADDING_X;
        const float textY = capitalsTop(theme::FONT_BODY, centerY);
        if (status.battery >= 0)
        {
            const std::string percent = std::to_string(status.battery) + "%";
            const float textWidth     = textSize(percent.c_str(), theme::FONT_BODY, true).x;
            drawText(drawList, ImVec2(right - textWidth, textY), theme::FONT_BODY, white,
                percent.c_str(), nullptr, true);
            right -= textWidth + batteryGap + batteryW;
            drawBattery(drawList, ImVec2(right, std::round(centerY - batteryH * 0.5f)),
                status.battery);
            right -= statusGap;
        }
        if (!status.time.empty())
        {
            const float textWidth = textSize(status.time.c_str(), theme::FONT_BODY, true).x;
            right -= textWidth;
            drawText(drawList, ImVec2(right, textY), theme::FONT_BODY, white, status.time.c_str(),
                nullptr, true);
            right -= statusGap;
        }
        if (!status.user.empty())
        {
            // The name gives way to the screen's name when both cannot fit.
            constexpr float maxUser = 220.0f;
            const float userWidth =
                std::min(maxUser, textSize(status.user.c_str(), theme::FONT_BODY, true).x);
            right -= userWidth;
            drawTextEllipsis(drawList, bold, theme::FONT_BODY, ImVec2(right, textY), white,
                status.user.c_str(), userWidth);
            right -= avatarGap + avatarSize;
            const ImVec2 avatarCenter(right + avatarSize * 0.5f, centerY);
            drawList->AddCircleFilled(avatarCenter, avatarSize * 0.5f, theme::rgb(theme::HEADER_EDGE), 32);
            drawList->AddCircleFilled(avatarCenter, avatarSize * 0.5f - 2.0f, white, 32);
            icons::drawSized(drawList, icons::Icon::Trainer,
                ImVec2(std::round(avatarCenter.x - avatarIcon * 0.5f),
                    std::round(avatarCenter.y - avatarIcon * 0.5f)),
                avatarIcon, theme::rgb(theme::TEXT));
            right -= statusGap;
        }

        if (title && title[0] != '\0')
        {
            drawTextEllipsis(drawList, bold, theme::FONT_SUBTITLE,
                ImVec2(titleLeft, capitalsTop(theme::FONT_SUBTITLE, centerY)), white, title,
                right - titleLeft);
        }
        drawList->PopClipRect();

        // Content starts under the strip, after the screen's own sectionGap().
        ImGui::SetCursorScreenPos(
            ImVec2(origin.x + window->WindowPadding.x, bandBottom + theme::HEADER_EDGE_SIZE));
        ImGui::Dummy(ImVec2(0.0f, 0.0f));
    }

    float drawButtonLegend(std::initializer_list<ButtonHint> hints)
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

        // Text is centred on its strokes, not on its line: the line leaves room below for
        // descenders, which button names never have, so they would sit off centre. Labels are
        // centred on the capitals, so they share the button names' baseline.
        ImFont* font            = theme::boldFont();
        const float centerY     = top + faceSize * 0.5f;
        const ImVec4 capitals   = inkBounds(font, theme::FONT_BODY, "H");
        const float labelY      = std::round(centerY - (capitals.y + capitals.w) * 0.5f);
        ImDrawList* drawList    = ImGui::GetWindowDrawList();
        float x                 = windowPos.x + windowSize.x - padding.x - total;
        for (const ButtonHint& hint : hints)
        {
            const float width     = faceWidth(hint);
            const ImVec4 ink      = inkBounds(font, theme::FONT_BODY, hint.button);
            const ImVec2 labelSize = textSize(hint.label, theme::FONT_BODY, true);
            drawList->AddRectFilled(ImVec2(x, top), ImVec2(x + width, top + faceSize),
                theme::rgb(theme::BUTTON_FACE), faceSize * 0.5f);
            const ImVec2 buttonPosition(std::round(x + width * 0.5f - (ink.x + ink.z) * 0.5f),
                std::round(centerY - (ink.y + ink.w) * 0.5f));
            drawText(drawList, buttonPosition, theme::FONT_BODY, theme::rgb(theme::ON_DARK),
                hint.button, nullptr, true);
            x += width + faceGap;
            drawText(drawList, ImVec2(std::round(x), labelY), theme::FONT_BODY,
                theme::rgb(theme::TEXT_SOFT), hint.label, nullptr, true);
            x += labelSize.x + hintGap;
        }
        return total;
    }

    void drawFooterMessage(const std::string& text, unsigned color, float maxWidth)
    {
        if (text.empty() || maxWidth <= 0.0f)
        {
            return;
        }
        const ImGuiWindow* window = ImGui::GetCurrentWindowRead();
        const ImVec2 padding      = window->WindowPadding;
        const float centerY =
            window->Pos.y + window->Size.y - padding.y - theme::LEGEND_HEIGHT * 0.5f;
        const float height = textSize(text.c_str(), theme::FONT_BODY, true).y;
        drawTextEllipsis(ImGui::GetWindowDrawList(), theme::boldFont(), theme::FONT_BODY,
            ImVec2(window->Pos.x + padding.x, std::round(centerY - height * 0.5f)),
            theme::rgb(color), text.c_str(), maxWidth);
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
            text(caption, theme::FONT_CAPTION, theme::CAPTION, true);
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

    bool drawFieldCard(const char* id, const char* label, const char* value, const ImVec2& size,
        bool selected, bool editable)
    {
        // The canvas card: 16 pixels above and below, 20 at the sides, a 15 pixel label with a
        // 22 pixel pencil, and the value at 26 pixels.
        constexpr float paddingX  = 20.0f;
        constexpr float paddingY  = 16.0f;
        constexpr int pencilSize  = 22;
        constexpr float pencilGap = 12.0f;
        constexpr float labelSize = 15.0f;
        constexpr float valueSize = 26.0f;

        const ImVec2 min   = ImGui::GetCursorScreenPos();
        const bool touched = ImGui::InvisibleButton(id, size);
        const ImVec2 max(min.x + size.x, min.y + size.y);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(min, max,
            theme::rgb(selected ? theme::CARD_SELECTED : theme::CARD), theme::ROW_RADIUS);
        if (selected)
        {
            drawSelectionOutline(drawList, min, max, theme::ROW_RADIUS);
        }

        const ImU32 muted     = theme::rgb(theme::TEXT_MUTED);
        const float left      = min.x + paddingX;
        const float right     = max.x - paddingX;
        const float labelTop  = min.y + paddingY;
        const float labelHigh = textSize(label, labelSize, true).y;
        const float labelEnd  = editable ? right - pencilSize - pencilGap : right;
        drawTextEllipsis(drawList, theme::boldFont(), labelSize, ImVec2(left, labelTop), muted,
            label, labelEnd - left);
        if (editable)
        {
            icons::drawSized(drawList, icons::Icon::Edit,
                ImVec2(right - pencilSize, labelTop + (labelHigh - pencilSize) * 0.5f), pencilSize,
                muted);
        }

        const float valueHigh = textSize(value, valueSize, true).y;
        drawTextEllipsis(drawList, theme::boldFont(), valueSize,
            ImVec2(left, max.y - paddingY - valueHigh), theme::rgb(theme::TEXT), value,
            right - left);
        return touched;
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
        icons::draw(drawList, row.icon, iconMin, theme::ICON_BOX_SIZE,
            theme::rgb(highlighted ? theme::ON_DARK
                                   : (row.enabled ? theme::TEXT : theme::TEXT_MUTED)));

        // The lines are centred together in the row and stop before the arrow.
        const float textLeft  = iconMin.x + theme::ICON_BOX_SIZE + textGap;
        const float textRight = max.x - arrowRight - arrowWidth - textGap;
        struct Line
        {
            const char* text;
            float size;
            unsigned color;
            bool bold;
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
                          textSize(line.text, line.size, line.bold).y;
            }
        }
        float y = min.y + (theme::LIST_ROW_HEIGHT - height) * 0.5f;
        for (const Line& line : lines)
        {
            if (!line.text || line.text[0] == '\0')
            {
                continue;
            }
            drawTextEllipsis(drawList, fontFor(line.bold), line.size, ImVec2(textLeft, y),
                theme::rgb(line.color), line.text, textRight - textLeft);
            y += textSize(line.text, line.size, line.bold).y + lineGap;
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
        // The game grid's "+" is in the accent; elsewhere it matches the label.
        const ImU32 plusColor   = stacked ? theme::rgb(theme::ACCENT) : textColor;
        const float centerX     = (min.x + max.x) * 0.5f;
        const float centerY     = (min.y + max.y) * 0.5f;
        if (stacked)
        {
            const float gap    = 10.0f;
            const float top    = centerY - (plusSize.y + gap + labelSize.y) * 0.5f;
            drawText(drawList, ImVec2(centerX - plusSize.x * 0.5f, top), plusSizePx, plusColor, "+");
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

    void drawStatus(ImDrawList* drawList, const ImVec2& bottomLeft, const char* text,
        unsigned dotColor, bool filled)
    {
        constexpr float dotSize = 8.0f;
        constexpr float gap     = 6.0f;
        const ImVec2 size       = textSize(text, theme::FONT_PATH);
        const float height      = std::max(size.y, dotSize);
        const float top         = bottomLeft.y - height;
        const ImVec2 center(bottomLeft.x + dotSize * 0.5f, top + height * 0.5f);
        if (filled)
        {
            drawList->AddCircleFilled(center, dotSize * 0.5f, theme::rgb(dotColor), 12);
        }
        else
        {
            // A 2 pixel ring inside the dot's 8 pixels.
            drawList->AddCircle(center, dotSize * 0.5f - 1.0f, theme::rgb(dotColor), 12, 2.0f);
        }
        drawText(drawList, ImVec2(bottomLeft.x + dotSize + gap, top + (height - size.y) * 0.5f),
            theme::FONT_PATH, theme::rgb(theme::TEXT_MUTED), text);
    }
}
