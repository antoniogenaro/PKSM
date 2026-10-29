#include "SwitchUi.hpp"
#include "domain/GameGrid.hpp"
#include "platform/SaveStorage.hpp"
#include "ui/Icons.hpp"
#include "ui/Text.hpp"
#include "ui/Theme.hpp"
#include "ui/Widgets.hpp"

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"

#include <switch.h>
#include <algorithm>
#include <cfloat>
#include <cstdio>
#include <iterator>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

namespace pksm::switchui
{
    namespace
    {
        constexpr int SCREEN_WIDTH = 1280;
        constexpr int SCREEN_HEIGHT = 720;
        constexpr ImVec4 CARD_COLOR      = theme::vec(theme::CARD);
        constexpr ImVec4 SELECTED_COLOR  = theme::vec(theme::CARD_SELECTED);
        constexpr ImVec4 ACCENT_COLOR    = theme::vec(theme::ACCENT);
        constexpr ImVec4 ERROR_COLOR     = theme::vec(theme::ERROR_TEXT);
        constexpr ImVec4 HAS_SAVES_COLOR = theme::vec(theme::POSITIVE);
        constexpr ImVec4 NO_SAVES_COLOR  = theme::vec(theme::NEUTRAL);
        constexpr char POKEMON_SPRITESHEET_PATH[] = "romfs:/sprites/pkm_spritesheet.t3x";
        constexpr char BALL_SPRITESHEET_PATH[] = "romfs:/sprites/types_spritesheet.t3x";
        constexpr size_t BALL_SPRITE_BASE = 144;
        constexpr size_t MAX_BALL_SPRITE = 25;

        // Box and editor colours from the canvas, as 0xRRGGBB.
        constexpr unsigned EMPTY_SLOT = theme::EMPTY_SLOT;
        constexpr unsigned DETAIL_ART = theme::ART;
        constexpr unsigned PILL       = theme::CONTROL;
        constexpr unsigned MALE       = 0x2F6FD6;
        constexpr unsigned FEMALE     = 0xD6407A;
        constexpr unsigned SHINY      = theme::GOLD;
        constexpr unsigned RAISED     = theme::RAISED;
        constexpr unsigned LOWERED    = theme::LOWERED;

        // The usual colour of each type, in the order of pksm::Type.
        constexpr unsigned TYPE_COLORS[] = {0xA8A77A, 0xC22E28, 0xA98FF3, 0xA33EA1, 0xE2BF65,
            0xB6A136, 0xA6B91A, 0x735797, 0xB7B7CE, 0xEE8130, 0x6390F0, 0x7AC74C, 0xF7D02C,
            0xF95587, 0x96D9D6, 0x6F35FC, 0x705746, 0xD685AD};

        unsigned typeColor(size_t type)
        {
            return type < std::size(TYPE_COLORS) ? TYPE_COLORS[type] : theme::NEUTRAL;
        }

        // The lighter ring around a type's dot: its colour halfway to white.
        ImU32 typeRing(size_t type)
        {
            const unsigned color = typeColor(type);
            const auto lighten   = [](unsigned channel) { return (channel + 255u) / 2u; };
            return IM_COL32(lighten((color >> 16) & 0xFF), lighten((color >> 8) & 0xFF),
                lighten(color & 0xFF), 255);
        }

        // Type pills from `origin`, left to right: a dot in the type's colour and its name.
        void drawTypePills(ImDrawList* drawList, const ImVec2& origin,
            const std::vector<std::pair<std::string, size_t>>& types)
        {
            constexpr float height = 32.0f;
            ImFont* bold           = theme::boldFont();
            float x                = origin.x;
            for (const auto& [name, type] : types)
            {
                const ImVec2 text = bold->CalcTextSizeA(theme::FONT_SMALL, FLT_MAX, 0.0f, name.c_str());
                const float width = 4.0f + 24.0f + 8.0f + text.x + 14.0f;
                drawList->AddRectFilled(ImVec2(x, origin.y), ImVec2(x + width, origin.y + height),
                    theme::rgb(PILL), height * 0.5f);
                const ImVec2 dot(x + 16.0f, origin.y + height * 0.5f);
                drawList->AddCircleFilled(dot, 12.0f, typeRing(type), 24);
                drawList->AddCircleFilled(dot, 7.0f, theme::rgb(typeColor(type)), 24);
                drawList->AddText(bold, theme::FONT_SMALL,
                    ImVec2(x + 36.0f, origin.y + (height - text.y) * 0.5f),
                    theme::rgb(theme::TEXT_SOFT), name.c_str());
                x += width + 8.0f;
            }
        }

        // The canvas dialogs over a dimmed screen.
        constexpr float MODAL_RADIUS  = 24.0f;
        constexpr float MODAL_PADDING = 20.0f;
        constexpr float MODAL_LEGEND_GAP = 14.0f; // Between a dialog's content and its legend.

        // The space under the content panels: the button legend and the gap above it.
        constexpr float SCREEN_BOTTOM_SPACE = theme::LEGEND_HEIGHT + theme::SECTION_GAP;

        // Leaves theme::SECTION_GAP between the last item and the next one.
        void sectionGap()
        {
            ImGui::Dummy(
                ImVec2(0.0f, theme::SECTION_GAP - ImGui::GetStyle().ItemSpacing.y * 2.0f));
        }

        bool digitsAt(const std::string& text, size_t first, size_t count)
        {
            for (size_t i = first; i < first + count; ++i)
            {
                if (text[i] < '0' || text[i] > '9')
                {
                    return false;
                }
            }
            return true;
        }

        // The folder of a backup as the save list shows it. Exported backups are named
        // "<trainer> - YYYY-MM-DD_HH-MM-SS", which becomes "<trainer> · YYYY-MM-DD HH:MM";
        // other folder names are shown as they are.
        std::string backupLabel(const std::string& folderPath)
        {
            const size_t slash = folderPath.find_last_of('/');
            const std::string folder =
                slash == std::string::npos ? folderPath : folderPath.substr(slash + 1);
            const size_t dash = folder.rfind(" - ");
            if (dash == std::string::npos)
            {
                return folder;
            }
            const std::string stamp = folder.substr(dash + 3);
            if (stamp.size() != 19 || stamp[4] != '-' || stamp[7] != '-' || stamp[10] != '_' ||
                stamp[13] != '-' || stamp[16] != '-' || !digitsAt(stamp, 0, 4) ||
                !digitsAt(stamp, 5, 2) || !digitsAt(stamp, 8, 2) || !digitsAt(stamp, 11, 2) ||
                !digitsAt(stamp, 14, 2) || !digitsAt(stamp, 17, 2))
            {
                return folder;
            }
            return folder.substr(0, dash) + " · " + stamp.substr(0, 10) + " " +
                   stamp.substr(11, 2) + ":" + stamp.substr(14, 2);
        }

        std::string saveCountText(size_t count)
        {
            if (count == 0)
            {
                return tr(Text::SaveCountNone);
            }
            return count == 1 ? std::string(tr(Text::SaveCountOne))
                              : tr(Text::SaveCountMany, {std::to_string(count)});
        }

        // What a game card says about its saves: the console save first, then the backups.
        std::string saveStatusText(const GameCard& game)
        {
            if (!game.consoleSave)
            {
                return saveCountText(game.saveCount);
            }
            if (game.saveCount == 0)
            {
                return tr(Text::ConsoleSaveStatus);
            }
            return std::string(tr(Text::ConsoleSaveStatus)) + " · " + saveCountText(game.saveCount);
        }

        bool hasSaves(const GameCard& game)
        {
            return game.consoleSave || game.saveCount > 0;
        }

        bool readU16(const std::vector<u8>& data, size_t offset, u16& result)
        {
            if (offset > data.size() || data.size() - offset < 2)
            {
                return false;
            }
            result = static_cast<u16>(data[offset]) |
                     static_cast<u16>(static_cast<u16>(data[offset + 1]) << 8);
            return true;
        }

        bool decodeLz11(
            const std::vector<u8>& input, size_t offset, size_t outputSize, std::vector<u8>& output)
        {
            if (offset > input.size() || input.size() - offset < 4 || input[offset] != 0x11)
            {
                return false;
            }
            size_t cursor = offset + 1;
            size_t encodedSize = static_cast<size_t>(input[cursor]) |
                                 (static_cast<size_t>(input[cursor + 1]) << 8) |
                                 (static_cast<size_t>(input[cursor + 2]) << 16);
            cursor += 3;
            if (encodedSize == 0)
            {
                if (input.size() - cursor < 4)
                {
                    return false;
                }
                encodedSize = static_cast<size_t>(input[cursor]) |
                              (static_cast<size_t>(input[cursor + 1]) << 8) |
                              (static_cast<size_t>(input[cursor + 2]) << 16) |
                              (static_cast<size_t>(input[cursor + 3]) << 24);
                cursor += 4;
            }
            if (encodedSize != outputSize)
            {
                return false;
            }

            output.resize(outputSize);
            size_t written = 0;
            while (written < outputSize)
            {
                if (cursor >= input.size())
                {
                    return false;
                }
                u8 flags = input[cursor++];
                for (int bit = 7; bit >= 0 && written < outputSize; --bit, flags <<= 1)
                {
                    if ((flags & 0x80) == 0)
                    {
                        if (cursor >= input.size())
                        {
                            return false;
                        }
                        output[written++] = input[cursor++];
                        continue;
                    }

                    if (cursor >= input.size())
                    {
                        return false;
                    }
                    const u8 first = input[cursor++];
                    size_t length = 0;
                    size_t distance = 0;
                    const u8 lengthType = first >> 4;
                    if (lengthType == 0)
                    {
                        if (input.size() - cursor < 2)
                        {
                            return false;
                        }
                        const u8 second = input[cursor++];
                        const u8 third = input[cursor++];
                        length = ((static_cast<size_t>(first & 0x0F) << 4) | (second >> 4)) + 0x11;
                        distance = ((static_cast<size_t>(second & 0x0F) << 8) | third) + 1;
                    }
                    else if (lengthType == 1)
                    {
                        if (input.size() - cursor < 3)
                        {
                            return false;
                        }
                        const u8 second = input[cursor++];
                        const u8 third = input[cursor++];
                        const u8 fourth = input[cursor++];
                        length = ((static_cast<size_t>(first & 0x0F) << 12) |
                                     (static_cast<size_t>(second) << 4) | (third >> 4)) +
                                 0x111;
                        distance = ((static_cast<size_t>(third & 0x0F) << 8) | fourth) + 1;
                    }
                    else
                    {
                        if (cursor >= input.size())
                        {
                            return false;
                        }
                        length = lengthType + 1;
                        distance = ((static_cast<size_t>(first & 0x0F) << 8) | input[cursor++]) + 1;
                    }

                    if (distance > written)
                    {
                        return false;
                    }
                    length = std::min(length, outputSize - written);
                    for (size_t i = 0; i < length; ++i)
                    {
                        output[written] = output[written - distance];
                        ++written;
                    }
                }
            }
            return true;
        }
    }

    SwitchUi::~SwitchUi()
    {
        if (imguiInitialized_)
        {
            ImGui_ImplSDLRenderer2_Shutdown();
            ImGui_ImplSDL2_Shutdown();
            ImGui::DestroyContext();
            imguiInitialized_ = false;
        }
        if (pokemonSpritesheet_)
        {
            SDL_DestroyTexture(pokemonSpritesheet_);
        }
        if (ballSpritesheet_)
        {
            SDL_DestroyTexture(ballSpritesheet_);
        }
        // The icon textures belong to the renderer, so they go first.
        icons::unload();
        if (renderer_)
        {
            SDL_DestroyRenderer(renderer_);
        }
        if (window_)
        {
            SDL_DestroyWindow(window_);
        }
        SDL_Quit();
    }

    bool SwitchUi::initialize(std::string& error)
    {
        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        padInitializeDefault(&pad_);
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            error = tr(Text::SdlInitFailed, {SDL_GetError()});
            return false;
        }

        window_ = SDL_CreateWindow(
            "PKSM Switch", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_FULLSCREEN);
        if (!window_)
        {
            error = tr(Text::WindowFailed, {SDL_GetError()});
            return false;
        }
        renderer_ = SDL_CreateRenderer(
            window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer_)
        {
            renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
        }
        if (!renderer_)
        {
            error = tr(Text::RendererFailed, {SDL_GetError()});
            return false;
        }
        if (!loadSpriteSheet(POKEMON_SPRITESHEET_PATH, tr(Text::PokemonSheet), pokemonSpritesheet_,
                pokemonSprites_, error) ||
            !loadSpriteSheet(BALL_SPRITESHEET_PATH, tr(Text::BallSheet), ballSpritesheet_, ballSprites_,
                error))
        {
            return false;
        }
        if (ballSprites_.size() <= BALL_SPRITE_BASE + MAX_BALL_SPRITE)
        {
            error = tr(Text::BallSheetIncomplete);
            return false;
        }
        const char* failedIcon = nullptr;
        if (!icons::load(renderer_, failedIcon))
        {
            error = tr(Text::IconLoadFailed, {failedIcon});
            return false;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        theme::applyStyle(ImGui::GetStyle());
        const char* failedFont = nullptr;
        if (!theme::loadFonts(failedFont))
        {
            error = tr(Text::FontLoadFailed, {failedFont});
            ImGui::DestroyContext();
            return false;
        }
        if (!ImGui_ImplSDL2_InitForSDLRenderer(window_, renderer_))
        {
            error = tr(Text::ImGuiPlatformFailed);
            ImGui::DestroyContext();
            return false;
        }
        if (!ImGui_ImplSDLRenderer2_Init(renderer_))
        {
            error = tr(Text::ImGuiRendererFailed);
            ImGui_ImplSDL2_Shutdown();
            ImGui::DestroyContext();
            return false;
        }
        imguiInitialized_ = true;

        return true;
    }

    bool SwitchUi::loadSpriteSheet(const char* path, const std::string& name,
        SDL_Texture*& sheetTexture, std::vector<PokemonSprite>& sheetSprites, std::string& error)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            error = tr(Text::SheetOpenFailed, {name});
            return false;
        }
        const std::vector<u8> data(
            (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (file.bad() || data.size() < 5)
        {
            error = tr(Text::SheetIncomplete, {name});
            return false;
        }

        u16 spriteCount = 0;
        if (!readU16(data, 0, spriteCount) || spriteCount == 0 || spriteCount > 2048)
        {
            error = tr(Text::SheetBadHeader, {name});
            return false;
        }
        const u8 dimensions = data[2];
        const size_t width = size_t{1} << ((dimensions & 0x07) + 3);
        const size_t height = size_t{1} << (((dimensions >> 3) & 0x07) + 3);
        if ((dimensions & 0x40) != 0 || data[3] != 0 || data[4] != 0)
        {
            error = tr(Text::SheetUnsupported, {name});
            return false;
        }

        constexpr size_t tableOffset = 5;
        if (spriteCount > (data.size() - tableOffset) / 12)
        {
            error = tr(Text::SheetTableIncomplete, {name});
            return false;
        }
        const size_t textureOffset = tableOffset + static_cast<size_t>(spriteCount) * 12;
        const size_t textureSize = width * height * 4;
        std::vector<u8> textureData;
        if (!decodeLz11(data, textureOffset, textureSize, textureData))
        {
            error = tr(Text::SheetDecompressFailed, {name});
            return false;
        }

        std::vector<PokemonSprite> sprites;
        sprites.reserve(spriteCount);
        for (size_t i = 0; i < spriteCount; ++i)
        {
            const size_t offset = tableOffset + i * 12;
            u16 spriteWidth = 0;
            u16 spriteHeight = 0;
            u16 left = 0;
            u16 top = 0;
            u16 right = 0;
            u16 bottom = 0;
            if (!readU16(data, offset, spriteWidth) ||
                !readU16(data, offset + 2, spriteHeight) ||
                !readU16(data, offset + 4, left) || !readU16(data, offset + 6, top) ||
                !readU16(data, offset + 8, right) || !readU16(data, offset + 10, bottom))
            {
                error = tr(Text::SheetTableIncomplete, {name});
                return false;
            }

            const auto toPixelX = [width](u16 coordinate)
            {
                return static_cast<int>(static_cast<size_t>(coordinate) * width / 1024);
            };
            const auto toPixelY = [height](u16 coordinate)
            {
                return static_cast<int>(static_cast<size_t>(coordinate) * height / 1024);
            };
            const bool rotated = top < bottom;
            const int sourceX = rotated ? toPixelX(top) : toPixelX(left);
            const int sourceY =
                static_cast<int>(height) - toPixelY(rotated ? right : top);
            const int sourceWidth =
                rotated ? toPixelX(bottom) - toPixelX(top) : toPixelX(right) - toPixelX(left);
            const int sourceHeight =
                rotated ? toPixelY(right) - toPixelY(left) : toPixelY(top) - toPixelY(bottom);
            if (sourceX < 0 || sourceY < 0 || sourceWidth <= 0 || sourceHeight <= 0 ||
                static_cast<size_t>(sourceX + sourceWidth) > width ||
                static_cast<size_t>(sourceY + sourceHeight) > height)
            {
                error = tr(Text::SheetBadCoordinates, {name});
                return false;
            }
            sprites.push_back(
                {{sourceX, sourceY, sourceWidth, sourceHeight}, spriteWidth, spriteHeight, rotated,
                    static_cast<int>(width), static_cast<int>(height)});
        }

        std::vector<u8> pixels(textureSize);
        const size_t tilesPerRow = width / 8;
        for (size_t y = 0; y < height; ++y)
        {
            for (size_t x = 0; x < width; ++x)
            {
                const size_t tileX = x / 8;
                const size_t tileY = y / 8;
                const size_t localX = x & 7;
                const size_t localY = y & 7;
                const size_t morton = (localX & 1) | ((localY & 1) << 1) |
                                      ((localX & 2) << 1) | ((localY & 2) << 2) |
                                      ((localX & 4) << 2) | ((localY & 4) << 3);
                const size_t source = ((tileY * tilesPerRow + tileX) * 64 + morton) * 4;
                const size_t destination = (y * width + x) * 4;
                pixels[destination] = textureData[source + 3];
                pixels[destination + 1] = textureData[source + 2];
                pixels[destination + 2] = textureData[source + 1];
                pixels[destination + 3] = textureData[source];
            }
        }

        SDL_Texture* texture = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_STATIC, static_cast<int>(width), static_cast<int>(height));
        if (!texture)
        {
            error = tr(Text::TextureCreateFailed, {name, SDL_GetError()});
            return false;
        }
        if (SDL_UpdateTexture(texture, nullptr, pixels.data(), static_cast<int>(width * 4)) != 0 ||
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND) != 0)
        {
            error = tr(Text::TextureLoadFailed, {name, SDL_GetError()});
            SDL_DestroyTexture(texture);
            return false;
        }
        sheetTexture = texture;
        sheetSprites = std::move(sprites);
        return true;
    }

    void SwitchUi::drawSprite(SDL_Texture* texture, const std::vector<PokemonSprite>& sprites,
        size_t index, size_t baseIndex, const SDL_Rect& destination)
    {
        if (!texture || index > std::numeric_limits<size_t>::max() - baseIndex)
        {
            return;
        }
        const size_t spriteIndex = baseIndex + index;
        if (spriteIndex >= sprites.size())
        {
            return;
        }

        const PokemonSprite& sprite = sprites[spriteIndex];
        const int spriteWidth = sprite.rotated ? sprite.height : sprite.width;
        const int spriteHeight = sprite.rotated ? sprite.width : sprite.height;
        if (spriteWidth <= 0 || spriteHeight <= 0)
        {
            return;
        }
        const float scale = std::min(static_cast<float>(destination.w) / spriteWidth,
            static_cast<float>(destination.h) / spriteHeight);
        const float width = static_cast<float>(spriteWidth) * scale;
        const float height = static_cast<float>(spriteHeight) * scale;
        const ImVec2 topLeft(static_cast<float>(destination.x) +
                (static_cast<float>(destination.w) - width) * 0.5f,
            static_cast<float>(destination.y) + (static_cast<float>(destination.h) - height) * 0.5f);
        const ImVec2 topRight(topLeft.x + width, topLeft.y);
        const ImVec2 bottomRight(topLeft.x + width, topLeft.y + height);
        const ImVec2 bottomLeft(topLeft.x, topLeft.y + height);
        const float u0 = static_cast<float>(sprite.source.x);
        const float v0 = static_cast<float>(sprite.source.y);
        const float u1 = static_cast<float>(sprite.source.x + sprite.source.w);
        const float v1 = static_cast<float>(sprite.source.y + sprite.source.h);
        const ImVec2 uvTopLeft = sprite.rotated ? ImVec2(u0, v1) : ImVec2(u0, v0);
        const ImVec2 uvTopRight = sprite.rotated ? ImVec2(u0, v0) : ImVec2(u1, v0);
        const ImVec2 uvBottomRight = sprite.rotated ? ImVec2(u1, v0) : ImVec2(u1, v1);
        const ImVec2 uvBottomLeft = sprite.rotated ? ImVec2(u1, v1) : ImVec2(u0, v1);

        const ImTextureRef textureRef(static_cast<void*>(texture));
        ImGui::GetWindowDrawList()->AddImageQuad(textureRef, topLeft, topRight, bottomRight,
            bottomLeft, ImVec2(uvTopLeft.x / sprite.sheetWidth, uvTopLeft.y / sprite.sheetHeight),
            ImVec2(uvTopRight.x / sprite.sheetWidth, uvTopRight.y / sprite.sheetHeight),
            ImVec2(uvBottomRight.x / sprite.sheetWidth, uvBottomRight.y / sprite.sheetHeight),
            ImVec2(uvBottomLeft.x / sprite.sheetWidth, uvBottomLeft.y / sprite.sheetHeight));
    }

    void SwitchUi::drawPokemonSprite(
        size_t index, const SDL_Rect& destination, size_t formSprite)
    {
        if (index == 0)
        {
            return;
        }
        if (formSprite != 0 && formSprite < ballSprites_.size())
        {
            drawSprite(ballSpritesheet_, ballSprites_, formSprite, 0, destination);
            return;
        }
        drawSprite(pokemonSpritesheet_, pokemonSprites_, index, 0, destination);
    }

    void SwitchUi::drawBallSprite(size_t index, const SDL_Rect& destination)
    {
        drawSprite(ballSpritesheet_, ballSprites_, std::min(index, MAX_BALL_SPRITE),
            BALL_SPRITE_BASE, destination);
    }

    void SwitchUi::beginScreens()
    {
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGui::GetIO().DisplaySize =
            ImVec2(static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT));
    }

    void SwitchUi::endScreens()
    {
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer_, (theme::BACKGROUND >> 16) & 0xFF,
            (theme::BACKGROUND >> 8) & 0xFF, theme::BACKGROUND & 0xFF, 255);
        SDL_RenderClear(renderer_);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer_);
        SDL_RenderPresent(renderer_);
    }

    void SwitchUi::beginFrame()
    {
        const ImGuiIO& io = ImGui::GetIO();
        // One full-screen window with the canvas margins; SDL clears the background. Screens
        // place their last line against the bottom margin, so it never scrolls.
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
            ImVec2(theme::SCREEN_PADDING_X, theme::SCREEN_PADDING_Y));
        ImGui::Begin("PKSM Switch", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PopStyleVar();
    }

    void SwitchUi::endFrame()
    {
        ImGui::End();
    }

    void SwitchUi::beginModal(float width, float height)
    {
        // A window above the screen below: the screen dimmed, and a rounded dialog centred.
        const ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("PKSM Modal", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                ImGuiWindowFlags_NoBackground);
        ImGui::PopStyleVar();
        // The canvas dim: black at 55% over the whole screen, its legend included.
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(ImVec2(0.0f, 0.0f), io.DisplaySize, IM_COL32(0, 0, 0, 140));

        // The dialog's own button legend sits along its bottom, under the content.
        height += MODAL_LEGEND_GAP + theme::LEGEND_HEIGHT;
        const ImVec2 min(std::round((io.DisplaySize.x - width) * 0.5f),
            std::round((io.DisplaySize.y - height) * 0.5f));
        const ImVec2 max(min.x + width, min.y + height);
        // The canvas shadow: a soft dark border around the dialog.
        for (int i = 1; i <= 6; ++i)
        {
            const float spread = static_cast<float>(i) * 4.0f;
            drawList->AddRectFilled(ImVec2(min.x - spread, min.y - spread + 12.0f),
                ImVec2(max.x + spread, max.y + spread + 12.0f), IM_COL32(0, 0, 0, 18),
                MODAL_RADIUS + spread);
        }
        drawList->AddRectFilled(min, max, theme::rgb(theme::PANEL), MODAL_RADIUS);
        drawList->AddRect(min, max, theme::rgb(theme::BORDER), MODAL_RADIUS);

        // A window the size of the dialog, so the legend lands in its bottom right corner.
        ImGui::SetCursorScreenPos(min);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(MODAL_PADDING, MODAL_PADDING));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
        ImGui::BeginChild("dialog", ImVec2(width, height), ImGuiChildFlags_AlwaysUseWindowPadding,
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PopStyleVar(2);
    }

    void SwitchUi::endModal()
    {
        ImGui::EndChild();
        ImGui::End();
    }

    void SwitchUi::drawStatusLine(bool dirty, const std::string& error)
    {
        if (dirty)
        {
            ImGui::TextColored(ACCENT_COLOR, "%s", tr(Text::PendingChanges));
        }
        if (!error.empty())
        {
            ImGui::SameLine();
            ImGui::TextColored(ERROR_COLOR, "%s", error.c_str());
        }
    }

    Action SwitchUi::pollAction()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (imguiInitialized_)
            {
                ImGui_ImplSDL2_ProcessEvent(&event);
            }
            if (event.type == SDL_QUIT)
            {
                return Action::Quit;
            }
        }

        padUpdate(&pad_);
        const u64 buttons = padGetButtonsDown(&pad_);
        // The left stick moves like the d-pad.
        if (buttons & (HidNpadButton_Up | HidNpadButton_StickLUp))
        {
            return Action::Up;
        }
        if (buttons & (HidNpadButton_Down | HidNpadButton_StickLDown))
        {
            return Action::Down;
        }
        if (buttons & (HidNpadButton_Left | HidNpadButton_StickLLeft))
        {
            return Action::Left;
        }
        if (buttons & (HidNpadButton_Right | HidNpadButton_StickLRight))
        {
            return Action::Right;
        }
        if (buttons & HidNpadButton_L)
        {
            return Action::PreviousPage;
        }
        if (buttons & HidNpadButton_R)
        {
            return Action::NextPage;
        }
        if (buttons & HidNpadButton_A)
        {
            return Action::Confirm;
        }
        if (buttons & HidNpadButton_B)
        {
            return Action::Back;
        }
        if (buttons & HidNpadButton_X)
        {
            return Action::Discard;
        }
        if (buttons & HidNpadButton_Y)
        {
            return Action::Save;
        }
        if (buttons & HidNpadButton_ZL)
        {
            return Action::PreviousTab;
        }
        if (buttons & HidNpadButton_ZR)
        {
            return Action::NextTab;
        }
        if (buttons & HidNpadButton_Minus)
        {
            return Action::Alternate;
        }
        if (buttons & HidNpadButton_Plus)
        {
            return Action::Quit;
        }

        return Action::None;
    }

    void SwitchUi::drawGameArt(const GameCard& game, const ImVec2& min, float size)
    {
        // The game's colour with its mascot on top.
        const ImVec2 max(min.x + size, min.y + size);
        ImGui::GetWindowDrawList()->AddRectFilled(min, max, theme::rgb(game.artColor),
            theme::CARD_RADIUS, ImDrawFlags_RoundCornersTop);
        const float inset = size * 0.1f;
        drawPokemonSprite(game.mascotSpecies,
            {static_cast<int>(min.x + inset), static_cast<int>(min.y + inset),
                static_cast<int>(size - inset * 2.0f), static_cast<int>(size - inset * 2.0f)});
    }

    void SwitchUi::drawGamePanel(const GameCard& game, const std::string& group,
        std::initializer_list<CardLine> lines, const std::string& message, unsigned messageColor)
    {
        constexpr float infoPadding = 14.0f;
        constexpr float lineGap     = 5.0f;
        constexpr float dotSize     = 8.0f;
        constexpr float dotGap      = 7.0f;

        widgets::beginPanel("game-panel", ImVec2(theme::SIDE_PANEL_WIDTH, -SCREEN_BOTTOM_SPACE),
            group.c_str(), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        const float width = ImGui::GetContentRegionAvail().x;
        const ImVec2 min  = ImGui::GetCursorScreenPos();

        // Measure the lines first, so the card ends right after them.
        const auto textSize = [](const std::string& text, float size, bool bold)
        {
            ImGui::PushFont(bold ? theme::boldFont() : nullptr, size);
            const ImVec2 result = ImGui::CalcTextSize(text.c_str());
            ImGui::PopFont();
            return result;
        };
        const float nameHeight = textSize(game.name, theme::FONT_HEADING, true).y;
        float infoHeight       = nameHeight;
        for (const CardLine& line : lines)
        {
            infoHeight += lineGap + textSize(line.text, line.size, line.bold).y;
        }
        const ImVec2 max(min.x + width, min.y + width + infoPadding * 2.0f + infoHeight);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(min, max, theme::rgb(theme::CARD_SELECTED), theme::CARD_RADIUS);
        drawGameArt(game, min, width);

        const float left  = min.x + infoPadding;
        const float right = max.x - infoPadding;
        float y           = min.y + width + infoPadding;
        widgets::drawTextEllipsis(drawList, theme::boldFont(), theme::FONT_HEADING,
            ImVec2(left, y), theme::rgb(theme::TEXT), game.name.c_str(), right - left);
        y += nameHeight + lineGap;
        for (const CardLine& line : lines)
        {
            const ImVec2 size = textSize(line.text, line.size, line.bold);
            float x           = left;
            if (line.dotColor != 0)
            {
                // "No saves" is a ring, as on the game grid; other dots are filled.
                const ImVec2 center(x + dotSize * 0.5f, y + size.y * 0.5f);
                if (line.dotColor == theme::NEUTRAL)
                {
                    drawList->AddCircle(
                        center, dotSize * 0.5f - 1.0f, theme::rgb(line.dotColor), 12, 2.0f);
                }
                else
                {
                    drawList->AddCircleFilled(center, dotSize * 0.5f, theme::rgb(line.dotColor), 12);
                }
                x += dotSize + dotGap;
            }
            widgets::drawTextEllipsis(drawList,
                line.bold ? theme::boldFont() : ImGui::GetFont(), line.size, ImVec2(x, y),
                theme::rgb(line.color), line.text.c_str(), right - x);
            y += size.y + lineGap;
        }
        ImGui::Dummy(ImVec2(width, max.y - min.y));

        if (!message.empty())
        {
            ImGui::Dummy(ImVec2(0.0f, 4.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, theme::vec(messageColor));
            ImGui::TextWrapped("%s", message.c_str());
            ImGui::PopStyleColor();
        }
        widgets::endPanel();
    }

    bool SwitchUi::drawGameTile(const GameCard& game, float width, float height, bool selected)
    {
        constexpr float infoPadding = 10.0f;

        const ImVec2 min   = ImGui::GetCursorScreenPos();
        const bool touched = ImGui::InvisibleButton("game-tile", ImVec2(width, height));
        const ImVec2 max(min.x + width, min.y + height);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(min, max,
            theme::rgb(selected ? theme::CARD_SELECTED : theme::CARD), theme::CARD_RADIUS);
        drawGameArt(game, min, width);

        // The name under the art, wrapping onto a second line when it must.
        const float artBottom = min.y + width;
        const ImVec4 clip(min.x, artBottom, max.x, max.y);
        drawList->AddText(theme::boldFont(), theme::FONT_BODY,
            ImVec2(min.x + infoPadding, artBottom + infoPadding), theme::rgb(theme::TEXT),
            game.name.c_str(), nullptr, width - infoPadding * 2.0f, &clip);

        // The tile is too narrow for both, so a console save hides the backup count.
        const std::string status = game.consoleSave ? std::string(tr(Text::ConsoleSaveStatus))
                                                    : saveCountText(game.saveCount);
        widgets::drawStatus(drawList, ImVec2(min.x + infoPadding, max.y - 12.0f), status.c_str(),
            hasSaves(game) ? theme::POSITIVE : theme::NEUTRAL, hasSaves(game));

        if (selected)
        {
            widgets::drawSelectionOutline(drawList, min, max, theme::CARD_RADIUS);
        }
        return touched;
    }

    void SwitchUi::drawGameList(const GameGrid& grid, const std::vector<GameCard>& games,
        size_t& selected, const std::string& error, bool canSwitchUser)
    {
        beginFrame();
        widgets::drawHeader(tr(Text::GameListPrompt), status_.current());
        if (!error.empty())
        {
            ImGui::TextColored(ERROR_COLOR, "%s", error.c_str());
        }

        // Cards fill rows of GameGrid::COLUMNS; each holds its games side by side, and a card
        // with one game keeps it at the width of a pair's.
        constexpr float cardGap    = 16.0f;
        constexpr float cardHeight = 256.0f;
        constexpr float tileGap    = 10.0f;
        const float columns        = static_cast<float>(GameGrid::COLUMNS);
        const float cardWidth =
            (ImGui::GetContentRegionAvail().x - cardGap * (columns - 1.0f)) / columns;
        const float tileWidth = (cardWidth - theme::PANEL_PADDING * 2.0f - tileGap) / 2.0f;
        const auto& cards     = grid.cards();
        // The canvas leaves 24 pixels above the grid, 8 more than the usual section gap.
        sectionGap();
        ImGui::Dummy(ImVec2(0.0f, 8.0f - ImGui::GetStyle().ItemSpacing.y));
        for (size_t c = 0; c < cards.size(); ++c)
        {
            if (c % GameGrid::COLUMNS != 0)
            {
                ImGui::SameLine(0.0f, cardGap);
            }
            else if (c != 0)
            {
                ImGui::Dummy(ImVec2(0.0f, cardGap - ImGui::GetStyle().ItemSpacing.y * 2.0f));
            }
            ImGui::PushID(static_cast<int>(c));
            widgets::beginPanel("game-card", ImVec2(cardWidth, cardHeight), tr(cards[c].group),
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            const float tileHeight = ImGui::GetContentRegionAvail().y;
            for (size_t slot = 0; slot < cards[c].games.size(); ++slot)
            {
                const size_t game = cards[c].games[slot];
                if (game >= games.size())
                {
                    continue;
                }
                if (slot != 0)
                {
                    ImGui::SameLine(0.0f, tileGap);
                }
                ImGui::PushID(static_cast<int>(slot));
                if (drawGameTile(games[game], tileWidth, tileHeight, game == selected))
                {
                    selected = game;
                }
                ImGui::PopID();
            }
            widgets::endPanel();
            ImGui::PopID();
        }

        if (canSwitchUser)
        {
            widgets::drawButtonLegend({{"A", tr(Text::HintOpen)}, {"B", tr(Text::HintQuit)},
                {"X", tr(Text::HintSwitchUser)}});
        }
        else
        {
            widgets::drawButtonLegend({{"A", tr(Text::HintOpen)}, {"B", tr(Text::HintQuit)}});
        }
        endFrame();
    }

    void SwitchUi::drawSaveList(const GameCard& game, const std::string& group,
        const std::string& consoleUser, const std::vector<std::string>& files, size_t& selected,
        const std::string& error)
    {
        beginFrame();
        widgets::drawHeader(tr(Text::SaveListTitle), status_.current());
        sectionGap();

        drawGamePanel(game, group,
            {{saveStatusText(game), theme::FONT_CAPTION, theme::TEXT_MUTED, false,
                hasSaves(game) ? theme::POSITIVE : theme::NEUTRAL}});
        const size_t firstFile = game.consoleSave ? 1 : 0;

        ImGui::SameLine(0.0f, theme::PANEL_GAP);
        widgets::beginPanel("save-files", ImVec2(0.0f, -SCREEN_BOTTOM_SPACE));
        if (!error.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ERROR_COLOR);
            ImGui::TextWrapped("%s", error.c_str());
            ImGui::PopStyleColor();
        }
        if (!hasSaves(game))
        {
            widgets::text(tr(Text::NoSavesFound), theme::FONT_ROW_TITLE, theme::TEXT, true);
            ImGui::PushStyleColor(ImGuiCol_Text, theme::vec(theme::TEXT_MUTED));
            ImGui::TextWrapped("%s", tr(Text::NoSavesHint));
            ImGui::PopStyleColor();
        }
        if (game.consoleSave)
        {
            // The user's save on the console, above the backups on the SD card.
            const std::string user = tr(Text::ConsoleSaveOf, {consoleUser});
            ImGui::PushID("console");
            if (widgets::drawListRow("save",
                    {tr(Text::ConsoleSaveTitle), user.c_str(), tr(Text::ConsoleSaveDetail),
                        icons::Icon::Trainer},
                    selected == 0))
            {
                selected = 0;
            }
            if (selected == 0)
            {
                ImGui::SetScrollHereY();
            }
            ImGui::PopID();
        }
        for (size_t file = 0; file < files.size(); ++file)
        {
            const size_t i = file + firstFile;
            // The file name; the folder it is in, which names the trainer and the date of the
            // backup; and the full path.
            const std::string& relative = files[file];
            const size_t slash          = relative.find_last_of('/');
            const std::string name =
                slash == std::string::npos ? relative : relative.substr(slash + 1);
            std::string folder =
                slash == std::string::npos ? std::string{} : backupLabel(relative.substr(0, slash));
            // Backups made by other save managers say which one made them.
            if (const std::string tool = storage::backupTool(relative); !tool.empty())
            {
                folder = tool + " · " + folder;
            }
            const std::string path = storage::savePath(relative);
            ImGui::PushID(static_cast<int>(i));
            if (widgets::drawListRow("save",
                    {name.c_str(), folder.c_str(), path.c_str(), icons::Icon::Backup},
                    i == selected))
            {
                selected = i;
            }
            if (i == selected)
            {
                ImGui::SetScrollHereY();
            }
            ImGui::PopID();
        }
        widgets::endPanel();

        // Without saves there is nothing to open.
        if (!hasSaves(game))
        {
            widgets::drawButtonLegend({{"B", tr(Text::HintBack)}, {"+", tr(Text::HintQuit)}});
        }
        else
        {
            widgets::drawButtonLegend({{"A", tr(Text::HintOpen)}, {"B", tr(Text::HintBack)},
                {"+", tr(Text::HintQuit)}});
        }
        endFrame();
    }

    void SwitchUi::drawSavePanel(const SaveSummary& save, const std::string& message)
    {
        drawGamePanel(save.game, save.group,
            {{save.file, 15.0f, theme::TEXT_SOFT, true, 0},
                {save.details, theme::FONT_SMALL, theme::TEXT_MUTED, false, 0},
                {save.path, theme::FONT_PATH, theme::TEXT_PATH, false, 0}},
            message, theme::ACCENT);
    }

    void SwitchUi::drawGameMenu(const SaveSummary& save, size_t selected, const std::string& message)
    {
        struct MenuOption
        {
            Text label;
            Text description;
            icons::Icon icon;
            bool available;
        };
        // The order matches GameMenuScreen. A divider follows the third option.
        static constexpr MenuOption options[] = {
            {Text::MenuPokemonEditor, Text::MenuPokemonEditorHint, icons::Icon::Boxes, true},
            {Text::MenuStorage, Text::MenuStorageHint, icons::Icon::Bank, true},
            {Text::MenuBag, Text::MenuBagHint, icons::Icon::Bag, true},
            {Text::MenuEvents, Text::MenuEventsHint, icons::Icon::Gift, false},
            {Text::MenuTrainerEditor, Text::MenuTrainerEditorHint, icons::Icon::Trainer, true},
            {Text::MenuOther, Text::MenuOtherHint, icons::Icon::More, false},
        };
        constexpr size_t dividerAfter = 2;

        beginFrame();
        widgets::drawHeader(tr(Text::ChooseAction), status_.current());
        sectionGap();

        drawSavePanel(save, message);

        ImGui::SameLine(0.0f, theme::PANEL_GAP);
        widgets::beginPanel("menu-options", ImVec2(0.0f, -SCREEN_BOTTOM_SPACE));
        for (size_t i = 0; i < sizeof(options) / sizeof(options[0]); ++i)
        {
            const MenuOption& option = options[i];
            ImGui::PushID(static_cast<int>(i));
            widgets::drawListRow("option",
                {tr(option.label), tr(option.description),
                    option.available ? nullptr : tr(Text::ComingSoon), option.icon,
                    option.available},
                i == selected);
            ImGui::PopID();
            if (i == dividerAfter)
            {
                // A line between the save's contents and the other tools, as on the canvas.
                const ImVec2 start = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddLine(ImVec2(start.x, start.y + 2.0f),
                    ImVec2(start.x + ImGui::GetContentRegionAvail().x, start.y + 2.0f),
                    theme::rgb(theme::BORDER));
                ImGui::Dummy(ImVec2(0.0f, 4.0f));
            }
        }
        widgets::endPanel();

        widgets::drawButtonLegend({{"A", tr(Text::HintOpen)}, {"B", tr(Text::HintBack)},
            {"Y", tr(Text::HintSave)}, {"X", tr(Text::HintDiscard)}, {"+", tr(Text::HintQuit)}});
        endFrame();
    }

    void SwitchUi::drawSlotTile(const PokemonSlot& slot, const ImVec2& min, float size,
        bool selected)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 max(min.x + size, min.y + size);
        if (slot.empty)
        {
            drawList->AddRectFilled(min, max, theme::rgb(EMPTY_SLOT), theme::CARD_RADIUS);
            drawList->AddRect(min, max, theme::rgb(theme::BORDER), theme::CARD_RADIUS);
        }
        else
        {
            drawList->AddRectFilled(min, max, theme::rgb(theme::CARD_SELECTED), theme::CARD_RADIUS);
            const float inset = size * 0.06f;
            drawPokemonSprite(slot.species,
                {static_cast<int>(min.x + inset), static_cast<int>(min.y + inset),
                    static_cast<int>(size - inset * 2.0f), static_cast<int>(size - inset * 2.0f)},
                slot.formSprite);
        }
        if (selected)
        {
            widgets::drawSelectionOutline(drawList, min, max, theme::CARD_RADIUS);
        }
    }

    void SwitchUi::drawPokemonDetail(const PokemonDetail& detail, bool compact)
    {
        // The box screen's sizes, and the bank screen's smaller ones.
        struct Metrics
        {
            float sectionGap, artSize, artGap, lineGap, iconSize;
            float numberSize, nameSize, levelSize, levelRow, pillHeight, pillFont, pillDot;
            float infoHeight, infoGap, infoLabel, infoValue;
            float ivHeight, ivGap, ivLabel, ivValue, ivPadding;
            float moveHeight, moveGap, moveFont, moveRadius;
            size_t moveColumns;
        };
        static constexpr Metrics regularMetrics{12.0f, 170.0f, 16.0f, 6.0f, 24.0f, 14.0f, 30.0f,
            theme::FONT_BODY, 26.0f, 32.0f, theme::FONT_SMALL, 12.0f, 52.0f, 8.0f,
            theme::FONT_PATH, theme::FONT_BODY, 58.0f, 6.0f, theme::FONT_PATH, 20.0f, 8.0f,
            44.0f, 8.0f, theme::FONT_SMALL, 12.0f, 4};
        static constexpr Metrics compactMetrics{8.0f, 116.0f, 12.0f, 4.0f, 20.0f, 12.0f, 22.0f,
            theme::FONT_SMALL, 22.0f, 26.0f, theme::FONT_CAPTION, 10.0f, 46.0f, 6.0f, 11.0f,
            15.0f, 48.0f, 5.0f, 11.0f, 17.0f, 6.0f, 36.0f, 6.0f, theme::FONT_CAPTION, 10.0f, 2};
        const Metrics& m       = compact ? compactMetrics : regularMetrics;
        const float sectionGap = m.sectionGap;
        const float artSize    = m.artSize;
        const float artGap     = m.artGap;
        const float lineGap    = m.lineGap;
        const float iconSize   = m.iconSize;

        ImDrawList* drawList   = ImGui::GetWindowDrawList();
        ImFont* bold           = theme::boldFont();
        ImFont* regular        = ImGui::GetFont();
        const ImVec2 origin    = ImGui::GetCursorScreenPos();
        const ImVec2 available = ImGui::GetContentRegionAvail();
        const float right      = origin.x + available.x;
        const auto height      = [](ImFont* font, float size, const std::string& text)
        { return font->CalcTextSizeA(size, FLT_MAX, 0.0f, text.c_str()).y; };
        const auto width = [](ImFont* font, float size, const std::string& text)
        { return font->CalcTextSizeA(size, FLT_MAX, 0.0f, text.c_str()).x; };

        if (detail.empty)
        {
            const std::string empty = tr(Text::Empty);
            drawList->AddText(bold, theme::FONT_ROW_TITLE,
                ImVec2(origin.x + (available.x - width(bold, theme::FONT_ROW_TITLE, empty)) * 0.5f,
                    origin.y + (available.y - height(bold, theme::FONT_ROW_TITLE, empty)) * 0.5f),
                theme::rgb(theme::TEXT_MUTED), empty.c_str());
            return;
        }

        // The art, and beside it the number, species, level, nickname and types, centred
        // together against it.
        drawList->AddRectFilled(origin, ImVec2(origin.x + artSize, origin.y + artSize),
            theme::rgb(DETAIL_ART), theme::ROW_RADIUS);
        const float artInset = artSize * 0.07f;
        drawPokemonSprite(detail.species, {static_cast<int>(origin.x + artInset),
                                              static_cast<int>(origin.y + artInset),
                                              static_cast<int>(artSize - artInset * 2.0f),
                                              static_cast<int>(artSize - artInset * 2.0f)},
            detail.formSprite);

        const float numberSize = m.numberSize;
        const float nameSize   = m.nameSize;
        const float pillHeight = m.pillHeight;
        const float levelSize  = m.levelSize;
        const float levelRow   = m.levelRow;
        const float textLeft       = origin.x + artSize + artGap;
        const float textWidth      = right - textLeft;
        float columnHeight         = height(bold, numberSize, detail.number) + lineGap +
                             height(bold, nameSize, detail.name) + lineGap + levelRow + lineGap +
                             pillHeight;
        if (!detail.nickname.empty())
        {
            columnHeight += height(regular, theme::FONT_SMALL, detail.nickname) + lineGap;
        }
        float y = origin.y + std::max(0.0f, (artSize - columnHeight) * 0.5f);

        widgets::drawTextEllipsis(drawList, bold, numberSize, ImVec2(textLeft, y),
            theme::rgb(theme::TEXT_MUTED), detail.number.c_str(), textWidth);
        y += height(bold, numberSize, detail.number) + lineGap;

        // The name, then the gender and shiny marks right after it.
        const float nameHeight = height(bold, nameSize, detail.name);
        const float marks      = (detail.male || detail.female ? iconSize + 10.0f : 0.0f) +
                            (detail.shiny ? iconSize + 10.0f : 0.0f);
        const float nameWidth  = std::min(width(bold, nameSize, detail.name), textWidth - marks);
        widgets::drawTextEllipsis(drawList, bold, nameSize, ImVec2(textLeft, y),
            theme::rgb(theme::TEXT), detail.name.c_str(), textWidth - marks);
        float markX        = textLeft + nameWidth + 10.0f;
        const float markY  = y + (nameHeight - iconSize) * 0.5f;
        if (detail.male || detail.female)
        {
            icons::drawSized(drawList, detail.male ? icons::Icon::Male : icons::Icon::Female,
                ImVec2(markX, markY), static_cast<int>(iconSize),
                theme::rgb(detail.male ? MALE : FEMALE));
            markX += iconSize + 10.0f;
        }
        if (detail.shiny)
        {
            icons::drawSized(drawList, icons::Icon::Shiny, ImVec2(markX, markY),
                static_cast<int>(iconSize), theme::rgb(SHINY));
        }
        y += nameHeight + lineGap;

        // The level with the ball it was caught in.
        const float levelHeight = height(regular, levelSize, detail.level);
        drawList->AddText(regular, levelSize, ImVec2(textLeft, y + (levelRow - levelHeight) * 0.5f),
            theme::rgb(theme::TEXT_SOFT), detail.level.c_str());
        const float ballX = textLeft + width(regular, levelSize, detail.level) + 10.0f;
        drawBallSprite(detail.ball, {static_cast<int>(ballX), static_cast<int>(y),
                                        static_cast<int>(levelRow), static_cast<int>(levelRow)});
        y += levelRow + lineGap;

        if (!detail.nickname.empty())
        {
            widgets::drawTextEllipsis(drawList, regular, theme::FONT_SMALL, ImVec2(textLeft, y),
                theme::rgb(theme::TEXT_MUTED), detail.nickname.c_str(), textWidth);
            y += height(regular, theme::FONT_SMALL, detail.nickname) + lineGap;
        }

        // Type pills: a dot in the type's colour and its name.
        float pillX = textLeft;
        const float dotRadius = m.pillDot;
        for (const auto& [name, type] : detail.types)
        {
            const float textX     = 4.0f + dotRadius * 2.0f + 8.0f;
            const float pillWidth = textX + width(bold, m.pillFont, name) + 14.0f;
            drawList->AddRectFilled(ImVec2(pillX, y), ImVec2(pillX + pillWidth, y + pillHeight),
                theme::rgb(PILL), pillHeight * 0.5f);
            const ImVec2 dot(pillX + 4.0f + dotRadius, y + pillHeight * 0.5f);
            drawList->AddCircleFilled(dot, dotRadius, typeRing(type), 24);
            drawList->AddCircleFilled(dot, dotRadius * 7.0f / 12.0f, theme::rgb(typeColor(type)), 24);
            drawList->AddText(bold, m.pillFont,
                ImVec2(pillX + textX, y + (pillHeight - height(bold, m.pillFont, name)) * 0.5f),
                theme::rgb(theme::TEXT_SOFT), name.c_str());
            pillX += pillWidth + 8.0f;
        }

        // Trainer, ID, item, ability, nature and origin game, two to a row.
        const float infoHeight     = m.infoHeight;
        const float infoGap        = m.infoGap;
        float top                  = origin.y + artSize + sectionGap;
        const float infoWidth      = (available.x - infoGap) / 2.0f;
        for (size_t i = 0; i < detail.info.size(); ++i)
        {
            const ImVec2 min(origin.x + static_cast<float>(i % 2) * (infoWidth + infoGap),
                top + static_cast<float>(i / 2) * (infoHeight + infoGap));
            drawList->AddRectFilled(min, ImVec2(min.x + infoWidth, min.y + infoHeight),
                theme::rgb(theme::CARD), theme::CARD_RADIUS);
            const std::string& label = detail.info[i].label;
            const std::string& value = detail.info[i].value;
            const float labelHeight  = height(bold, m.infoLabel, label);
            const float valueHeight  = height(bold, m.infoValue, value);
            const float lineTop      = min.y + (infoHeight - labelHeight - 1.0f - valueHeight) * 0.5f;
            widgets::drawTextEllipsis(drawList, bold, m.infoLabel, ImVec2(min.x + 14.0f, lineTop),
                theme::rgb(theme::TEXT_MUTED), label.c_str(), infoWidth - 28.0f);
            widgets::drawTextEllipsis(drawList, bold, m.infoValue,
                ImVec2(min.x + 14.0f, lineTop + labelHeight + 1.0f), theme::rgb(theme::TEXT),
                value.c_str(), infoWidth - 28.0f);
        }
        const size_t infoRows = (detail.info.size() + 1) / 2;
        top += static_cast<float>(infoRows) * infoHeight +
               static_cast<float>(infoRows > 0 ? infoRows - 1 : 0) * infoGap + sectionGap;

        // The IVs, with the stats the nature raises in red and lowers in blue.
        const float ivGap        = m.ivGap;
        const float ivHeight     = m.ivHeight;
        const float ivPad        = m.ivPadding;
        const float ivCount      = static_cast<float>(std::max<size_t>(detail.ivs.size(), 1));
        const float ivWidth      = (available.x - ivGap * (ivCount - 1.0f)) / ivCount;
        for (size_t i = 0; i < detail.ivs.size(); ++i)
        {
            const PokemonDetail::Iv& iv = detail.ivs[i];
            const ImVec2 min(origin.x + static_cast<float>(i) * (ivWidth + ivGap), top);
            drawList->AddRectFilled(min, ImVec2(min.x + ivWidth, min.y + ivHeight),
                theme::rgb(theme::CARD), 12.0f);
            const unsigned color = iv.nature > 0 ? RAISED : iv.nature < 0 ? LOWERED : theme::TEXT_MUTED;
            const std::string label = iv.label + (iv.nature > 0 ? " +" : iv.nature < 0 ? " -" : "");
            widgets::drawTextEllipsis(drawList, bold, m.ivLabel, ImVec2(min.x + ivPad, min.y + ivPad),
                theme::rgb(color), label.c_str(), ivWidth - ivPad * 2.0f);
            drawList->AddText(bold, m.ivValue,
                ImVec2(min.x + ivPad, min.y + ivHeight - ivPad - height(bold, m.ivValue, iv.value)),
                theme::rgb(theme::TEXT), iv.value.c_str());
        }
        top += ivHeight + sectionGap;

        // The four moves: in one row, or two to a row when compact.
        const float moveGap    = m.moveGap;
        const float moveHeight = m.moveHeight;
        const size_t columns =
            std::max<size_t>(1, std::min(m.moveColumns, detail.moves.size()));
        const float moveCount = static_cast<float>(columns);
        const float moveWidth = (available.x - moveGap * (moveCount - 1.0f)) / moveCount;
        for (size_t i = 0; i < detail.moves.size(); ++i)
        {
            const ImVec2 min(origin.x + static_cast<float>(i % columns) * (moveWidth + moveGap),
                top + static_cast<float>(i / columns) * (moveHeight + moveGap));
            drawList->AddRectFilled(min, ImVec2(min.x + moveWidth, min.y + moveHeight),
                theme::rgb(PILL), m.moveRadius);
            const std::string& move = detail.moves[i];
            widgets::drawTextEllipsis(drawList, bold, m.moveFont,
                ImVec2(min.x + 8.0f, min.y + (moveHeight - height(bold, m.moveFont, move)) * 0.5f),
                theme::rgb(theme::TEXT_SOFT), move.c_str(), moveWidth - 16.0f);
        }
    }

    void SwitchUi::drawBoxes(BoxView& view)
    {
        constexpr float boxPanelWidth   = 498.0f;
        constexpr float partyPanelWidth = 104.0f;
        constexpr float headerHeight    = 48.0f;
        constexpr float boxTile         = 72.0f;
        constexpr float boxTileGap      = 8.0f;
        constexpr float partyTile       = 64.0f;
        constexpr float partyTileGap    = 12.0f;
        constexpr ImGuiWindowFlags fixed =
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        beginFrame();
        widgets::drawHeader(tr(Text::MenuPokemonEditor), status_.current());
        sectionGap();

        ImFont* bold         = theme::boldFont();
        const auto centered = [bold](ImDrawList* drawList, float size, const std::string& text,
                                  float centerX, float top, unsigned color)
        {
            const ImVec2 textSize = bold->CalcTextSizeA(size, FLT_MAX, 0.0f, text.c_str());
            drawList->AddText(bold, size, ImVec2(std::round(centerX - textSize.x * 0.5f), top),
                theme::rgb(color), text.c_str());
            return textSize.y;
        };
        // A panel title with its count under it, centred on `centerX`.
        const auto title = [&centered](ImDrawList* drawList, const std::string& name,
                               const std::string& count, float centerX, float top)
        {
            const float nameHeight =
                centered(drawList, theme::FONT_HEADING, name, centerX, top, theme::TEXT);
            ImFont* regular        = ImGui::GetFont();
            const float countWidth =
                regular->CalcTextSizeA(theme::FONT_SMALL, FLT_MAX, 0.0f, count.c_str()).x;
            drawList->AddText(regular, theme::FONT_SMALL,
                ImVec2(std::round(centerX - countWidth * 0.5f), top + nameHeight + 2.0f),
                theme::rgb(theme::TEXT_MUTED), count.c_str());
        };
        const float contentWidth = ImGui::GetContentRegionAvail().x;
        // The slot a carried Pokémon came from: dimmed, with a thin accent outline.
        const auto drawCarriedSource = [](const ImVec2& min, float size)
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const ImVec2 max(min.x + size, min.y + size);
            drawList->AddRectFilled(min, max, theme::rgb(theme::BACKGROUND, 170), theme::CARD_RADIUS);
            drawList->AddRect(min, max, theme::rgb(theme::ACCENT), theme::CARD_RADIUS,
                ImDrawFlags_None, 2.0f);
        };
        // The carried Pokémon, lifted above the selected slot.
        const auto drawCarried = [this, &view](const ImVec2& min, float size)
        {
            if (!view.carrying || view.carried.empty)
            {
                return;
            }
            const float sprite = size * 0.8f;
            const float lift   = size * 0.3f;
            const ImVec2 center(min.x + size * 0.5f, min.y + size * 0.5f - lift);
            ImGui::GetWindowDrawList()->AddCircleFilled(
                ImVec2(min.x + size * 0.5f, min.y + size * 0.82f), size * 0.22f,
                IM_COL32(0, 0, 0, 40), 24);
            drawPokemonSprite(view.carried.species,
                {static_cast<int>(center.x - sprite * 0.5f), static_cast<int>(center.y - sprite * 0.5f),
                    static_cast<int>(sprite), static_cast<int>(sprite)},
                view.carried.formSprite);
        };

        // The party on the left: its six slots in a column.
        widgets::beginPanel("party", ImVec2(partyPanelWidth, -SCREEN_BOTTOM_SPACE), nullptr, fixed);
        {
            ImDrawList* drawList   = ImGui::GetWindowDrawList();
            const ImVec2 origin    = ImGui::GetCursorScreenPos();
            const ImVec2 available = ImGui::GetContentRegionAvail();
            title(drawList, tr(Text::PartyTitle),
                tr(Text::SlotCount, {std::to_string(view.partyCount), std::to_string(view.party.size())}),
                origin.x + available.x * 0.5f, origin.y + 1.0f);
            const float count       = static_cast<float>(view.party.size());
            const float columnHeight = partyTile * count + partyTileGap * (count - 1.0f);
            const float top =
                origin.y + headerHeight + (available.y - headerHeight - columnHeight) * 0.5f;
            for (size_t i = 0; i < view.party.size(); ++i)
            {
                const ImVec2 min(origin.x + (available.x - partyTile) * 0.5f,
                    top + static_cast<float>(i) * (partyTile + partyTileGap));
                ImGui::SetCursorScreenPos(min);
                ImGui::PushID(static_cast<int>(i));
                // Only filled slots and the one after them can be chosen.
                if (ImGui::InvisibleButton("party-slot", ImVec2(partyTile, partyTile)) &&
                    i <= view.partyCount)
                {
                    view.inParty  = true;
                    view.selected = i;
                }
                ImGui::PopID();
                drawSlotTile(view.party[i], min, partyTile, view.inParty && view.selected == i);
                if (view.carriedShown && view.carriedInParty && view.carriedSlot == i)
                {
                    drawCarriedSource(min, partyTile);
                }
            }
            if (view.inParty && view.selected < view.party.size())
            {
                drawCarried(ImVec2(origin.x + (available.x - partyTile) * 0.5f,
                                top + static_cast<float>(view.selected) * (partyTile + partyTileGap)),
                    partyTile);
            }
        }
        widgets::endPanel();

        // The box: arrows around its name and how full it is, then its 30 slots.
        ImGui::SameLine(0.0f, theme::PANEL_GAP);
        widgets::beginPanel("box", ImVec2(boxPanelWidth, -SCREEN_BOTTOM_SPACE), nullptr, fixed);
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const ImVec2 origin  = ImGui::GetCursorScreenPos();
            const float width    = ImGui::GetContentRegionAvail().x;
            for (const int direction : {-1, 1})
            {
                const ImVec2 min(direction < 0 ? origin.x : origin.x + width - headerHeight, origin.y);
                ImGui::SetCursorScreenPos(min);
                ImGui::PushID(direction);
                if (ImGui::InvisibleButton("page", ImVec2(headerHeight, headerHeight)))
                {
                    view.pageRequest = direction;
                }
                ImGui::PopID();
                drawList->AddRectFilled(min, ImVec2(min.x + headerHeight, min.y + headerHeight),
                    theme::rgb(theme::CARD), 12.0f);
                // The canvas arrow, 14 x 22, pointing the way it pages.
                const float cx = min.x + headerHeight * 0.5f;
                const float cy = min.y + headerHeight * 0.5f;
                const float d  = static_cast<float>(direction);
                const ImVec2 points[] = {ImVec2(cx - 4.0f * d, cy - 8.0f), ImVec2(cx + 4.0f * d, cy),
                    ImVec2(cx - 4.0f * d, cy + 8.0f)};
                drawList->AddPolyline(points, 3, theme::rgb(theme::TEXT), ImDrawFlags_None, 3.0f);
            }
            title(drawList, view.boxName,
                tr(Text::SlotCount, {std::to_string(view.boxFilled), std::to_string(view.box.size())}),
                origin.x + width * 0.5f, origin.y + 1.0f);

            const float gridWidth =
                boxTile * BoxView::COLUMNS + boxTileGap * (BoxView::COLUMNS - 1);
            const ImVec2 grid(origin.x + (width - gridWidth) * 0.5f, origin.y + headerHeight + 12.0f);
            for (size_t i = 0; i < view.box.size(); ++i)
            {
                const ImVec2 min(grid.x + static_cast<float>(i % BoxView::COLUMNS) * (boxTile + boxTileGap),
                    grid.y + static_cast<float>(i / BoxView::COLUMNS) * (boxTile + boxTileGap));
                ImGui::SetCursorScreenPos(min);
                ImGui::PushID(static_cast<int>(i));
                if (ImGui::InvisibleButton("slot", ImVec2(boxTile, boxTile)))
                {
                    view.inParty  = false;
                    view.selected = i;
                }
                ImGui::PopID();
                drawSlotTile(view.box[i], min, boxTile, !view.inParty && view.selected == i);
                if (view.carriedShown && !view.carriedInParty && view.carriedSlot == i)
                {
                    drawCarriedSource(min, boxTile);
                }
            }
            if (!view.inParty && view.selected < view.box.size())
            {
                drawCarried(ImVec2(grid.x + static_cast<float>(view.selected % BoxView::COLUMNS) *
                                                (boxTile + boxTileGap),
                                grid.y + static_cast<float>(view.selected / BoxView::COLUMNS) *
                                             (boxTile + boxTileGap)),
                    boxTile);
            }
        }
        widgets::endPanel();

        // The selected Pokémon.
        ImGui::SameLine(0.0f, theme::PANEL_GAP);
        widgets::beginPanel("detail", ImVec2(0.0f, -SCREEN_BOTTOM_SPACE), nullptr, fixed);
        drawPokemonDetail(view.detail);
        widgets::endPanel();

        // Y moves Pokémon here; the game menu saves.
        const float legend = view.carrying
            ? widgets::drawButtonLegend({{"Y", tr(Text::HintDrop)},
                  {"L / R", tr(Text::HintChangeBox)}, {"B", tr(Text::HintCancel)}})
            : widgets::drawButtonLegend({{"A", tr(Text::HintEdit)}, {"Y", tr(Text::HintMove)},
                  {"L / R", tr(Text::HintChangeBox)}, {"X", tr(Text::HintDiscard)},
                  {"B", tr(Text::HintBack)}});
        widgets::drawFooterMessage(
            view.message, theme::ACCENT, contentWidth - legend - theme::PANEL_GAP);
        endFrame();
    }

    void SwitchUi::drawBank(BankView& view)
    {
        // The canvas tints: amber for the bank, violet for the save.
        struct Tint
        {
            unsigned edge, fill, header, tile, emptyFill, emptyEdge;
        };
        static constexpr Tint bankTint{0x9A5B00, 0xFFFAF0, 0xFBECC8, 0xF7EBD0, 0xFFFDF8, 0xEADCBC};
        static constexpr Tint gameTint{0x5B3FC4, 0xF8F6FF, 0xEBE5FF, 0xECE7FB, 0xFCFBFF, 0xDDD5F5};
        constexpr float boxPanelWidth = 372.0f;
        constexpr float panelGap      = 16.0f;
        constexpr float headerHeight  = 40.0f;
        constexpr float headerIcon    = 18.0f;
        constexpr float navSize       = 44.0f;
        constexpr float navGap        = 10.0f;
        constexpr float tile          = 52.0f;
        constexpr float tileGap       = 6.0f;
        constexpr float tileRadius    = 12.0f;
        constexpr float bodyPadding   = 12.0f;

        beginFrame();
        widgets::drawHeader(tr(Text::BankTitle), status_.current());
        sectionGap();

        ImDrawList* drawList     = ImGui::GetWindowDrawList();
        ImFont* bold             = theme::boldFont();
        const ImVec2 origin      = ImGui::GetCursorScreenPos();
        const float contentWidth = ImGui::GetContentRegionAvail().x;
        const float panelHeight  = ImGui::GetContentRegionAvail().y - SCREEN_BOTTOM_SPACE;
        const auto textSize      = [](ImFont* font, float size, const std::string& text)
        { return font->CalcTextSizeA(size, FLT_MAX, 0.0f, text.c_str()); };

        // A panel with a tinted bar along its top: an icon, a title, and `note` on the right.
        const auto drawPanel = [&](const ImVec2& min, float width, const Tint& tint, bool tinted,
                                   icons::Icon icon, const std::string& titleText, float titleSize,
                                   const std::string& note)
        {
            const ImVec2 max(min.x + width, min.y + panelHeight);
            drawList->AddRectFilled(min, max, theme::rgb(tinted ? tint.fill : theme::PANEL),
                theme::PANEL_RADIUS);
            drawList->AddRectFilled(min, ImVec2(max.x, min.y + headerHeight),
                theme::rgb(tint.header), theme::PANEL_RADIUS, ImDrawFlags_RoundCornersTop);
            if (tinted)
            {
                drawList->AddRect(ImVec2(min.x + 1.0f, min.y + 1.0f),
                    ImVec2(max.x - 1.0f, max.y - 1.0f), theme::rgb(tint.edge),
                    theme::PANEL_RADIUS - 1.0f, ImDrawFlags_None, 2.0f);
            }
            else
            {
                drawList->AddRect(min, max, theme::rgb(theme::BORDER), theme::PANEL_RADIUS);
            }
            float x = min.x + 14.0f;
            icons::drawSized(drawList, icon,
                ImVec2(x, std::round(min.y + (headerHeight - headerIcon) * 0.5f)),
                static_cast<int>(headerIcon), theme::rgb(tint.edge));
            x += headerIcon + 8.0f;
            const ImVec2 titleExtent = textSize(bold, titleSize, titleText);
            drawList->AddText(bold, titleSize,
                ImVec2(x, std::round(min.y + (headerHeight - titleExtent.y) * 0.5f)),
                theme::rgb(tint.edge), titleText.c_str());
            x += titleExtent.x + 12.0f;
            const float noteRoom  = max.x - 14.0f - x;
            const ImVec2 noteSize = textSize(bold, theme::FONT_CAPTION, note);
            widgets::drawTextEllipsis(drawList, bold, theme::FONT_CAPTION,
                ImVec2(std::max(x, max.x - 14.0f - noteSize.x),
                    std::round(min.y + (headerHeight - noteSize.y) * 0.5f)),
                theme::rgb(theme::TEXT_SOFT), note.c_str(), noteRoom);
        };

        // One box: the panel, the arrows around its name and fill, and its 30 slots.
        const auto drawSide = [&](const BankView::Side& side, bool game, const ImVec2& min)
        {
            const Tint& tint = game ? gameTint : bankTint;
            drawPanel(min, boxPanelWidth, tint, true, game ? icons::Icon::Boxes : icons::Icon::Bank,
                tr(game ? Text::BankGameLabel : Text::BankBankLabel), 17.0f, side.source);

            const float gridWidth  = tile * BankView::COLUMNS + tileGap * (BankView::COLUMNS - 1);
            const size_t rows      = (side.slots.size() + BankView::COLUMNS - 1) / BankView::COLUMNS;
            const float gridHeight = tile * static_cast<float>(rows) +
                                     tileGap * static_cast<float>(rows > 0 ? rows - 1 : 0);
            const float bodyTop    = min.y + headerHeight;
            const float bodyHeight = panelHeight - headerHeight;
            const float top =
                std::round(bodyTop + (bodyHeight - navSize - navGap - gridHeight) * 0.5f);
            const float left  = min.x + bodyPadding;
            const float right = min.x + boxPanelWidth - bodyPadding;

            for (const int direction : {-1, 1})
            {
                const ImVec2 button(direction < 0 ? left : right - navSize, top);
                ImGui::SetCursorScreenPos(button);
                ImGui::PushID(game ? 100 + direction : direction);
                if (ImGui::InvisibleButton("page", ImVec2(navSize, navSize)))
                {
                    view.pageRequest = direction;
                    view.pageInGame  = game;
                }
                ImGui::PopID();
                drawList->AddRectFilled(button, ImVec2(button.x + navSize, button.y + navSize),
                    theme::rgb(theme::CARD), 12.0f);
                const float cx = button.x + navSize * 0.5f;
                const float cy = button.y + navSize * 0.5f;
                const float d  = static_cast<float>(direction);
                const ImVec2 points[] = {ImVec2(cx - 4.0f * d, cy - 8.0f), ImVec2(cx + 4.0f * d, cy),
                    ImVec2(cx - 4.0f * d, cy + 8.0f)};
                drawList->AddPolyline(points, 3, theme::rgb(theme::TEXT), ImDrawFlags_None, 3.0f);
            }
            // The box's name over how full it is, between the arrows.
            const float nameRoom    = right - left - 2.0f * (navSize + 8.0f);
            const std::string count = tr(Text::SlotCount,
                {std::to_string(side.filled), std::to_string(side.slots.size())});
            const ImVec2 nameSize  = textSize(bold, theme::FONT_BODY, side.boxName);
            const ImVec2 countSize = textSize(ImGui::GetFont(), theme::FONT_PATH, count);
            const float nameTop    = top + (navSize - nameSize.y - countSize.y) * 0.5f;
            const float center     = (left + right) * 0.5f;
            widgets::drawTextEllipsis(drawList, bold, theme::FONT_BODY,
                ImVec2(std::round(center - std::min(nameSize.x, nameRoom) * 0.5f), nameTop),
                theme::rgb(theme::TEXT), side.boxName.c_str(), nameRoom);
            drawList->AddText(ImGui::GetFont(), theme::FONT_PATH,
                ImVec2(std::round(center - countSize.x * 0.5f), nameTop + nameSize.y),
                theme::rgb(theme::TEXT_MUTED), count.c_str());

            const ImVec2 grid(std::round(min.x + (boxPanelWidth - gridWidth) * 0.5f),
                top + navSize + navGap);
            const auto slotMin = [&grid](size_t i)
            {
                return ImVec2(
                    grid.x + static_cast<float>(i % BankView::COLUMNS) * (tile + tileGap),
                    grid.y + static_cast<float>(i / BankView::COLUMNS) * (tile + tileGap));
            };
            for (size_t i = 0; i < side.slots.size(); ++i)
            {
                const ImVec2 slot = slotMin(i);
                const ImVec2 max(slot.x + tile, slot.y + tile);
                ImGui::SetCursorScreenPos(slot);
                ImGui::PushID(static_cast<int>(game ? 1000 + i : i));
                if (ImGui::InvisibleButton("slot", ImVec2(tile, tile)))
                {
                    view.inGame   = game;
                    view.selected = i;
                }
                ImGui::PopID();
                const PokemonSlot& pokemon = side.slots[i];
                if (pokemon.empty)
                {
                    drawList->AddRectFilled(slot, max, theme::rgb(tint.emptyFill), tileRadius);
                    drawList->AddRect(slot, max, theme::rgb(tint.emptyEdge), tileRadius);
                }
                else
                {
                    drawList->AddRectFilled(slot, max, theme::rgb(tint.tile), tileRadius);
                    drawPokemonSprite(pokemon.species,
                        {static_cast<int>(slot.x + 2.0f), static_cast<int>(slot.y + 2.0f),
                            static_cast<int>(tile - 4.0f), static_cast<int>(tile - 4.0f)},
                        pokemon.formSprite);
                }
                if (view.carriedShown && view.carriedInGame == game && view.carriedSlot == i)
                {
                    // Where the carried Pokémon came from: dimmed, with a thin accent outline.
                    drawList->AddRectFilled(slot, max, theme::rgb(theme::BACKGROUND, 170), tileRadius);
                    drawList->AddRect(slot, max, theme::rgb(theme::ACCENT), tileRadius,
                        ImDrawFlags_None, 2.0f);
                }
            }
            // Drawn after every slot so no neighbour covers the outline.
            if (view.inGame == game && view.selected < side.slots.size())
            {
                const ImVec2 slot = slotMin(view.selected);
                widgets::drawSelectionOutline(
                    drawList, slot, ImVec2(slot.x + tile, slot.y + tile), tileRadius);
                if (view.carrying && !view.carried.empty)
                {
                    // The carried Pokémon, lifted above the selected slot.
                    const float sprite = tile * 0.9f;
                    const float lift   = tile * 0.35f;
                    drawList->AddCircleFilled(ImVec2(slot.x + tile * 0.5f, slot.y + tile * 0.82f),
                        tile * 0.22f, IM_COL32(0, 0, 0, 40), 24);
                    drawPokemonSprite(view.carried.species,
                        {static_cast<int>(slot.x + (tile - sprite) * 0.5f),
                            static_cast<int>(slot.y + (tile - sprite) * 0.5f - lift),
                            static_cast<int>(sprite), static_cast<int>(sprite)},
                        view.carried.formSprite);
                }
            }
        };

        drawSide(view.bank, false, origin);
        drawSide(view.game, true, ImVec2(origin.x + boxPanelWidth + panelGap, origin.y));

        // The selected Pokémon, under a bar tinted after the box it is in.
        const float detailLeft = origin.x + 2.0f * (boxPanelWidth + panelGap);
        const float detailWidth = origin.x + contentWidth - detailLeft;
        const ImVec2 detailMin(detailLeft, origin.y);
        drawPanel(detailMin, detailWidth, view.inGame ? gameTint : bankTint, false,
            view.inGame ? icons::Icon::Boxes : icons::Icon::Bank,
            tr(view.inGame ? Text::BankFromGame : Text::BankFromBank), theme::FONT_SMALL,
            view.detailWhere);
        ImGui::SetCursorScreenPos(
            ImVec2(detailMin.x + bodyPadding, detailMin.y + headerHeight + bodyPadding));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::BeginChild("bank-detail",
            ImVec2(detailWidth - bodyPadding * 2.0f, panelHeight - headerHeight - bodyPadding * 2.0f),
            ImGuiChildFlags_None,
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PopStyleVar();
        drawPokemonDetail(view.detail, true);
        ImGui::EndChild();

        // Leave the cursor under the panels, where the legend goes.
        ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + panelHeight));
        ImGui::Dummy(ImVec2(0.0f, 0.0f));

        const float legend = view.carrying
            ? widgets::drawButtonLegend({{"Y", tr(Text::HintDrop)},
                  {"L / R", tr(Text::HintChangeBox)}, {"B", tr(Text::HintCancel)}})
            : widgets::drawButtonLegend({{"A", tr(Text::HintEdit)}, {"Y", tr(Text::HintMove)},
                  {"L / R", tr(Text::HintChangeBox)}, {"B", tr(Text::HintBack)}});
        widgets::drawFooterMessage(
            view.message, theme::ACCENT, contentWidth - legend - theme::PANEL_GAP);
        endFrame();
    }

    void SwitchUi::drawPokemonEditor(EditorView& view)
    {
        constexpr float rightWidth  = 400.0f;
        constexpr float artSize     = 190.0f;
        constexpr float artGap      = 20.0f;
        constexpr float cardGap     = 8.0f;
        constexpr float tabHeight   = 30.0f;
        constexpr float statColumnIv = 60.0f;
        constexpr float statColumnEv = 64.0f;
        constexpr float statColumnTotal = 56.0f;
        constexpr float statGap     = 6.0f;
        constexpr float statRow     = 40.0f;
        constexpr float moveRow     = 44.0f;
        constexpr ImGuiWindowFlags fixed =
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        ImFont* bold    = theme::boldFont();
        ImFont* regular = ImGui::GetFont();
        const auto size = [](ImFont* font, float px, const std::string& text)
        { return font->CalcTextSizeA(px, FLT_MAX, 0.0f, text.c_str()); };
        const auto select = [&view](EditorView::Area area, size_t index)
        {
            view.area     = area;
            view.selected = index;
        };

        beginFrame();
        const float contentWidth = ImGui::GetContentRegionAvail().x;

        // Left: the Pokémon, then a page of field cards.
        widgets::beginPanel("editor-fields",
            ImVec2(contentWidth - rightWidth - theme::PANEL_GAP, -SCREEN_BOTTOM_SPACE), nullptr, fixed);
        {
            ImDrawList* drawList   = ImGui::GetWindowDrawList();
            const ImVec2 origin    = ImGui::GetCursorScreenPos();
            const ImVec2 available = ImGui::GetContentRegionAvail();
            const PokemonDetail& header = view.header;

            drawList->AddRectFilled(origin, ImVec2(origin.x + artSize, origin.y + artSize),
                theme::rgb(DETAIL_ART), theme::ROW_RADIUS);
            if (!header.empty)
            {
                drawPokemonSprite(header.species,
                    {static_cast<int>(origin.x + 14.0f), static_cast<int>(origin.y + 14.0f),
                        static_cast<int>(artSize - 28.0f), static_cast<int>(artSize - 28.0f)},
                    header.formSprite);
            }

            // Number, species, level with ball and types, centred against the art.
            const float left      = origin.x + artSize + artGap;
            const float textWidth = origin.x + available.x - left;
            const float numberH   = size(bold, 14.0f, header.number).y;
            const float nameH     = size(bold, 28.0f, header.name).y;
            const float column    = numberH + 6.0f + nameH + 6.0f + 26.0f + 6.0f + 32.0f;
            float y               = origin.y + (artSize - column) * 0.5f;
            widgets::drawTextEllipsis(drawList, bold, 14.0f, ImVec2(left, y),
                theme::rgb(theme::TEXT_MUTED), header.number.c_str(), textWidth);
            y += numberH + 6.0f;
            widgets::drawTextEllipsis(drawList, bold, 28.0f, ImVec2(left, y),
                theme::rgb(theme::TEXT), header.name.c_str(), textWidth);
            y += nameH + 6.0f;
            const float levelH = size(regular, theme::FONT_BODY, header.level).y;
            drawList->AddText(regular, theme::FONT_BODY, ImVec2(left, y + (26.0f - levelH) * 0.5f),
                theme::rgb(theme::TEXT_SOFT), header.level.c_str());
            if (!header.empty)
            {
                drawBallSprite(header.ball,
                    {static_cast<int>(left + size(regular, theme::FONT_BODY, header.level).x + 10.0f),
                        static_cast<int>(y), 26, 26});
            }
            y += 26.0f + 6.0f;
            drawTypePills(drawList, ImVec2(left, y), header.types);

            // The page tabs.
            float tabX         = origin.x;
            const float tabTop = origin.y + artSize + 12.0f;
            for (size_t page = 0; page < view.pageCount; ++page)
            {
                const std::string name = tr(page == 0 ? Text::EditorPageMain : Text::EditorPageMore);
                const float width      = size(bold, theme::FONT_SMALL, name).x + 28.0f;
                const bool active      = page == view.page;
                drawList->AddRectFilled(ImVec2(tabX, tabTop), ImVec2(tabX + width, tabTop + tabHeight),
                    theme::rgb(active ? theme::CARD_SELECTED : theme::CARD), tabHeight * 0.5f);
                if (active)
                {
                    drawList->AddRect(ImVec2(tabX, tabTop), ImVec2(tabX + width, tabTop + tabHeight),
                        theme::rgb(theme::ACCENT), tabHeight * 0.5f, ImDrawFlags_None, 2.0f);
                }
                drawList->AddText(bold, theme::FONT_SMALL,
                    ImVec2(tabX + 14.0f, tabTop + (tabHeight - size(bold, theme::FONT_SMALL, name).y) * 0.5f),
                    theme::rgb(active ? theme::TEXT : theme::TEXT_MUTED), name.c_str());
                tabX += width + 8.0f;
            }

            // The field cards, filled row by row; a last card alone with one other on its row
            // spans the rest of it.
            const float gridTop    = tabTop + tabHeight + 12.0f;
            const size_t count     = view.fields.size();
            const size_t rows      = std::max<size_t>(5, (count + EditorView::COLUMNS - 1) / EditorView::COLUMNS);
            const float columns    = static_cast<float>(EditorView::COLUMNS);
            const float cardWidth  = (available.x - cardGap * (columns - 1.0f)) / columns;
            const float cardHeight = (origin.y + available.y - gridTop - cardGap * static_cast<float>(rows - 1)) /
                                     static_cast<float>(rows);
            for (size_t i = 0; i < count; ++i)
            {
                const Field& field  = view.fields[i];
                const size_t column = i % EditorView::COLUMNS;
                const bool spans    = i + 1 == count && column + 1 < EditorView::COLUMNS;
                const float width   = spans ? available.x - static_cast<float>(column) * (cardWidth + cardGap)
                                            : cardWidth;
                const ImVec2 min(origin.x + static_cast<float>(column) * (cardWidth + cardGap),
                    gridTop + static_cast<float>(i / EditorView::COLUMNS) * (cardHeight + cardGap));
                const ImVec2 max(min.x + width, min.y + cardHeight);
                ImGui::SetCursorScreenPos(min);
                ImGui::PushID(static_cast<int>(i));
                if (ImGui::InvisibleButton("field", ImVec2(width, cardHeight)))
                {
                    select(EditorView::Area::Fields, i);
                }
                ImGui::PopID();
                const bool selected = view.area == EditorView::Area::Fields && view.selected == i;
                drawList->AddRectFilled(min, max, theme::rgb(theme::CARD), theme::CARD_RADIUS);
                if (selected)
                {
                    widgets::drawSelectionOutline(drawList, min, max, theme::CARD_RADIUS);
                }

                // What sits on the right: a switch, the gender mark, or an arrow to edit.
                float trailing = 0.0f;
                const float centerY = (min.y + max.y) * 0.5f;
                if (field.checkbox)
                {
                    trailing = 44.0f;
                    const ImVec2 switchMin(max.x - 14.0f - 44.0f, centerY - 13.0f);
                    drawList->AddRectFilled(switchMin, ImVec2(switchMin.x + 44.0f, switchMin.y + 26.0f),
                        theme::rgb(field.checked ? SHINY : theme::DASHED), 13.0f);
                    drawList->AddCircleFilled(
                        ImVec2(field.checked ? switchMin.x + 31.0f : switchMin.x + 13.0f, centerY),
                        10.0f, theme::rgb(theme::ON_DARK), 20);
                }
                else if (field.genderMark != 0)
                {
                    trailing = 22.0f;
                    icons::drawSized(drawList, field.genderMark == 1 ? icons::Icon::Male : icons::Icon::Female,
                        ImVec2(max.x - 14.0f - 22.0f, centerY - 11.0f), 22,
                        theme::rgb(field.genderMark == 1 ? MALE : FEMALE));
                }
                else if (field.editable)
                {
                    trailing = 10.0f;
                    const float x = max.x - 14.0f - 10.0f;
                    const ImVec2 points[] = {ImVec2(x + 2.0f, centerY - 6.0f),
                        ImVec2(x + 8.0f, centerY), ImVec2(x + 2.0f, centerY + 6.0f)};
                    drawList->AddPolyline(points, 3, theme::rgb(theme::TEXT_MUTED), ImDrawFlags_None, 2.4f);
                }

                const float textWidth = width - 28.0f - (trailing > 0.0f ? trailing + 8.0f : 0.0f);
                const float labelH    = size(bold, theme::FONT_PATH, field.label).y;
                const float valueH    = size(bold, theme::FONT_BODY, field.value).y;
                const float textTop   = centerY - (labelH + 1.0f + valueH) * 0.5f;
                widgets::drawTextEllipsis(drawList, bold, theme::FONT_PATH, ImVec2(min.x + 14.0f, textTop),
                    theme::rgb(theme::TEXT_MUTED), field.label.c_str(), textWidth);
                widgets::drawTextEllipsis(drawList, bold, theme::FONT_BODY,
                    ImVec2(min.x + 14.0f, textTop + labelH + 1.0f),
                    theme::rgb(field.editable ? theme::TEXT : theme::TEXT_MUTED), field.value.c_str(), textWidth);
            }
        }
        widgets::endPanel();

        // Right: the stats table, then the moves.
        ImGui::SameLine(0.0f, theme::PANEL_GAP);
        ImGui::BeginGroup();
        const PokemonStats& stats = view.stats;
        const float statsHeight   = theme::PANEL_PADDING * 2.0f + 28.0f + 6.0f + 15.0f + 6.0f +
                                  statRow * EditorView::STAT_ROWS + statGap * (EditorView::STAT_ROWS - 1);
        widgets::beginPanel("editor-stats", ImVec2(rightWidth, statsHeight), nullptr, fixed);
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const ImVec2 origin  = ImGui::GetCursorScreenPos();
            const float width    = ImGui::GetContentRegionAvail().x;
            const std::string title = tr(Text::StatsTitle);
            drawList->AddText(bold, theme::FONT_HEADING, origin, theme::rgb(theme::TEXT), title.c_str());
            const std::string total =
                tr(Text::EvTotal, {std::to_string(stats.evTotal), std::to_string(stats.evMax)});
            const ImVec2 totalSize = size(regular, theme::FONT_SMALL, total);
            drawList->AddText(regular, theme::FONT_SMALL,
                ImVec2(origin.x + width - totalSize.x, origin.y + 28.0f - totalSize.y - 2.0f),
                theme::rgb(theme::TEXT_MUTED), total.c_str());

            // Column positions, from the right edge inwards.
            const float totalX = origin.x + width - 12.0f - statColumnTotal;
            const float evX    = totalX - statGap - statColumnEv;
            const float ivX    = evX - statGap - statColumnIv;
            const float headY  = origin.y + 28.0f + 6.0f;
            const auto columnTitle = [&](const std::string& text, float x, float columnWidth)
            {
                const float textX = columnWidth > 0.0f
                                        ? x + (columnWidth - size(bold, theme::FONT_PATH, text).x) * 0.5f
                                        : x;
                drawList->AddText(bold, theme::FONT_PATH, ImVec2(textX, headY),
                    theme::rgb(theme::TEXT_MUTED), text.c_str());
            };
            columnTitle(tr(Text::StatColumn), origin.x + 14.0f, 0.0f);
            columnTitle("IV", ivX, statColumnIv);
            columnTitle("EV", evX, statColumnEv);
            columnTitle(tr(Text::StatTotal), totalX, statColumnTotal);

            float rowTop = headY + 15.0f + 6.0f;
            for (size_t r = 0; r < stats.rows.size(); ++r)
            {
                const PokemonStats::Row& row = stats.rows[r];
                const ImVec2 min(origin.x, rowTop);
                drawList->AddRectFilled(min, ImVec2(min.x + width, min.y + statRow),
                    theme::rgb(theme::CARD), theme::CARD_RADIUS);
                const unsigned color = row.nature > 0 ? RAISED : row.nature < 0 ? LOWERED : theme::TEXT;
                const std::string name = row.name + (row.nature > 0 ? " +" : row.nature < 0 ? " -" : "");
                drawList->AddText(bold, theme::FONT_BODY,
                    ImVec2(min.x + 12.0f, min.y + (statRow - size(bold, theme::FONT_BODY, name).y) * 0.5f),
                    theme::rgb(color), name.c_str());

                // The IV and EV buttons.
                const struct
                {
                    EditorView::Area area;
                    float x;
                    float cellWidth;
                    const std::string& text;
                } cells[] = {{EditorView::Area::Iv, ivX, statColumnIv, row.iv},
                    {EditorView::Area::Ev, evX, statColumnEv, row.ev}};
                for (const auto& cell : cells)
                {
                    const ImVec2 cellMin(cell.x, min.y + 4.0f);
                    const ImVec2 cellMax(cell.x + cell.cellWidth, min.y + statRow - 4.0f);
                    ImGui::SetCursorScreenPos(cellMin);
                    ImGui::PushID(static_cast<int>(r * 2 + (cell.area == EditorView::Area::Ev ? 1 : 0)));
                    if (ImGui::InvisibleButton("stat", ImVec2(cell.cellWidth, cellMax.y - cellMin.y)))
                    {
                        select(cell.area, r);
                    }
                    ImGui::PopID();
                    drawList->AddRectFilled(cellMin, cellMax, theme::rgb(PILL), 8.0f);
                    if (view.area == cell.area && view.selected == r)
                    {
                        widgets::drawSelectionOutline(drawList, cellMin, cellMax, 8.0f);
                    }
                    const ImVec2 textSize = size(bold, theme::FONT_BODY, cell.text);
                    drawList->AddText(bold, theme::FONT_BODY,
                        ImVec2(cell.x + (cell.cellWidth - textSize.x) * 0.5f,
                            cellMin.y + (cellMax.y - cellMin.y - textSize.y) * 0.5f),
                        theme::rgb(theme::TEXT), cell.text.c_str());
                }
                const ImVec2 totalText = size(bold, theme::FONT_ROW_TITLE, row.total);
                drawList->AddText(bold, theme::FONT_ROW_TITLE,
                    ImVec2(totalX + (statColumnTotal - totalText.x) * 0.5f,
                        min.y + (statRow - totalText.y) * 0.5f),
                    theme::rgb(theme::TEXT), row.total.c_str());
                rowTop += statRow + statGap;
            }
        }
        widgets::endPanel();

        ImGui::Dummy(ImVec2(0.0f, 12.0f - ImGui::GetStyle().ItemSpacing.y * 2.0f));
        widgets::beginPanel("editor-moves", ImVec2(rightWidth, -SCREEN_BOTTOM_SPACE), nullptr, fixed);
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const ImVec2 origin  = ImGui::GetCursorScreenPos();
            const float width    = ImGui::GetContentRegionAvail().x;
            const std::string title = tr(Text::MovesTitle);
            drawList->AddText(bold, theme::FONT_HEADING, origin, theme::rgb(theme::TEXT), title.c_str());
            float rowTop = origin.y + 28.0f + 6.0f;
            for (size_t i = 0; i < stats.moves.size(); ++i)
            {
                const PokemonStats::MoveRow& move = stats.moves[i];
                const ImVec2 min(origin.x, rowTop);
                const ImVec2 max(min.x + width, min.y + moveRow);
                ImGui::SetCursorScreenPos(min);
                ImGui::PushID(static_cast<int>(i));
                if (ImGui::InvisibleButton("move", ImVec2(width, moveRow)))
                {
                    select(EditorView::Area::Moves, i);
                }
                ImGui::PopID();
                drawList->AddRectFilled(min, max, theme::rgb(PILL), theme::CARD_RADIUS);
                if (view.area == EditorView::Area::Moves && view.selected == i)
                {
                    widgets::drawSelectionOutline(drawList, min, max, theme::CARD_RADIUS);
                }
                // The canvas dot for the move's type; core has no move types, so it stays grey.
                const ImVec2 dot(min.x + 14.0f + 12.0f, min.y + moveRow * 0.5f);
                drawList->AddCircleFilled(dot, 12.0f, theme::rgb(0xB8BDCB), 24);
                drawList->AddCircleFilled(dot, 7.0f, theme::rgb(0x8A8F9B), 24);
                const float ppWidth = move.pp.empty() ? 0.0f : size(bold, theme::FONT_SMALL, move.pp).x;
                widgets::drawTextEllipsis(drawList, bold, theme::FONT_BODY,
                    ImVec2(min.x + 48.0f, min.y + (moveRow - size(bold, theme::FONT_BODY, move.name).y) * 0.5f),
                    theme::rgb(theme::TEXT_SOFT), move.name.c_str(), width - 48.0f - 14.0f - ppWidth - 10.0f);
                if (!move.pp.empty())
                {
                    drawList->AddText(bold, theme::FONT_SMALL,
                        ImVec2(max.x - 14.0f - ppWidth,
                            min.y + (moveRow - size(bold, theme::FONT_SMALL, move.pp).y) * 0.5f),
                        theme::rgb(theme::TEXT_SOFT), move.pp.c_str());
                }
                rowTop += moveRow + 4.0f;
            }
        }
        widgets::endPanel();
        ImGui::EndGroup();

        const float legend = view.readOnly
            ? widgets::drawButtonLegend({{"L / R", tr(Text::HintPage)}, {"B", tr(Text::HintBack)}})
            : widgets::drawButtonLegend({{"A", tr(Text::HintEdit)},
                  {"ZL / ZR", tr(Text::HintChange)}, {"L / R", tr(Text::HintPage)},
                  {"X", tr(Text::HintReset)}, {"B", tr(Text::HintBack)},
                  {"+", tr(Text::HintSaveChanges)}});
        widgets::drawFooterMessage(
            view.message, theme::ACCENT, contentWidth - legend - theme::PANEL_GAP);
        endFrame();
    }

    void SwitchUi::drawTrainer(const SaveSummary& save, const std::vector<Field>& fields,
        size_t selected, const std::string& message)
    {
        // Three rows hold the trainer's fields; more fields add rows.
        constexpr size_t minimumRows = 3;
        constexpr float cardGap      = 12.0f;

        beginFrame();
        widgets::drawHeader(tr(Text::MenuTrainerEditor), status_.current());
        sectionGap();

        drawSavePanel(save, message);

        ImGui::SameLine(0.0f, theme::PANEL_GAP);
        widgets::beginPanel("trainer-fields", ImVec2(0.0f, -SCREEN_BOTTOM_SPACE), nullptr,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        const size_t rows =
            std::max(minimumRows, (fields.size() + TRAINER_COLUMNS - 1) / TRAINER_COLUMNS);
        const ImVec2 origin    = ImGui::GetCursorScreenPos();
        const ImVec2 available = ImGui::GetContentRegionAvail();
        const float columns    = static_cast<float>(TRAINER_COLUMNS);
        const ImVec2 cardSize((available.x - cardGap * (columns - 1.0f)) / columns,
            (available.y - cardGap * static_cast<float>(rows - 1)) / static_cast<float>(rows));
        for (size_t i = 0; i < fields.size(); ++i)
        {
            const float column = static_cast<float>(i % TRAINER_COLUMNS);
            const float row    = static_cast<float>(i / TRAINER_COLUMNS);
            ImGui::SetCursorScreenPos(ImVec2(origin.x + column * (cardSize.x + cardGap),
                origin.y + row * (cardSize.y + cardGap)));
            ImGui::PushID(static_cast<int>(i));
            widgets::drawFieldCard("field", fields[i].label.c_str(), fields[i].value.c_str(),
                cardSize, i == selected, fields[i].editable);
            ImGui::PopID();
        }
        widgets::endPanel();

        widgets::drawButtonLegend({{"A", tr(Text::HintEdit)}, {"ZL / ZR", tr(Text::HintChange)},
            {"Y", tr(Text::HintSave)}, {"X", tr(Text::HintDiscard)}, {"B", tr(Text::HintBack)}});
        endFrame();
    }

    void SwitchUi::drawBag(BagView& view)
    {
        // The canvas bag: 44 pixel pouch rows with a 30 pixel icon box, and the items two to a
        // row in 48 pixel cards under a title row with the L and R chips.
        constexpr float pouchRowHeight = 44.0f;
        constexpr float pouchGap       = 6.0f;
        constexpr float pouchIconBox   = 30.0f;
        constexpr int pouchIconSize    = 18;
        constexpr float rowRadius      = 12.0f;
        constexpr float titleRow       = 36.0f;
        constexpr float titleGap       = 10.0f;
        constexpr float cardHeight     = 48.0f;
        constexpr float cardGap        = 8.0f;
        constexpr float itemArt        = 32.0f;
        constexpr float stepButton     = 26.0f;
        constexpr float countBoxHeight = 30.0f;
        constexpr float scrollbar      = 18.0f; // Room on the right for the scroll bar.
        constexpr unsigned ART         = theme::ART; // The item's picture behind the selection.
        constexpr ImGuiWindowFlags fixed =
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        beginFrame();
        widgets::drawHeader(tr(Text::MenuBag), status_.current());
        sectionGap();

        ImFont* bold             = theme::boldFont();
        const float contentWidth = ImGui::GetContentRegionAvail().x;
        const auto size          = [bold](float px, const std::string& text)
        { return bold->CalcTextSizeA(px, FLT_MAX, 0.0f, text.c_str()); };
        // `text` vertically centred between `top` and `top + height`, from `x`.
        const auto centredText = [bold, &size](ImDrawList* drawList, float px, float x, float top,
                                     float height, unsigned color, const std::string& text)
        {
            drawList->AddText(bold, px, ImVec2(x, std::round(top + (height - size(px, text).y) * 0.5f)),
                theme::rgb(color), text.c_str());
        };

        // The pouches, the shown one on a lighter row with its icon in the accent colour.
        widgets::beginPanel("bag-pouches", ImVec2(theme::SIDE_PANEL_WIDTH, -SCREEN_BOTTOM_SPACE),
            view.caption.c_str(), fixed);
        {
            ImDrawList* drawList   = ImGui::GetWindowDrawList();
            const ImVec2 origin    = ImGui::GetCursorScreenPos();
            const ImVec2 available = ImGui::GetContentRegionAvail();
            const float count      = static_cast<float>(std::max<size_t>(view.pouches.size(), 1));
            // Rows shrink when a game has more pouches than fit.
            const float rowHeight =
                std::min(pouchRowHeight, (available.y - pouchGap * (count - 1.0f)) / count);
            for (size_t i = 0; i < view.pouches.size(); ++i)
            {
                const BagView::Pouch& pouch = view.pouches[i];
                const bool shown            = i == view.pouch;
                const ImVec2 min(origin.x, origin.y + static_cast<float>(i) * (rowHeight + pouchGap));
                const ImVec2 max(min.x + available.x, min.y + rowHeight);
                ImGui::SetCursorScreenPos(min);
                ImGui::PushID(static_cast<int>(i));
                if (ImGui::InvisibleButton("pouch", ImVec2(available.x, rowHeight)))
                {
                    view.pouch = i;
                }
                ImGui::PopID();
                if (shown)
                {
                    drawList->AddRectFilled(min, max, theme::rgb(theme::CARD_SELECTED), rowRadius);
                }
                const ImVec2 boxMin(min.x + 7.0f, min.y + (rowHeight - pouchIconBox) * 0.5f);
                drawList->AddRectFilled(boxMin,
                    ImVec2(boxMin.x + pouchIconBox, boxMin.y + pouchIconBox),
                    theme::rgb(shown ? theme::ACCENT : theme::BORDER), 8.0f);
                const float iconInset = (pouchIconBox - static_cast<float>(pouchIconSize)) * 0.5f;
                icons::drawSized(drawList, pouch.icon,
                    ImVec2(boxMin.x + iconInset, boxMin.y + iconInset), pouchIconSize,
                    theme::rgb(shown ? theme::ON_DARK : theme::TEXT));

                const float countWidth = size(theme::FONT_SMALL, pouch.count).x;
                const float countX     = max.x - 14.0f - countWidth;
                centredText(drawList, theme::FONT_SMALL, countX, min.y, rowHeight,
                    shown ? theme::TEXT_SOFT : theme::TEXT_MUTED, pouch.count);
                const float nameLeft = boxMin.x + pouchIconBox + 12.0f;
                widgets::drawTextEllipsis(drawList, bold, theme::FONT_BODY,
                    ImVec2(nameLeft,
                        std::round(min.y + (rowHeight - size(theme::FONT_BODY, pouch.name).y) * 0.5f)),
                    theme::rgb(theme::TEXT), pouch.name.c_str(), countX - 12.0f - nameLeft);
            }
        }
        widgets::endPanel();

        ImGui::SameLine(0.0f, theme::PANEL_GAP);
        widgets::beginPanel("bag-items", ImVec2(0.0f, -SCREEN_BOTTOM_SPACE), nullptr, fixed);
        {
            ImDrawList* drawList   = ImGui::GetWindowDrawList();
            const ImVec2 origin    = ImGui::GetCursorScreenPos();
            const ImVec2 available = ImGui::GetContentRegionAvail();

            // The title row: the pouch between its L and R chips, and how many items it holds.
            const auto chip = [&](float x, const char* letter)
            {
                const float width = size(theme::FONT_SMALL, letter).x + 18.0f;
                const ImVec2 min(x, origin.y + (titleRow - 26.0f) * 0.5f);
                drawList->AddRectFilled(min, ImVec2(min.x + width, min.y + 26.0f),
                    theme::rgb(theme::BUTTON_FACE), 7.0f);
                centredText(drawList, theme::FONT_SMALL, min.x + 9.0f, min.y, 26.0f,
                    theme::ON_DARK, letter);
                return width;
            };
            const std::string pouchName =
                view.pouch < view.pouches.size() ? view.pouches[view.pouch].name : std::string{};
            float x = origin.x + 4.0f;
            x += chip(x, "L") + 12.0f;
            centredText(drawList, 20.0f, x, origin.y, titleRow, theme::TEXT, pouchName);
            x += size(20.0f, pouchName).x + 12.0f;
            chip(x, "R");
            const float countWidth = size(theme::FONT_SMALL, view.itemCount).x;
            centredText(drawList, theme::FONT_SMALL, origin.x + available.x - 4.0f - countWidth,
                origin.y, titleRow, theme::TEXT_MUTED, view.itemCount);

            // The items, ROWS rows at a time.
            const float top        = origin.y + titleRow + titleGap;
            const size_t rows      = (view.items.size() + BagView::COLUMNS - 1) / BagView::COLUMNS;
            const bool scrolls     = rows > BagView::ROWS;
            const float columns    = static_cast<float>(BagView::COLUMNS);
            const float listWidth  = available.x - (scrolls ? scrollbar : 0.0f);
            const float cardWidth  = (listWidth - cardGap * (columns - 1.0f)) / columns;
            const size_t first     = view.firstRow * BagView::COLUMNS;
            const size_t last =
                std::min(view.items.size(), first + BagView::COLUMNS * BagView::ROWS);
            for (size_t i = first; i < last; ++i)
            {
                const BagView::Item& item = view.items[i];
                const size_t slot         = i - first;
                const ImVec2 min(
                    origin.x + static_cast<float>(slot % BagView::COLUMNS) * (cardWidth + cardGap),
                    top + static_cast<float>(slot / BagView::COLUMNS) * (cardHeight + cardGap));
                const ImVec2 max(min.x + cardWidth, min.y + cardHeight);
                const bool selected = i == view.selected;
                ImGui::SetCursorScreenPos(min);
                ImGui::PushID(static_cast<int>(i));
                if (item.name.empty())
                {
                    // The slot after the last item: a dashed button that adds one.
                    if (widgets::drawDashedButton("add", tr(Text::BagAddItem),
                            ImVec2(cardWidth, cardHeight), false, rowRadius))
                    {
                        view.selected = i;
                    }
                }
                else
                {
                    if (ImGui::InvisibleButton("item", ImVec2(cardWidth, cardHeight)))
                    {
                        view.selected = i;
                    }
                    drawList->AddRectFilled(min, max,
                        theme::rgb(selected ? theme::CARD_SELECTED : theme::CARD), rowRadius);
                    // Where the item's picture goes; there are no item sprites yet.
                    const ImVec2 artMin(min.x + 8.0f, min.y + (cardHeight - itemArt) * 0.5f);
                    drawList->AddRectFilled(artMin, ImVec2(artMin.x + itemArt, artMin.y + itemArt),
                        theme::rgb(selected ? ART : theme::BORDER), 8.0f);

                    float nameRight = max.x - 14.0f;
                    if (!item.count.empty() && selected && view.carried == BagView::NONE)
                    {
                        // The selected item's quantity as a stepper: −, the number with a pencil
                        // for the keyboard, and +.
                        const float numberWidth = std::max(30.0f, size(theme::FONT_BODY, item.count).x);
                        const float boxWidth    = 10.0f + numberWidth + 6.0f + 14.0f + 8.0f;
                        const float centerY     = min.y + cardHeight * 0.5f;
                        float right             = max.x - 8.0f;
                        const auto stepper      = [&](float left, const char* sign)
                        {
                            const ImVec2 bMin(left, centerY - stepButton * 0.5f);
                            drawList->AddRectFilled(bMin,
                                ImVec2(bMin.x + stepButton, bMin.y + stepButton),
                                theme::rgb(theme::CARD), 8.0f);
                            const ImVec2 signSize = size(18.0f, sign);
                            drawList->AddText(bold, 18.0f,
                                ImVec2(std::round(bMin.x + (stepButton - signSize.x) * 0.5f),
                                    std::round(bMin.y + (stepButton - signSize.y) * 0.5f)),
                                theme::rgb(theme::ACCENT), sign);
                        };
                        right -= stepButton;
                        stepper(right, "+");
                        right -= 6.0f + boxWidth;
                        const ImVec2 boxMin(right, centerY - countBoxHeight * 0.5f);
                        const ImVec2 boxMax(right + boxWidth, centerY + countBoxHeight * 0.5f);
                        drawList->AddRectFilled(boxMin, boxMax, theme::rgb(theme::PANEL), 8.0f);
                        drawList->AddRect(boxMin, boxMax, theme::rgb(theme::DASHED), 8.0f,
                            ImDrawFlags_None, 2.0f);
                        const float numberRight = boxMin.x + 10.0f + numberWidth;
                        centredText(drawList, theme::FONT_BODY,
                            numberRight - size(theme::FONT_BODY, item.count).x, boxMin.y,
                            countBoxHeight, theme::TEXT, item.count);
                        icons::drawSized(drawList, icons::Icon::Edit,
                            ImVec2(numberRight + 6.0f, centerY - 7.0f), 14,
                            theme::rgb(theme::TEXT_MUTED));
                        right -= 6.0f + stepButton;
                        stepper(right, "−");
                        nameRight = right - 12.0f;
                    }
                    else if (!item.count.empty())
                    {
                        const std::string count = tr(Text::BagItemCount, {item.count});
                        const float width       = size(15.0f, count).x;
                        centredText(drawList, 15.0f, max.x - 14.0f - width, min.y, cardHeight,
                            theme::TEXT_SOFT, count);
                        nameRight = max.x - 14.0f - width - 12.0f;
                    }
                    const float nameLeft = artMin.x + itemArt + 12.0f;
                    widgets::drawTextEllipsis(drawList, bold, theme::FONT_BODY,
                        ImVec2(nameLeft,
                            std::round(min.y + (cardHeight - size(theme::FONT_BODY, item.name).y) * 0.5f)),
                        theme::rgb(theme::TEXT), item.name.c_str(), nameRight - nameLeft);
                }
                ImGui::PopID();
                if (i == view.carried && !selected)
                {
                    // The item picked up to move: a thin accent outline where it waits.
                    drawList->AddRect(min, max, theme::rgb(theme::ACCENT), rowRadius,
                        ImDrawFlags_None, 2.0f);
                }
                if (selected)
                {
                    widgets::drawSelectionOutline(drawList, min, max, rowRadius);
                }
            }

            // The scroll bar, only when the pouch takes more than one page: how much it shows,
            // and where.
            if (scrolls)
            {
                const float height = static_cast<float>(BagView::ROWS) * (cardHeight + cardGap) - cardGap;
                const ImVec2 trackMin(origin.x + available.x - 6.0f, top);
                drawList->AddRectFilled(trackMin, ImVec2(trackMin.x + 6.0f, top + height),
                    theme::rgb(theme::BORDER), 3.0f);
                const float shown = static_cast<float>(BagView::ROWS) / static_cast<float>(rows);
                const float start = static_cast<float>(view.firstRow) / static_cast<float>(rows);
                drawList->AddRectFilled(ImVec2(trackMin.x, top + height * start),
                    ImVec2(trackMin.x + 6.0f, top + height * (start + shown)),
                    theme::rgb(theme::TEXT_MUTED), 3.0f);
            }
        }
        widgets::endPanel();

        // Y moves items here and X takes them out; the game menu saves and discards.
        const float legend = view.carried != BagView::NONE
            ? widgets::drawButtonLegend({{"Y", tr(Text::HintDrop)}, {"B", tr(Text::HintCancel)}})
            : widgets::drawButtonLegend({{"A", tr(Text::HintChangeItem)},
                  {"ZL / ZR", tr(Text::HintQuantity)}, {"−", tr(Text::HintTypeQuantity)},
                  {"L / R", tr(Text::HintPouch)}, {"Y", tr(Text::HintMove)},
                  {"X", tr(Text::HintRemove)}, {"B", tr(Text::HintBack)}});
        widgets::drawFooterMessage(
            view.message, theme::ACCENT, contentWidth - legend - theme::PANEL_GAP);
        endFrame();
    }

    bool SwitchUi::editText(const std::string& title, const std::string& initial,
        std::string& result, size_t maxLength)
    {
        SwkbdConfig config;
        if (R_FAILED(swkbdCreate(&config, 0)))
        {
            return false;
        }
        swkbdConfigMakePresetDefault(&config);
        swkbdConfigSetHeaderText(&config, title.c_str());
        swkbdConfigSetInitialText(&config, initial.c_str());
        swkbdConfigSetStringLenMax(&config, static_cast<u32>(maxLength));
        char output[256]{};
        const Result status = swkbdShow(&config, output, sizeof(output));
        swkbdClose(&config);
        if (R_FAILED(status) || output[0] == '\0')
        {
            return false;
        }
        result = output;
        return true;
    }

    void SwitchUi::drawOptionPicker(const std::string& title, const std::string& current,
        const std::vector<std::string>& options, const std::vector<size_t>& spriteIndices,
        size_t firstRow, size_t& index)
    {
        // The canvas move dialog: 1120 wide, a title row, then the options four to a row.
        constexpr float width     = 1120.0f;
        constexpr float titleRow  = 44.0f;
        constexpr float gap       = 14.0f;
        constexpr float rowHeight = 40.0f;
        constexpr float rowGap    = 4.0f;
        constexpr float columnGap = 6.0f;
        constexpr float scrollbar = 18.0f; // Room on the right for the scroll bar.
        // As tall as the options need, up to PICKER_ROWS rows.
        const size_t rows      = (options.size() + PICKER_COLUMNS - 1) / PICKER_COLUMNS;
        const size_t shownRows = std::clamp<size_t>(rows, 1, PICKER_ROWS);
        const float listHeight =
            rowHeight * static_cast<float>(shownRows) + rowGap * static_cast<float>(shownRows - 1);
        const float height = MODAL_PADDING * 2.0f + titleRow + gap + listHeight;

        beginModal(width, height);
        ImFont* bold         = theme::boldFont();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 origin  = ImGui::GetCursorScreenPos();
        const float inner    = width - MODAL_PADDING * 2.0f;
        const auto size      = [bold](float px, const std::string& text)
        { return bold->CalcTextSizeA(px, FLT_MAX, 0.0f, text.c_str()); };

        // The title, and on the right the value before picking.
        drawList->AddText(bold, 26.0f, ImVec2(origin.x, origin.y + (titleRow - size(26.0f, title).y) * 0.5f),
            theme::rgb(theme::TEXT), title.c_str());
        if (!current.empty())
        {
            const std::string label = tr(Text::PickerCurrent);
            const float valueWidth  = std::min(size(22.0f, current).x, inner * 0.45f);
            const float valueX      = origin.x + inner - valueWidth;
            widgets::drawTextEllipsis(drawList, bold, 22.0f,
                ImVec2(valueX, origin.y + (titleRow - size(22.0f, current).y) * 0.5f),
                theme::rgb(theme::TEXT), current.c_str(), valueWidth);
            drawList->AddText(bold, theme::FONT_SMALL,
                ImVec2(valueX - 12.0f - size(theme::FONT_SMALL, label).x,
                    origin.y + (titleRow - size(theme::FONT_SMALL, label).y) * 0.5f),
                theme::rgb(theme::TEXT_MUTED), label.c_str());
        }

        // The options.
        const float top         = origin.y + titleRow + gap;
        const float columns     = static_cast<float>(PICKER_COLUMNS);
        const float optionWidth = (inner - scrollbar - columnGap * (columns - 1.0f)) / columns;
        const size_t first      = firstRow * PICKER_COLUMNS;
        const size_t last       = std::min(options.size(), first + PICKER_COLUMNS * PICKER_ROWS);
        for (size_t i = first; i < last; ++i)
        {
            const size_t slot = i - first;
            const ImVec2 min(origin.x + static_cast<float>(slot % PICKER_COLUMNS) * (optionWidth + columnGap),
                top + static_cast<float>(slot / PICKER_COLUMNS) * (rowHeight + rowGap));
            const ImVec2 max(min.x + optionWidth, min.y + rowHeight);
            ImGui::SetCursorScreenPos(min);
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::InvisibleButton("option", ImVec2(optionWidth, rowHeight)))
            {
                index = i;
            }
            ImGui::PopID();
            const bool selected = i == index;
            drawList->AddRectFilled(min, max, theme::rgb(theme::CARD), 10.0f);
            if (selected)
            {
                widgets::drawSelectionOutline(drawList, min, max, 10.0f);
            }
            float textLeft = min.x + 10.0f;
            if (i < spriteIndices.size())
            {
                drawBallSprite(spriteIndices[i],
                    {static_cast<int>(textLeft), static_cast<int>(min.y + (rowHeight - 24.0f) * 0.5f), 24, 24});
                textLeft += 24.0f + 8.0f;
            }
            widgets::drawTextEllipsis(drawList, bold, theme::FONT_SMALL,
                ImVec2(textLeft, min.y + (rowHeight - size(theme::FONT_SMALL, options[i]).y) * 0.5f),
                theme::rgb(theme::TEXT), options[i].c_str(), max.x - 10.0f - textLeft);
        }

        // The scroll bar, only when the options take more than one page: how much the page
        // shows, and where.
        if (rows > PICKER_ROWS)
        {
            const ImVec2 trackMin(origin.x + inner - 6.0f, top);
            drawList->AddRectFilled(trackMin, ImVec2(trackMin.x + 6.0f, top + listHeight),
                theme::rgb(theme::BORDER), 3.0f);
            const float shown = static_cast<float>(PICKER_ROWS) / static_cast<float>(rows);
            const float start = static_cast<float>(firstRow) / static_cast<float>(rows);
            drawList->AddRectFilled(ImVec2(trackMin.x, top + listHeight * start),
                ImVec2(trackMin.x + 6.0f, top + listHeight * (start + shown)),
                theme::rgb(theme::TEXT_MUTED), 3.0f);
        }

        // L/R only page through lists longer than one page.
        if (rows > PICKER_ROWS)
        {
            widgets::drawButtonLegend({{"A", tr(Text::HintSelect)}, {"B", tr(Text::HintBack)},
                {"L / R", tr(Text::HintChangePage)}});
        }
        else
        {
            widgets::drawButtonLegend({{"A", tr(Text::HintSelect)}, {"B", tr(Text::HintBack)}});
        }
        endModal();
    }

    void SwitchUi::drawNaturePicker(
        const std::vector<std::string>& natures, size_t current, size_t& row, size_t& column)
    {
        // Rows raise a stat and columns lower one, in this order; the diagonal is neutral.
        static constexpr Text stats[] = {Text::StatShortAttack, Text::StatShortDefense,
            Text::StatShortSpeed, Text::StatShortSpAttack, Text::StatShortSpDefense};
        // The canvas nature dialog: 780 wide, a 96 pixel label column and 56 pixel cells.
        constexpr float width      = 780.0f;
        constexpr float labelWidth = 96.0f;
        constexpr float cell       = 56.0f;
        constexpr float cellGap    = 6.0f;
        constexpr float gap        = 14.0f;
        constexpr unsigned NEUTRAL_TEXT = SHINY;

        ImFont* bold     = theme::boldFont();
        const auto size  = [bold](float px, const std::string& text)
        { return bold->CalcTextSizeA(px, FLT_MAX, 0.0f, text.c_str()); };
        const std::string key = tr(Text::NatureKey);
        constexpr float titleHeight = 44.0f; // The title row of the other pickers.
        const float gridHeight  = cell * 6.0f + cellGap * 5.0f;
        const float height      = MODAL_PADDING * 2.0f + titleHeight + gap + gridHeight + gap +
                             size(theme::FONT_SMALL, key).y;

        beginModal(width, height);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 origin  = ImGui::GetCursorScreenPos();
        const float inner    = width - MODAL_PADDING * 2.0f;

        std::string raised[5];
        std::string lowered[5];
        for (size_t i = 0; i < 5; ++i)
        {
            raised[i]  = std::string("+") + tr(stats[i]);
            lowered[i] = std::string("\xE2\x88\x92") + tr(stats[i]);
        }

        // The title, and on the right the Pokémon's nature before picking, as in the other pickers.
        const std::string title = tr(Text::FieldNature);
        drawList->AddText(bold, 26.0f,
            ImVec2(origin.x, origin.y + (titleHeight - size(26.0f, title).y) * 0.5f),
            theme::rgb(theme::TEXT), title.c_str());
        if (natures.size() == 25 && current < 25)
        {
            const std::string& name = natures[current];
            const std::string label = tr(Text::PickerCurrent);
            const float valueWidth  = std::min(size(22.0f, name).x, inner * 0.45f);
            const float valueX      = origin.x + inner - valueWidth;
            widgets::drawTextEllipsis(drawList, bold, 22.0f,
                ImVec2(valueX, origin.y + (titleHeight - size(22.0f, name).y) * 0.5f),
                theme::rgb(theme::TEXT), name.c_str(), valueWidth);
            drawList->AddText(bold, theme::FONT_SMALL,
                ImVec2(valueX - 12.0f - size(theme::FONT_SMALL, label).x,
                    origin.y + (titleHeight - size(theme::FONT_SMALL, label).y) * 0.5f),
                theme::rgb(theme::TEXT_MUTED), label.c_str());
        }

        // The grid: the neutral corner, the lowered stats along the top, the raised ones down
        // the left, and the 25 natures.
        const float top       = origin.y + titleHeight + gap;
        const float cellWidth = (inner - labelWidth - cellGap * 5.0f) / 5.0f;
        const auto centred    = [&](const std::string& text, float px, const ImVec2& min, float w,
                                 unsigned color)
        {
            const ImVec2 textSize = size(px, text);
            drawList->AddText(bold, px,
                ImVec2(std::round(min.x + (w - textSize.x) * 0.5f),
                    std::round(min.y + (cell - textSize.y) * 0.5f)),
                theme::rgb(color), text.c_str());
        };
        centred(tr(Text::NatureNeutral), 13.0f, ImVec2(origin.x, top), labelWidth, NEUTRAL_TEXT);
        for (size_t x = 0; x < 5; ++x)
        {
            centred(lowered[x], theme::FONT_BODY,
                ImVec2(origin.x + labelWidth + cellGap + static_cast<float>(x) * (cellWidth + cellGap), top),
                cellWidth, LOWERED);
        }
        for (size_t y = 0; y < 5; ++y)
        {
            const float rowTop = top + static_cast<float>(y + 1) * (cell + cellGap);
            centred(raised[y], theme::FONT_BODY, ImVec2(origin.x, rowTop), labelWidth, RAISED);
            for (size_t x = 0; x < 5 && natures.size() == 25; ++x)
            {
                const ImVec2 min(origin.x + labelWidth + cellGap + static_cast<float>(x) * (cellWidth + cellGap),
                    rowTop);
                const ImVec2 max(min.x + cellWidth, min.y + cell);
                ImGui::SetCursorScreenPos(min);
                ImGui::PushID(static_cast<int>(y * 5 + x));
                if (ImGui::InvisibleButton("nature", ImVec2(cellWidth, cell)))
                {
                    row    = y;
                    column = x;
                }
                ImGui::PopID();
                drawList->AddRectFilled(min, max, theme::rgb(theme::CARD), 12.0f);
                if (row == y && column == x)
                {
                    widgets::drawSelectionOutline(drawList, min, max, 12.0f);
                }
                centred(natures[y * 5 + x], theme::FONT_ROW_TITLE, min, cellWidth,
                    x == y ? NEUTRAL_TEXT : theme::TEXT);
            }
        }

        drawList->AddText(bold, theme::FONT_SMALL, ImVec2(origin.x, top + gridHeight + gap),
            theme::rgb(theme::TEXT_MUTED), key.c_str());

        widgets::drawButtonLegend({{"A", tr(Text::HintSelect)}, {"B", tr(Text::HintBack)}});
        endModal();
    }

    void SwitchUi::drawUserPicker(
        const std::vector<std::string>& users, size_t current, size_t& selected)
    {
        // The canvas "Cambiar usuario" dialog: 760 wide, a red title bar, and a row of 150 pixel
        // cards for up to four users; more users take more rows.
        constexpr float width       = 760.0f;
        constexpr float sidePadding = 28.0f;
        constexpr float barHeight   = 56.0f;
        constexpr float barEdge     = 4.0f;
        constexpr float border      = 3.0f;
        constexpr float bodyTop     = 24.0f;
        constexpr float hintGap     = 22.0f;
        constexpr float cardWidth   = 150.0f;
        constexpr float cardGap     = 20.0f;
        constexpr float cardRadius  = 18.0f;
        constexpr float avatarSize  = 96.0f;
        constexpr float avatarTop   = 18.0f;
        constexpr float nameGap     = 10.0f;
        constexpr float nameSize    = 17.0f;
        constexpr float badgeHeight = 22.0f;
        constexpr float cardBottom  = 16.0f;
        constexpr size_t perRow     = 4;
        // The canvas avatar colours, then more for consoles with up to eight users.
        static constexpr unsigned AVATARS[] = {0x5B8BD6, 0xE0A030, 0x4CAF50, 0x8A4FD0, 0xD6405A,
            0x22B8A8, 0xE0502E, 0x6F7A8C};

        ImFont* bold    = theme::boldFont();
        ImFont* regular = ImGui::GetFont();
        const char* hint = tr(Text::UserPickerHint);
        const float inner = width - sidePadding * 2.0f;
        const float hintHeight =
            regular->CalcTextSizeA(theme::FONT_BODY, FLT_MAX, inner, hint).y;
        const float nameHeight = bold->CalcTextSizeA(nameSize, FLT_MAX, 0.0f, "Ag").y;
        const float cardHeight =
            avatarTop + avatarSize + nameGap + nameHeight + nameGap + badgeHeight + cardBottom;
        const size_t rows   = std::max<size_t>(1, (users.size() + perRow - 1) / perRow);
        const float cardsHeight =
            static_cast<float>(rows) * cardHeight + static_cast<float>(rows - 1) * cardGap;
        const float contentBottom =
            barHeight + barEdge + bodyTop + hintHeight + hintGap + cardsHeight + 8.0f;

        beginModal(width, contentBottom + MODAL_PADDING);
        const ImVec2 min  = ImGui::GetWindowPos();
        const ImVec2 size = ImGui::GetWindowSize();
        const ImVec2 max(min.x + size.x, min.y + size.y);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // The canvas dialog is the screen background with a black outline and a red bar.
        drawList->AddRectFilled(min, max, theme::rgb(theme::BACKGROUND), MODAL_RADIUS);
        drawList->AddRectFilled(min, ImVec2(max.x, min.y + barHeight), theme::rgb(theme::HEADER),
            MODAL_RADIUS, ImDrawFlags_RoundCornersTop);
        drawList->AddRectFilled(ImVec2(min.x, min.y + barHeight),
            ImVec2(max.x, min.y + barHeight + barEdge), theme::rgb(theme::HEADER_EDGE));

        constexpr int titleIcon = 22;
        const char* title       = tr(Text::UserPickerTitle);
        const float barCenter   = min.y + barHeight * 0.5f;
        icons::drawSized(drawList, icons::Icon::Trainer,
            ImVec2(min.x + sidePadding, std::round(barCenter - titleIcon * 0.5f)), titleIcon,
            theme::rgb(theme::ON_DARK));
        const ImVec2 titleSize = bold->CalcTextSizeA(theme::FONT_HEADING, FLT_MAX, 0.0f, title);
        drawList->AddText(bold, theme::FONT_HEADING,
            ImVec2(min.x + sidePadding + titleIcon + 12.0f, std::round(barCenter - titleSize.y * 0.5f)),
            theme::rgb(theme::ON_DARK), title);

        const float hintTop = min.y + barHeight + barEdge + bodyTop;
        drawList->AddText(regular, theme::FONT_BODY, ImVec2(min.x + sidePadding, hintTop),
            theme::rgb(theme::TEXT_MUTED), hint, nullptr, inner);

        const float cardsTop = hintTop + hintHeight + hintGap;
        for (size_t i = 0; i < users.size(); ++i)
        {
            const size_t row     = i / perRow;
            const size_t inRow   = std::min(perRow, users.size() - row * perRow);
            const float rowWidth = static_cast<float>(inRow) * cardWidth +
                                   static_cast<float>(inRow - 1) * cardGap;
            const ImVec2 cardMin(
                std::round(min.x + (width - rowWidth) * 0.5f +
                           static_cast<float>(i % perRow) * (cardWidth + cardGap)),
                cardsTop + static_cast<float>(row) * (cardHeight + cardGap));
            const ImVec2 cardMax(cardMin.x + cardWidth, cardMin.y + cardHeight);

            ImGui::SetCursorScreenPos(cardMin);
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::InvisibleButton("user", ImVec2(cardWidth, cardHeight)))
            {
                selected = i;
            }
            ImGui::PopID();

            drawList->AddRectFilled(cardMin, cardMax, theme::rgb(theme::PANEL), cardRadius);
            const ImVec2 avatar(cardMin.x + cardWidth * 0.5f, cardMin.y + avatarTop + avatarSize * 0.5f);
            drawList->AddCircleFilled(
                avatar, avatarSize * 0.5f, theme::rgb(AVATARS[i % std::size(AVATARS)]), 48);
            constexpr int avatarIcon = 48;
            icons::drawSized(drawList, icons::Icon::Trainer,
                ImVec2(std::round(avatar.x - avatarIcon * 0.5f), std::round(avatar.y - avatarIcon * 0.5f)),
                avatarIcon, theme::rgb(theme::ON_DARK));

            const float nameTop  = cardMin.y + avatarTop + avatarSize + nameGap;
            const float maxName  = cardWidth - 16.0f;
            const float nameWide = std::min(
                maxName, bold->CalcTextSizeA(nameSize, FLT_MAX, 0.0f, users[i].c_str()).x);
            widgets::drawTextEllipsis(drawList, bold, nameSize,
                ImVec2(std::round(cardMin.x + (cardWidth - nameWide) * 0.5f), nameTop),
                theme::rgb(theme::TEXT), users[i].c_str(), maxName);

            if (i == current)
            {
                const char* badge     = tr(Text::UserCurrent);
                const ImVec2 textSize = bold->CalcTextSizeA(12.0f, FLT_MAX, 0.0f, badge);
                const float badgeWidth = textSize.x + 20.0f;
                const ImVec2 badgeMin(std::round(cardMin.x + (cardWidth - badgeWidth) * 0.5f),
                    nameTop + nameHeight + nameGap);
                drawList->AddRectFilled(badgeMin,
                    ImVec2(badgeMin.x + badgeWidth, badgeMin.y + badgeHeight),
                    theme::rgb(theme::ACCENT), badgeHeight * 0.5f);
                drawList->AddText(bold, 12.0f,
                    ImVec2(badgeMin.x + 10.0f, std::round(badgeMin.y + (badgeHeight - textSize.y) * 0.5f)),
                    theme::rgb(theme::ON_DARK), badge);
            }
            if (i == selected)
            {
                widgets::drawSelectionOutline(drawList, cardMin, cardMax, cardRadius);
            }
        }

        drawList->AddRect(min, max, theme::rgb(theme::HEADER_EDGE), MODAL_RADIUS, ImDrawFlags_None,
            border);
        widgets::drawButtonLegend({{"A", tr(Text::HintSelect)}, {"B", tr(Text::HintCancel)}});
        endModal();
    }

    void SwitchUi::drawDatePicker(DatePickerView& view)
    {
        // A dialog like the option picker's: a title row, then a column for each part with its
        // header, the value Up would pick, the value, and the value Down would pick.
        constexpr float width     = 720.0f;
        constexpr float titleRow  = 44.0f;
        constexpr float gap       = 14.0f;
        constexpr float headerRow = 24.0f;
        constexpr float besideRow = 44.0f;
        constexpr float valueRow  = 64.0f;
        constexpr float rowGap    = 6.0f;
        constexpr float columnGap = 12.0f;
        const float columnsHeight = headerRow + besideRow * 2.0f + valueRow + rowGap * 3.0f;
        const float height        = MODAL_PADDING * 2.0f + titleRow + gap + columnsHeight;

        beginModal(width, height);
        ImFont* bold         = theme::boldFont();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 origin  = ImGui::GetCursorScreenPos();
        const float inner    = width - MODAL_PADDING * 2.0f;
        const auto size      = [bold](float px, const std::string& text)
        { return bold->CalcTextSizeA(px, FLT_MAX, 0.0f, text.c_str()); };
        const auto centred = [&](const std::string& text, float px, const ImVec2& min,
                                 const ImVec2& max, unsigned color)
        {
            const ImVec2 textSize = size(px, text);
            const float textWidth = std::min(textSize.x, max.x - min.x - 16.0f);
            widgets::drawTextEllipsis(drawList, bold, px,
                ImVec2(std::round(min.x + (max.x - min.x - textWidth) * 0.5f),
                    std::round(min.y + (max.y - min.y - textSize.y) * 0.5f)),
                theme::rgb(color), text.c_str(), textWidth);
        };

        // The title, and on the right the date before picking.
        drawList->AddText(bold, 26.0f,
            ImVec2(origin.x, origin.y + (titleRow - size(26.0f, view.title).y) * 0.5f),
            theme::rgb(theme::TEXT), view.title.c_str());
        if (!view.current.empty())
        {
            const std::string label = tr(Text::PickerCurrent);
            const float valueWidth  = std::min(size(22.0f, view.current).x, inner * 0.45f);
            const float valueX      = origin.x + inner - valueWidth;
            widgets::drawTextEllipsis(drawList, bold, 22.0f,
                ImVec2(valueX, origin.y + (titleRow - size(22.0f, view.current).y) * 0.5f),
                theme::rgb(theme::TEXT), view.current.c_str(), valueWidth);
            drawList->AddText(bold, theme::FONT_SMALL,
                ImVec2(valueX - 12.0f - size(theme::FONT_SMALL, label).x,
                    origin.y + (titleRow - size(theme::FONT_SMALL, label).y) * 0.5f),
                theme::rgb(theme::TEXT_MUTED), label.c_str());
        }

        // The columns share the width equally.
        const size_t count = view.columns.size();
        const float top    = origin.y + titleRow + gap;
        const float columnWidth =
            count == 0 ? inner
                       : (inner - columnGap * static_cast<float>(count - 1)) / static_cast<float>(count);
        for (size_t i = 0; i < count; ++i)
        {
            const DatePickerView::Column& column = view.columns[i];
            const bool selected                  = i == view.column;
            const float left = origin.x + static_cast<float>(i) * (columnWidth + columnGap);
            centred(column.header, theme::FONT_SMALL, ImVec2(left, top),
                ImVec2(left + columnWidth, top + headerRow), selected ? theme::TEXT : theme::TEXT_MUTED);

            // Up, the value, and down; touching one beside the value picks it.
            const float upTop    = top + headerRow + rowGap;
            const float valueTop = upTop + besideRow + rowGap;
            const float downTop  = valueTop + valueRow + rowGap;
            struct Row
            {
                float top;
                float height;
                int step;
            };
            const Row rows[] = {{upTop, besideRow, 1}, {valueTop, valueRow, 0}, {downTop, besideRow, -1}};
            for (const Row& row : rows)
            {
                const ImVec2 min(left, row.top);
                const ImVec2 max(left + columnWidth, row.top + row.height);
                ImGui::SetCursorScreenPos(min);
                ImGui::PushID(static_cast<int>(i * 3) + row.step + 1);
                if (ImGui::InvisibleButton("date", ImVec2(columnWidth, row.height)))
                {
                    view.column      = i;
                    view.stepRequest = row.step;
                }
                ImGui::PopID();
                if (row.step == 0)
                {
                    drawList->AddRectFilled(min, max, theme::rgb(theme::CARD), 12.0f);
                    if (selected)
                    {
                        widgets::drawSelectionOutline(drawList, min, max, 12.0f);
                    }
                    centred(column.value, 28.0f, min, max, theme::TEXT);
                }
                else
                {
                    centred(row.step > 0 ? column.up : column.down, theme::FONT_ROW_TITLE, min, max,
                        selected ? theme::TEXT_SOFT : theme::TEXT_MUTED);
                }
            }
        }

        widgets::drawButtonLegend({{"A", tr(Text::HintSelect)}, {"B", tr(Text::HintBack)},
            {"\xE2\x86\x91 / \xE2\x86\x93", tr(Text::HintChange)},
            // Roboto has no left and right arrows.
            {"< / >", tr(Text::HintDatePart)}});
        endModal();
    }
}
