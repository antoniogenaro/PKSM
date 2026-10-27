#include "SwitchUi.hpp"

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"

#include <switch.h>
#include <algorithm>
#include <cstdio>
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
        constexpr SDL_Color BACKGROUND{20, 27, 43, 255};
        constexpr ImVec4 SELECTED_COLOR{0.16f, 0.26f, 0.38f, 1.0f};
        constexpr ImVec4 ACCENT_COLOR{0.28f, 0.70f, 0.96f, 1.0f};
        constexpr ImVec4 ERROR_COLOR{1.0f, 0.45f, 0.35f, 1.0f};
        constexpr char POKEMON_SPRITESHEET_PATH[] = "romfs:/sprites/pkm_spritesheet.t3x";
        constexpr char BALL_SPRITESHEET_PATH[] = "romfs:/sprites/types_spritesheet.t3x";
        constexpr size_t BALL_SPRITE_BASE = 144;
        constexpr size_t MAX_BALL_SPRITE = 25;

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
            error = std::string("No se pudo iniciar SDL: ") + SDL_GetError();
            return false;
        }

        window_ = SDL_CreateWindow(
            "PKSM Switch", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_FULLSCREEN);
        if (!window_)
        {
            error = std::string("No se pudo crear la ventana: ") + SDL_GetError();
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
            error = std::string("No se pudo crear el renderizador: ") + SDL_GetError();
            return false;
        }
        if (!loadSpriteSheet(POKEMON_SPRITESHEET_PATH, "Pokémon", pokemonSpritesheet_,
                pokemonSprites_, error) ||
            !loadSpriteSheet(BALL_SPRITESHEET_PATH, "Poké Ball", ballSpritesheet_, ballSprites_,
                error))
        {
            return false;
        }
        if (ballSprites_.size() <= BALL_SPRITE_BASE + MAX_BALL_SPRITE)
        {
            error = "El spritesheet de Poké Ball no contiene todos los iconos.";
            return false;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        if (!ImGui_ImplSDL2_InitForSDLRenderer(window_, renderer_))
        {
            error = "No se pudo iniciar el backend SDL2 de Dear ImGui.";
            ImGui::DestroyContext();
            return false;
        }
        if (!ImGui_ImplSDLRenderer2_Init(renderer_))
        {
            error = "No se pudo iniciar el backend de renderizado de Dear ImGui.";
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
            error = "No se pudo abrir el spritesheet de " + name + ".";
            return false;
        }
        const std::vector<u8> data(
            (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (file.bad() || data.size() < 5)
        {
            error = "El spritesheet de " + name + " está incompleto.";
            return false;
        }

        u16 spriteCount = 0;
        if (!readU16(data, 0, spriteCount) || spriteCount == 0 || spriteCount > 2048)
        {
            error = "El spritesheet de " + name + " no tiene un encabezado válido.";
            return false;
        }
        const u8 dimensions = data[2];
        const size_t width = size_t{1} << ((dimensions & 0x07) + 3);
        const size_t height = size_t{1} << (((dimensions >> 3) & 0x07) + 3);
        if ((dimensions & 0x40) != 0 || data[3] != 0 || data[4] != 0)
        {
            error = "El formato del spritesheet de " + name + " no es compatible.";
            return false;
        }

        constexpr size_t tableOffset = 5;
        if (spriteCount > (data.size() - tableOffset) / 12)
        {
            error = "La tabla de sprites de " + name + " está incompleta.";
            return false;
        }
        const size_t textureOffset = tableOffset + static_cast<size_t>(spriteCount) * 12;
        const size_t textureSize = width * height * 4;
        std::vector<u8> textureData;
        if (!decodeLz11(data, textureOffset, textureSize, textureData))
        {
            error = "No se pudo descomprimir el spritesheet de " + name + ".";
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
                error = "La tabla de sprites de " + name + " está incompleta.";
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
                error = "La tabla de sprites de " + name + " contiene coordenadas inválidas.";
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
            error = "No se pudo crear la textura de " + name + ": " + SDL_GetError();
            return false;
        }
        if (SDL_UpdateTexture(texture, nullptr, pixels.data(), static_cast<int>(width * 4)) != 0 ||
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND) != 0)
        {
            error = "No se pudo cargar la textura de " + name + ": " + SDL_GetError();
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

    void SwitchUi::drawPokemonSprite(size_t index, const SDL_Rect& destination)
    {
        if (index == 0)
        {
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
        ImGui::SetNextWindowPos(ImVec2(28.0f, 28.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(1224.0f, 664.0f), ImGuiCond_Always);
        ImGui::Begin("PKSM Switch", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    }

    void SwitchUi::endFrame()
    {
        ImGui::End();
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer_, BACKGROUND.r, BACKGROUND.g, BACKGROUND.b, 255);
        SDL_RenderClear(renderer_);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer_);
        SDL_RenderPresent(renderer_);
    }

    void SwitchUi::drawStatusLine(bool dirty, const std::string& error)
    {
        if (dirty)
        {
            ImGui::TextColored(ACCENT_COLOR, "Cambios pendientes de exportar");
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

    void SwitchUi::drawFiles(
        const std::vector<std::string>& files, const std::vector<std::string>& gameNames,
        const std::vector<size_t>& gameMascots, size_t selected, const std::string& error)
    {
        (void)gameMascots;
        beginFrame();

        ImGui::TextUnformatted("Administrador de respaldos Pokémon");
        ImGui::Separator();
        ImGui::TextDisabled("A: abrir   |   +: salir");
        ImGui::Spacing();

        if (!error.empty())
        {
            ImGui::TextColored(ERROR_COLOR, "%s", error.c_str());
        }
        else if (files.empty())
        {
            ImGui::TextWrapped("No se encontraron respaldos compatibles.");
            ImGui::TextWrapped("Copia tus archivos a /switch/pksm/ en la tarjeta SD.");
        }
        else
        {
            ImGui::Text("%zu respaldos encontrados", files.size());
            ImGui::BeginChild("save-files", ImVec2(0.0f, -32.0f), ImGuiChildFlags_Borders);
            for (size_t i = 0; i < files.size(); ++i)
            {
                const std::string label =
                    i < gameNames.size() ? gameNames[i] + "  /  " + files[i] : files[i];
                ImGui::PushID(static_cast<int>(i));
                const ImVec2 itemPosition = ImGui::GetCursorScreenPos();
                if (i < gameMascots.size())
                {
                    drawPokemonSprite(gameMascots[i],
                        {static_cast<int>(itemPosition.x), static_cast<int>(itemPosition.y), 40, 40});
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 46.0f);
                }
                ImGui::Selectable(label.c_str(), i == selected, ImGuiSelectableFlags_SpanAllColumns,
                    ImVec2(0.0f, 40.0f));
                ImGui::PopID();
            }
            ImGui::EndChild();
        }
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 28.0f);
        ImGui::TextDisabled("Los guardados originales no se modifican hasta guardar los cambios.");
        endFrame();
    }

    void SwitchUi::drawGameMenu(const std::string& game, const std::string& path,
        const std::string& details, size_t selected, const std::string& message)
    {
        struct MenuOption
        {
            const char* label;
            const char* description;
            bool available;
        };
        static constexpr MenuOption options[] = {
            {"Almacenamiento", "Gestiona tus cajas y Pokémon", false},
            {"Editor Pokémon", "Edita los datos de tus Pokémon", true},
            {"Eventos", "Gestiona regalos misteriosos", false},
            {"Scripts", "Ejecuta scripts para el guardado", false},
            {"Bolsa", "Edita los objetos de la bolsa", false},
            {"Editor entrenador", "Edita los datos del entrenador", true},
        };

        beginFrame();

        ImGui::TextUnformatted(game.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("/ %s", path.c_str());
        ImGui::Separator();
        ImGui::TextWrapped("%s", details.c_str());
        ImGui::Spacing();

        constexpr float cardHeight = 118.0f;
        constexpr float cardGap    = 12.0f;
        const float cardWidth = (ImGui::GetContentRegionAvail().x - cardGap) / 2.0f;
        for (size_t i = 0; i < sizeof(options) / sizeof(options[0]); ++i)
        {
            if (i % 2 != 0)
            {
                ImGui::SameLine(0.0f, cardGap);
            }

            ImGui::PushID(static_cast<int>(i));
            if (i == selected)
            {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, SELECTED_COLOR);
                ImGui::PushStyleColor(ImGuiCol_Border,
                    options[i].available ? ACCENT_COLOR :
                                           ImVec4(0.35f, 0.40f, 0.46f, 1.0f));
            }
            ImGui::BeginChild("menu-option", ImVec2(cardWidth, cardHeight),
                ImGuiChildFlags_Borders);
            if (options[i].available)
            {
                ImGui::TextUnformatted(options[i].label);
                ImGui::Spacing();
                ImGui::TextWrapped("%s", options[i].description);
            }
            else
            {
                ImGui::TextDisabled("%s", options[i].label);
                ImGui::Spacing();
                ImGui::TextDisabled("%s", options[i].description);
                ImGui::SetCursorPosY(cardHeight - 30.0f);
                ImGui::TextDisabled("Próximamente");
            }
            ImGui::EndChild();
            if (i == selected)
            {
                ImGui::PopStyleColor(2);
            }
            ImGui::PopID();
        }

        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 52.0f);
        if (!message.empty())
        {
            ImGui::TextColored(ACCENT_COLOR, "%s", message.c_str());
        }
        else
        {
            ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight()));
        }
        ImGui::TextDisabled("A: abrir    B: volver    Y: exportar    X: descartar    +: salir");
        endFrame();
    }

    void SwitchUi::drawPokemonSlots(const std::string& game, const std::string& path,
        const std::string& section, const std::vector<Field>& fields, size_t selected, bool dirty,
        const std::string& error)
    {
        beginFrame();
        ImGui::Text("Editor de Pokémon  /  %s  /  %s  /  %s", game.c_str(), path.c_str(),
            section.c_str());
        ImGui::Separator();

        constexpr float summaryWidth = 290.0f;
        constexpr float gap = 12.0f;
        const float gridWidth = ImGui::GetContentRegionAvail().x - summaryWidth - gap;
        ImGui::BeginChild("slot-grid", ImVec2(gridWidth, -58.0f), ImGuiChildFlags_Borders);
        const size_t columns = fields.size() <= 6 ? 3 : 6;
        const float tileGap = 6.0f;
        const float tileWidth =
            (ImGui::GetContentRegionAvail().x - tileGap * static_cast<float>(columns - 1)) /
            static_cast<float>(columns);
        for (size_t i = 0; i < fields.size(); ++i)
        {
            if (i % columns != 0)
            {
                ImGui::SameLine(0.0f, tileGap);
            }
            ImGui::PushID(static_cast<int>(i));
            if (i == selected)
            {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, SELECTED_COLOR);
                ImGui::PushStyleColor(ImGuiCol_Border, ACCENT_COLOR);
            }
            ImGui::BeginChild("slot", ImVec2(tileWidth, 68.0f), ImGuiChildFlags_Borders);
            const ImVec2 tilePosition = ImGui::GetCursorScreenPos();
            drawPokemonSprite(fields[i].spriteIndex,
                {static_cast<int>(tilePosition.x + 4.0f),
                    static_cast<int>(tilePosition.y + 6.0f), 48, 48});
            ImGui::SetCursorPosX(60.0f);
            ImGui::TextDisabled("%zu", i + 1);
            ImGui::TextWrapped("%s", fields[i].value.c_str());
            ImGui::EndChild();
            if (i == selected)
            {
                ImGui::PopStyleColor(2);
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
        ImGui::SameLine(0.0f, gap);
        ImGui::BeginChild("slot-summary", ImVec2(summaryWidth, -58.0f), ImGuiChildFlags_Borders);
        ImGui::TextColored(ACCENT_COLOR, "Pokémon seleccionado");
        ImGui::Separator();
        if (selected < fields.size())
        {
            const ImVec2 spritePosition = ImGui::GetCursorScreenPos();
            drawPokemonSprite(fields[selected].spriteIndex,
                {static_cast<int>(spritePosition.x + 35.0f),
                    static_cast<int>(spritePosition.y), 180, 130});
            ImGui::Dummy(ImVec2(0.0f, 132.0f));
            ImGui::TextDisabled("%s", fields[selected].label.c_str());
            ImGui::Spacing();
            ImGui::TextWrapped("%s", fields[selected].value.c_str());
        }
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 120.0f);
        ImGui::TextDisabled("A: abrir editor");
        ImGui::EndChild();
        drawStatusLine(dirty, error);
        ImGui::TextDisabled("D-pad: mover    L/R: cajas/equipo    A: editar    Y: exportar    B: volver");
        endFrame();
    }

    void SwitchUi::drawPokemonEditor(const std::string& game, const std::string& path,
        const std::string& pokemon, size_t speciesIndex, const std::vector<Field>& fields,
        size_t selected, bool dirty, const std::string& error)
    {
        beginFrame();
        ImGui::Text("Editor de Pokémon  /  %s  /  %s  /  %s", game.c_str(), path.c_str(),
            pokemon.c_str());
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
        const auto valueFor = [&fields](const char* label) -> std::string
        {
            const auto field = std::find_if(fields.begin(), fields.end(),
                [label](const Field& candidate) { return candidate.label == label; });
            return field == fields.end() ? std::string{} : field->value;
        };
        ImGui::TextColored(ACCENT_COLOR, "Resumen del Pokémon");
        ImGui::Separator();
        const ImVec2 spritePosition = ImGui::GetCursorScreenPos();
        drawPokemonSprite(speciesIndex,
            {static_cast<int>(spritePosition.x + 70.0f),
                static_cast<int>(spritePosition.y), 150, 112});
        ImGui::Dummy(ImVec2(0.0f, 114.0f));
        ImGui::TextWrapped("%s", valueFor("Especie").c_str());
        ImGui::TextDisabled("%s", valueFor("Apodo").c_str());
        ImGui::Spacing();
        for (const char* label : {"Nivel", "Naturaleza", "Género", "Variocolor"})
        {
            ImGui::TextDisabled("%s", label);
            ImGui::SameLine(120.0f);
            ImGui::TextWrapped("%s", valueFor(label).c_str());
        }
        ImGui::EndChild();
        drawStatusLine(dirty, error);
        ImGui::TextDisabled("D-pad: mover/cambiar    A: editar    Y: exportar    B: volver    X: descartar");
        endFrame();
    }

    void SwitchUi::drawTrainer(const std::string& game, const std::string& path,
        const std::vector<Field>& fields, size_t selected, bool dirty, const std::string& error)
    {
        beginFrame();
        ImGui::Text("Ficha del entrenador  /  %s  /  %s", game.c_str(), path.c_str());
        ImGui::Separator();
        ImGui::BeginChild("trainer-fields", ImVec2(0.0f, -58.0f), ImGuiChildFlags_Borders);
        for (size_t i = 0; i < fields.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            if (i == selected)
            {
                ImGui::PushStyleColor(ImGuiCol_Header, SELECTED_COLOR);
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, SELECTED_COLOR);
            }
            ImGui::Selectable(fields[i].label.c_str(), i == selected,
                ImGuiSelectableFlags_SpanAllColumns, ImVec2(0.0f, 42.0f));
            ImGui::SameLine(420.0f);
            ImGui::TextWrapped("%s", fields[i].value.c_str());
            if (i == selected)
            {
                ImGui::PopStyleColor(2);
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
        drawStatusLine(dirty, error);
        ImGui::TextDisabled("D-pad: mover/cambiar    A: nombre    Y: exportar    B: menú    X: descartar");
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

    bool SwitchUi::selectOption(const std::string& title,
        const std::vector<std::string>& options, size_t current, size_t& selected,
        const std::vector<size_t>& spriteIndices)
    {
        if (options.empty())
        {
            return false;
        }

        constexpr size_t visibleRows = 9;
        const size_t lastStart = options.size() > visibleRows ? options.size() - visibleRows : 0;
        size_t index = std::min(current, options.size() - 1);
        while (appletMainLoop())
        {
            const Action action = pollAction();
            if (action == Action::Confirm)
            {
                selected = index;
                return true;
            }
            if (action == Action::Back || action == Action::Quit)
            {
                return false;
            }
            if (action == Action::Up)
            {
                if (index > 0)
                {
                    --index;
                }
            }
            else if (action == Action::Down)
            {
                if (index + 1 < options.size())
                {
                    ++index;
                }
            }
            else if (action == Action::Left)
            {
                index = index > 10 ? index - 10 : 0;
            }
            else if (action == Action::Right)
            {
                index = std::min(options.size() - 1, index + 10);
            }
            else if (action == Action::PreviousPage)
            {
                index = index > 100 ? index - 100 : 0;
            }
            else if (action == Action::NextPage)
            {
                index = std::min(options.size() - 1, index + 100);
            }

            const size_t first = std::min(
                index > visibleRows / 2 ? index - visibleRows / 2 : 0, lastStart);
            beginFrame();
            ImGui::TextUnformatted(title.c_str());
            ImGui::Separator();
            ImGui::TextDisabled("Selecciona una opción");
            ImGui::BeginChild("options", ImVec2(0.0f, -58.0f), ImGuiChildFlags_Borders);
            const size_t last = std::min(options.size(), first + visibleRows);
            for (size_t i = first; i < last; ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                if (i == index)
                {
                    ImGui::PushStyleColor(ImGuiCol_Header, SELECTED_COLOR);
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, SELECTED_COLOR);
                }
                if (i < spriteIndices.size())
                {
                    const ImVec2 itemPosition = ImGui::GetCursorScreenPos();
                    drawBallSprite(spriteIndices[i],
                        {static_cast<int>(itemPosition.x + 8.0f),
                            static_cast<int>(itemPosition.y + 4.0f), 32, 32});
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 48.0f);
                }
                if (ImGui::Selectable(options[i].c_str(), i == index,
                        ImGuiSelectableFlags_SpanAllColumns, ImVec2(0.0f, 40.0f)))
                {
                    index = i;
                }
                if (i == index)
                {
                    ImGui::PopStyleColor(2);
                }
                ImGui::PopID();
            }
            ImGui::EndChild();
            ImGui::TextDisabled("%zu / %zu", index + 1, options.size());
            ImGui::SameLine();
            ImGui::TextDisabled("Flechas: mover    L/R: avanzar 100 opciones    A: seleccionar    B: volver");
            endFrame();
            SDL_Delay(16);
        }
        return false;
    }

    bool SwitchUi::selectNature(
        const std::vector<std::string>& natures, size_t current, size_t& selected)
    {
        static constexpr size_t natureGrid[5][5] = {
            {0, 1, 2, 3, 4},
            {5, 6, 7, 8, 9},
            {10, 11, 12, 13, 14},
            {15, 16, 17, 18, 19},
            {20, 21, 22, 23, 24},
        };
        static constexpr const char* columnLabels[] = {
            "Neutra", "-Ataque", "-Defensa", "-Velocidad", "-At. Esp.", "-Def. Esp."};
        static constexpr const char* rowLabels[] = {
            "+Ataque", "+Defensa", "+Velocidad", "+At. Esp.", "+Def. Esp."};
        if (natures.size() != 25)
        {
            return false;
        }

        size_t row = 0;
        size_t column = 0;
        bool found = false;
        for (size_t y = 0; y < 5 && !found; ++y)
        {
            for (size_t x = 0; x < 5; ++x)
            {
                if (natureGrid[y][x] == current)
                {
                    row = y;
                    column = x;
                    found = true;
                    break;
                }
            }
        }
        if (!found)
        {
            row = 0;
            column = 0;
        }

        while (appletMainLoop())
        {
            const Action action = pollAction();
            if (action == Action::Confirm)
            {
                selected = natureGrid[row][column];
                return true;
            }
            if (action == Action::Back || action == Action::Quit)
            {
                return false;
            }
            if (action == Action::Up && row > 0)
            {
                --row;
            }
            else if (action == Action::Down && row < 4)
            {
                ++row;
            }
            else if (action == Action::Left && column > 0)
            {
                --column;
            }
            else if (action == Action::Right && column < 4)
            {
                ++column;
            }

            beginFrame();
            ImGui::TextUnformatted("Naturaleza");
            ImGui::Separator();
            ImGui::TextDisabled("Selecciona una naturaleza según sus estadísticas");
            ImGui::BeginChild("nature-grid", ImVec2(0.0f, -58.0f), ImGuiChildFlags_Borders);
            constexpr float rowLabelWidth = 140.0f;
            constexpr float cellGap = 4.0f;
            const float cellWidth =
                (ImGui::GetContentRegionAvail().x - rowLabelWidth - cellGap * 5.0f) / 5.0f;
            ImGui::TextColored(ImVec4(1.0f, 0.86f, 0.36f, 1.0f), "%s", columnLabels[0]);
            for (size_t x = 0; x < 5; ++x)
            {
                ImGui::SameLine(rowLabelWidth + cellGap + static_cast<float>(x) * (cellWidth + cellGap));
                ImGui::TextWrapped("%s", columnLabels[x + 1]);
            }
            ImGui::Separator();
            for (size_t y = 0; y < 5; ++y)
            {
                ImGui::TextUnformatted(rowLabels[y]);
                for (size_t x = 0; x < 5; ++x)
                {
                    const size_t natureIndex = natureGrid[y][x];
                    const bool isSelected = row == y && column == x;
                    ImGui::SameLine(rowLabelWidth + cellGap + static_cast<float>(x) * (cellWidth + cellGap));
                    ImGui::PushID(static_cast<int>(natureIndex));
                    if (isSelected)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Button, SELECTED_COLOR);
                        ImGui::PushStyleColor(ImGuiCol_Border, ACCENT_COLOR);
                    }
                    if (ImGui::Button(natures[natureIndex].c_str(), ImVec2(cellWidth, 44.0f)))
                    {
                        row = y;
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
            ImGui::TextDisabled("Flechas: mover    A: seleccionar    B: volver");
            endFrame();
            SDL_Delay(16);
        }
        return false;
    }

    bool SwitchUi::editTrainerName(
        const std::string& initial, std::string& result, size_t maxLength)
    {
        return editText("Nombre del entrenador", initial, result, maxLength);
    }
}
