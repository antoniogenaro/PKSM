#include "ui/Theme.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <vector>

namespace pksm::switchui::theme
{
    void applyStyle(ImGuiStyle& style)
    {
        ImGui::StyleColorsDark(&style);

        style.FontSizeBase      = FONT_BODY;
        style.WindowPadding     = ImVec2(PANEL_PADDING, PANEL_PADDING);
        style.FramePadding      = ImVec2(12.0f, 8.0f);
        style.ItemSpacing       = ImVec2(10.0f, ITEM_GAP);
        style.ItemInnerSpacing  = ImVec2(8.0f, 6.0f);
        style.ScrollbarSize     = 10.0f;
        style.WindowRounding    = 0.0f;
        style.ChildRounding     = CARD_RADIUS;
        style.FrameRounding     = CARD_RADIUS;
        style.PopupRounding     = CARD_RADIUS;
        style.ScrollbarRounding = 5.0f;
        style.GrabRounding      = 5.0f;
        style.WindowBorderSize  = 0.0f;
        style.ChildBorderSize   = 1.0f;
        style.FrameBorderSize   = 0.0f;

        ImVec4* colors                     = style.Colors;
        colors[ImGuiCol_Text]              = vec(TEXT);
        colors[ImGuiCol_TextDisabled]      = vec(TEXT_MUTED);
        // The screen background is cleared by SDL, so the main window draws none.
        colors[ImGuiCol_WindowBg]          = vec(BACKGROUND, 0.0f);
        colors[ImGuiCol_ChildBg]           = vec(PANEL);
        colors[ImGuiCol_PopupBg]           = vec(PANEL);
        colors[ImGuiCol_Border]            = vec(BORDER);
        colors[ImGuiCol_BorderShadow]      = vec(BACKGROUND, 0.0f);
        colors[ImGuiCol_FrameBg]           = vec(CARD);
        colors[ImGuiCol_FrameBgHovered]    = vec(CARD_SELECTED);
        colors[ImGuiCol_FrameBgActive]     = vec(CARD_SELECTED);
        colors[ImGuiCol_Button]            = vec(CARD);
        colors[ImGuiCol_ButtonHovered]     = vec(CARD_SELECTED);
        colors[ImGuiCol_ButtonActive]      = vec(CARD_SELECTED);
        colors[ImGuiCol_Header]            = vec(CARD_SELECTED);
        colors[ImGuiCol_HeaderHovered]     = vec(CARD);
        colors[ImGuiCol_HeaderActive]      = vec(CARD_SELECTED);
        colors[ImGuiCol_Separator]         = vec(BORDER);
        colors[ImGuiCol_ScrollbarBg]       = vec(BACKGROUND, 0.0f);
        colors[ImGuiCol_ScrollbarGrab]     = vec(BORDER);
        colors[ImGuiCol_ScrollbarGrabHovered] = vec(DASHED);
        colors[ImGuiCol_ScrollbarGrabActive]  = vec(DASHED);
        colors[ImGuiCol_CheckMark]         = vec(ACCENT);
        colors[ImGuiCol_NavCursor]         = vec(ACCENT);
    }

    namespace
    {
        ImFont* bold = nullptr;

        ImFont* loadFontFile(const char* path)
        {
            // Read the file here so a missing font is an error message, not a failed assertion.
            std::ifstream file(path, std::ios::binary);
            if (!file)
            {
                return nullptr;
            }
            const std::vector<char> data(
                (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            if (file.bad() || data.size() < 12)
            {
                return nullptr;
            }

            // The atlas keeps and frees the copy, and reads it again whenever it needs a new
            // size.
            void* fontData = IM_ALLOC(data.size());
            std::copy(data.begin(), data.end(), static_cast<char*>(fontData));
            ImFontConfig config;
            config.FontDataOwnedByAtlas = true;
            return ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
                fontData, static_cast<int>(data.size()), FONT_BODY, &config);
        }
    }

    bool loadFonts(const char*& failedPath)
    {
        // The first font added is ImGui's default, so the regular one goes first.
        if (!loadFontFile(FONT_REGULAR_PATH))
        {
            failedPath = FONT_REGULAR_PATH;
            return false;
        }
        bold = loadFontFile(FONT_BOLD_PATH);
        if (!bold)
        {
            failedPath = FONT_BOLD_PATH;
            return false;
        }
        return true;
    }

    ImFont* boldFont()
    {
        return bold ? bold : ImGui::GetFont();
    }
}
