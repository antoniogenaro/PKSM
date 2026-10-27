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

        // Box screen colours from the canvas, as 0xRRGGBB.
        constexpr unsigned EMPTY_SLOT = 0x1A1E2B;
        constexpr unsigned DETAIL_ART = 0x3A4160;
        constexpr unsigned PILL       = 0x2A3047;
        constexpr unsigned MALE       = 0x8FB8FF;
        constexpr unsigned FEMALE     = 0xFF8FB8;
        constexpr unsigned SHINY      = 0xFFD84D;
        constexpr unsigned RAISED     = 0xFF7A6B;
        constexpr unsigned LOWERED    = 0x7DB2FF;

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

    void SwitchUi::beginFrame()
    {
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io    = ImGui::GetIO();
        io.DisplaySize = ImVec2(static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT));
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
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer_, (theme::BACKGROUND >> 16) & 0xFF,
            (theme::BACKGROUND >> 8) & 0xFF, theme::BACKGROUND & 0xFF, 255);
        SDL_RenderClear(renderer_);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer_);
        SDL_RenderPresent(renderer_);
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
        if (buttons & HidNpadButton_Up)
        {
            return Action::Up;
        }
        if (buttons & HidNpadButton_Down)
        {
            return Action::Down;
        }
        if (buttons & HidNpadButton_Left)
        {
            return Action::Left;
        }
        if (buttons & HidNpadButton_Right)
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
                drawList->AddCircleFilled(ImVec2(x + dotSize * 0.5f, y + size.y * 0.5f),
                    dotSize * 0.5f, theme::rgb(line.dotColor), 12);
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

        widgets::drawStatus(drawList, ImVec2(min.x + infoPadding, max.y - 12.0f),
            saveCountText(game.saveCount).c_str(),
            game.saveCount > 0 ? theme::POSITIVE : theme::NEUTRAL);

        if (selected)
        {
            widgets::drawSelectionOutline(drawList, min, max, theme::CARD_RADIUS);
        }
        return touched;
    }

    void SwitchUi::drawGameList(const GameGrid& grid, const std::vector<GameCard>& games,
        size_t& selected, const std::string& error)
    {
        beginFrame();
        widgets::drawHeader(tr(Text::GameListPrompt));
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
        sectionGap();
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

        widgets::drawButtonLegend({{"A", tr(Text::HintOpen)}, {"B", tr(Text::HintQuit)}});
        endFrame();
    }

    void SwitchUi::drawSaveList(const GameCard& game, const std::string& group,
        const std::vector<std::string>& files, size_t& selected, const std::string& error)
    {
        beginFrame();
        widgets::drawHeader(tr(Text::SaveListTitle));
        sectionGap();

        drawGamePanel(game, group,
            {{saveCountText(game.saveCount), theme::FONT_CAPTION, theme::TEXT_MUTED, false,
                game.saveCount > 0 ? theme::POSITIVE : theme::NEUTRAL}});

        ImGui::SameLine(0.0f, theme::PANEL_GAP);
        widgets::beginPanel("save-files", ImVec2(0.0f, -SCREEN_BOTTOM_SPACE));
        if (!error.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ERROR_COLOR);
            ImGui::TextWrapped("%s", error.c_str());
            ImGui::PopStyleColor();
        }
        if (files.empty())
        {
            widgets::text(tr(Text::NoSavesFound), theme::FONT_ROW_TITLE, theme::TEXT, true);
            ImGui::PushStyleColor(ImGuiCol_Text, theme::vec(theme::TEXT_MUTED));
            ImGui::TextWrapped("%s", tr(Text::NoSavesHint));
            ImGui::PopStyleColor();
        }
        for (size_t i = 0; i < files.size(); ++i)
        {
            // The file name; the folder it is in, which names the trainer and the date of the
            // backup; and the full path.
            const size_t slash     = files[i].find_last_of('/');
            const std::string name =
                slash == std::string::npos ? files[i] : files[i].substr(slash + 1);
            const std::string folder =
                slash == std::string::npos ? std::string{} : backupLabel(files[i].substr(0, slash));
            const std::string path = std::string(storage::SAVE_DIRECTORY) + "/" + files[i];
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
        if (files.empty())
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
            {{save.file, 15.0f, theme::BUTTON_FACE, true, 0},
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
            {Text::MenuStorage, Text::MenuStorageHint, icons::Icon::Bank, false},
            {Text::MenuBag, Text::MenuBagHint, icons::Icon::Bag, false},
            {Text::MenuEvents, Text::MenuEventsHint, icons::Icon::Gift, false},
            {Text::MenuTrainerEditor, Text::MenuTrainerEditorHint, icons::Icon::Trainer, true},
            {Text::MenuOther, Text::MenuOtherHint, icons::Icon::More, false},
        };
        constexpr size_t dividerAfter = 2;

        beginFrame();
        widgets::drawHeader(tr(Text::ChooseAction));
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
            {"Y", tr(Text::HintExport)}, {"X", tr(Text::HintDiscard)}, {"+", tr(Text::HintQuit)}});
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

    void SwitchUi::drawPokemonDetail(const PokemonDetail& detail)
    {
        constexpr float sectionGap = 12.0f;
        constexpr float artSize    = 170.0f;
        constexpr float artGap     = 16.0f;
        constexpr float lineGap    = 6.0f;
        constexpr float iconSize   = 24.0f;

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
        drawPokemonSprite(detail.species, {static_cast<int>(origin.x + 12.0f),
                                              static_cast<int>(origin.y + 12.0f),
                                              static_cast<int>(artSize - 24.0f),
                                              static_cast<int>(artSize - 24.0f)},
            detail.formSprite);

        constexpr float numberSize = 14.0f;
        constexpr float nameSize   = 30.0f;
        constexpr float pillHeight = 32.0f;
        const float textLeft       = origin.x + artSize + artGap;
        const float textWidth      = right - textLeft;
        float columnHeight         = height(bold, numberSize, detail.number) + lineGap +
                             height(bold, nameSize, detail.name) + lineGap + 26.0f + lineGap +
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
        const float levelHeight = height(regular, theme::FONT_BODY, detail.level);
        drawList->AddText(regular, theme::FONT_BODY, ImVec2(textLeft, y + (26.0f - levelHeight) * 0.5f),
            theme::rgb(theme::TEXT_SOFT), detail.level.c_str());
        const float ballX = textLeft + width(regular, theme::FONT_BODY, detail.level) + 10.0f;
        drawBallSprite(detail.ball, {static_cast<int>(ballX), static_cast<int>(y), 26, 26});
        y += 26.0f + lineGap;

        if (!detail.nickname.empty())
        {
            widgets::drawTextEllipsis(drawList, regular, theme::FONT_SMALL, ImVec2(textLeft, y),
                theme::rgb(theme::TEXT_MUTED), detail.nickname.c_str(), textWidth);
            y += height(regular, theme::FONT_SMALL, detail.nickname) + lineGap;
        }

        // Type pills: a dot in the type's colour and its name.
        float pillX = textLeft;
        for (const auto& [name, type] : detail.types)
        {
            const float pillWidth = 4.0f + 24.0f + 8.0f + width(bold, theme::FONT_SMALL, name) + 14.0f;
            drawList->AddRectFilled(ImVec2(pillX, y), ImVec2(pillX + pillWidth, y + pillHeight),
                theme::rgb(PILL), pillHeight * 0.5f);
            const ImVec2 dot(pillX + 4.0f + 12.0f, y + pillHeight * 0.5f);
            drawList->AddCircleFilled(dot, 12.0f, typeRing(type), 24);
            drawList->AddCircleFilled(dot, 7.0f, theme::rgb(typeColor(type)), 24);
            drawList->AddText(bold, theme::FONT_SMALL,
                ImVec2(pillX + 36.0f,
                    y + (pillHeight - height(bold, theme::FONT_SMALL, name)) * 0.5f),
                theme::rgb(theme::BUTTON_FACE), name.c_str());
            pillX += pillWidth + 8.0f;
        }

        // Trainer, ID, item, ability, nature and origin game, two to a row.
        constexpr float infoHeight = 52.0f;
        constexpr float infoGap    = 8.0f;
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
            const float labelHeight  = height(bold, theme::FONT_PATH, label);
            const float valueHeight  = height(bold, theme::FONT_BODY, value);
            const float lineTop      = min.y + (infoHeight - labelHeight - 1.0f - valueHeight) * 0.5f;
            widgets::drawTextEllipsis(drawList, bold, theme::FONT_PATH, ImVec2(min.x + 14.0f, lineTop),
                theme::rgb(theme::TEXT_MUTED), label.c_str(), infoWidth - 28.0f);
            widgets::drawTextEllipsis(drawList, bold, theme::FONT_BODY,
                ImVec2(min.x + 14.0f, lineTop + labelHeight + 1.0f), theme::rgb(theme::TEXT),
                value.c_str(), infoWidth - 28.0f);
        }
        const size_t infoRows = (detail.info.size() + 1) / 2;
        top += static_cast<float>(infoRows) * infoHeight +
               static_cast<float>(infoRows > 0 ? infoRows - 1 : 0) * infoGap + sectionGap;

        // The IVs, with the stats the nature raises in red and lowers in blue.
        constexpr float ivGap    = 6.0f;
        constexpr float ivHeight = 58.0f;
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
            widgets::drawTextEllipsis(drawList, bold, theme::FONT_PATH, ImVec2(min.x + 8.0f, min.y + 8.0f),
                theme::rgb(color), label.c_str(), ivWidth - 16.0f);
            drawList->AddText(bold, 20.0f,
                ImVec2(min.x + 8.0f, min.y + ivHeight - 8.0f - height(bold, 20.0f, iv.value)),
                theme::rgb(theme::TEXT), iv.value.c_str());
        }
        top += ivHeight + sectionGap;

        // The four moves.
        constexpr float moveGap    = 8.0f;
        constexpr float moveHeight = 44.0f;
        const float moveCount      = static_cast<float>(std::max<size_t>(detail.moves.size(), 1));
        const float moveWidth      = (available.x - moveGap * (moveCount - 1.0f)) / moveCount;
        for (size_t i = 0; i < detail.moves.size(); ++i)
        {
            const ImVec2 min(origin.x + static_cast<float>(i) * (moveWidth + moveGap), top);
            drawList->AddRectFilled(min, ImVec2(min.x + moveWidth, min.y + moveHeight),
                theme::rgb(PILL), 12.0f);
            const std::string& move = detail.moves[i];
            widgets::drawTextEllipsis(drawList, bold, theme::FONT_SMALL,
                ImVec2(min.x + 8.0f, min.y + (moveHeight - height(bold, theme::FONT_SMALL, move)) * 0.5f),
                theme::rgb(theme::BUTTON_FACE), move.c_str(), moveWidth - 16.0f);
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
        widgets::drawHeader(tr(Text::MenuPokemonEditor));
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
                theme::rgb(theme::BACKGROUND, 140), 24);
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

        // Y moves Pokémon here; the game menu exports.
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

    void SwitchUi::drawPokemonEditor(const std::string& game, const std::string& path,
        const std::string& pokemon, size_t speciesIndex, size_t formSprite,
        const std::vector<Field>& fields,
        size_t selected, bool dirty, const std::string& error)
    {
        beginFrame();
        ImGui::TextUnformatted(tr(Text::PokemonEditorHeader, {game, path, pokemon}).c_str());
        ImGui::Separator();

        constexpr float summaryWidth = 315.0f;
        constexpr float gap = 12.0f;
        const float listWidth = ImGui::GetContentRegionAvail().x - summaryWidth - gap;
        ImGui::BeginChild("field-list", ImVec2(listWidth, -58.0f), ImGuiChildFlags_Borders);
        constexpr size_t visibleRows = 7;
        const size_t first = selected >= visibleRows ? selected - visibleRows + 1 : 0;
        const size_t last = std::min(fields.size(), first + visibleRows);
        for (size_t i = first; i < last; ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            if (i == selected)
            {
                ImGui::PushStyleColor(ImGuiCol_Header, SELECTED_COLOR);
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, SELECTED_COLOR);
            }
            ImGui::Selectable(fields[i].label.c_str(), i == selected,
                ImGuiSelectableFlags_SpanAllColumns, ImVec2(0.0f, 42.0f));
            ImGui::SameLine(340.0f);
            ImGui::TextWrapped("%s", fields[i].value.c_str());
            if (i == selected)
            {
                ImGui::PopStyleColor(2);
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
        ImGui::SameLine(0.0f, gap);
        ImGui::BeginChild("pokemon-summary", ImVec2(summaryWidth, -58.0f), ImGuiChildFlags_Borders);
        const auto valueFor = [&fields](Text key) -> std::string
        {
            const std::string_view label = tr(key);
            const auto field = std::find_if(fields.begin(), fields.end(),
                [label](const Field& candidate) { return candidate.label == label; });
            return field == fields.end() ? std::string{} : field->value;
        };
        ImGui::TextColored(ACCENT_COLOR, "%s", tr(Text::PokemonSummaryTitle));
        ImGui::Separator();
        const ImVec2 spritePosition = ImGui::GetCursorScreenPos();
        drawPokemonSprite(speciesIndex,
            {static_cast<int>(spritePosition.x + 70.0f),
                static_cast<int>(spritePosition.y), 150, 112},
            formSprite);
        ImGui::Dummy(ImVec2(0.0f, 114.0f));
        ImGui::TextWrapped("%s", valueFor(Text::FieldSpecies).c_str());
        ImGui::TextDisabled("%s", valueFor(Text::FieldNickname).c_str());
        ImGui::Spacing();
        for (const Text label :
            {Text::FieldLevel, Text::FieldNature, Text::FieldGender, Text::FieldShiny})
        {
            ImGui::TextDisabled("%s", tr(label));
            ImGui::SameLine(120.0f);
            ImGui::TextWrapped("%s", valueFor(label).c_str());
        }
        ImGui::EndChild();
        drawStatusLine(dirty, error);
        ImGui::TextDisabled("%s", tr(Text::PokemonEditorControls));
        endFrame();
    }

    void SwitchUi::drawTrainer(const SaveSummary& save, const std::vector<Field>& fields,
        size_t selected, const std::string& message)
    {
        // Three rows hold the trainer's fields; more fields add rows.
        constexpr size_t minimumRows = 3;
        constexpr float cardGap      = 12.0f;

        beginFrame();
        widgets::drawHeader(tr(Text::MenuTrainerEditor));
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

        widgets::drawButtonLegend({{"A", tr(Text::HintEdit)}, {"L / R", tr(Text::HintChange)},
            {"Y", tr(Text::HintExport)}, {"X", tr(Text::HintDiscard)}, {"B", tr(Text::HintBack)}});
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

    void SwitchUi::drawOptionPicker(const std::string& title,
        const std::vector<std::string>& options, const std::vector<size_t>& spriteIndices,
        size_t first, size_t visibleRows, size_t& index)
    {
        beginFrame();
        ImGui::TextUnformatted(title.c_str());
        ImGui::Separator();
        ImGui::TextDisabled("%s", tr(Text::ChooseOption));
        ImGui::BeginChild("options", ImVec2(0.0f, -58.0f), ImGuiChildFlags_Borders);
        const size_t last = std::min(options.size(), first + visibleRows);
        for (size_t i = first; i < last; ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            const bool selected = i == index;
            if (selected)
            {
                ImGui::PushStyleColor(ImGuiCol_Header, SELECTED_COLOR);
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, SELECTED_COLOR);
            }
            if (i < spriteIndices.size())
            {
                const ImVec2 itemPosition = ImGui::GetCursorScreenPos();
                drawBallSprite(spriteIndices[i], {static_cast<int>(itemPosition.x + 8.0f),
                                                     static_cast<int>(itemPosition.y + 4.0f), 32, 32});
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 48.0f);
            }
            if (ImGui::Selectable(options[i].c_str(), selected,
                    ImGuiSelectableFlags_SpanAllColumns, ImVec2(0.0f, 40.0f)))
            {
                index = i;
            }
            if (selected)
            {
                ImGui::PopStyleColor(2);
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
        ImGui::TextDisabled("%zu / %zu", index + 1, options.size());
        ImGui::SameLine();
        ImGui::TextDisabled("%s", tr(Text::OptionPickerControls));
        endFrame();
    }

    void SwitchUi::drawNaturePicker(
        const std::vector<std::string>& natures, size_t& row, size_t& column)
    {
        // Rows raise a stat and columns lower one, in this order; the diagonal is neutral.
        static constexpr Text stats[] = {Text::StatAttack, Text::StatDefense, Text::StatSpeed,
            Text::StatSpAttack, Text::StatSpDefense};

        beginFrame();
        std::string raised[5];
        std::string lowered[5];
        // Wide enough for the longest raised stat, which can be longer in some languages.
        float rowLabelWidth = 140.0f;
        for (size_t i = 0; i < 5; ++i)
        {
            raised[i]     = std::string("+") + tr(stats[i]);
            lowered[i]    = std::string("-") + tr(stats[i]);
            rowLabelWidth = std::max(rowLabelWidth,
                ImGui::CalcTextSize(raised[i].c_str()).x + ImGui::GetStyle().ItemSpacing.x);
        }
        ImGui::TextUnformatted(tr(Text::FieldNature));
        ImGui::Separator();
        ImGui::TextDisabled("%s", tr(Text::NaturePickerHint));
        ImGui::BeginChild("nature-grid", ImVec2(0.0f, -58.0f), ImGuiChildFlags_Borders);
        constexpr float cellGap = 4.0f;
        const float cellWidth =
            (ImGui::GetContentRegionAvail().x - rowLabelWidth - cellGap * 5.0f) / 5.0f;
        ImGui::TextColored(ImVec4(1.0f, 0.86f, 0.36f, 1.0f), "%s", tr(Text::NatureNeutral));
        for (size_t x = 0; x < 5; ++x)
        {
            ImGui::SameLine(rowLabelWidth + cellGap + static_cast<float>(x) * (cellWidth + cellGap));
            ImGui::TextWrapped("%s", lowered[x].c_str());
        }
        ImGui::Separator();
        for (size_t y = 0; y < 5; ++y)
        {
            ImGui::TextUnformatted(raised[y].c_str());
            for (size_t x = 0; x < 5; ++x)
            {
                const size_t natureIndex = y * 5 + x;
                const bool isSelected    = row == y && column == x;
                ImGui::SameLine(rowLabelWidth + cellGap + static_cast<float>(x) * (cellWidth + cellGap));
                ImGui::PushID(static_cast<int>(natureIndex));
                if (isSelected)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, SELECTED_COLOR);
                    ImGui::PushStyleColor(ImGuiCol_Border, ACCENT_COLOR);
                }
                if (ImGui::Button(natures[natureIndex].c_str(), ImVec2(cellWidth, 44.0f)))
                {
                    row    = y;
                    column = x;
                }
                if (isSelected)
                {
                    ImGui::PopStyleColor(2);
                }
                ImGui::PopID();
            }
        }
        ImGui::EndChild();
        ImGui::TextDisabled("%s", tr(Text::NaturePickerControls));
        endFrame();
    }
}
