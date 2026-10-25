#include "SwitchUi.hpp"

#include <switch.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

namespace pksm::switchui
{
    namespace
    {
        constexpr int SCREEN_WIDTH = 1280;
        constexpr int SCREEN_HEIGHT = 720;
        constexpr SDL_Color BACKGROUND{20, 27, 43, 255};
        constexpr SDL_Color PANEL{31, 42, 63, 255};
        constexpr SDL_Color TABLE_HEADER{25, 34, 52, 255};
        constexpr SDL_Color PANEL_BORDER{61, 82, 112, 255};
        constexpr SDL_Color ROW_ALT{31, 42, 63, 255};
        constexpr SDL_Color ROW_DIVIDER{61, 82, 112, 255};
        constexpr SDL_Color TEXT{236, 241, 248, 255};
        constexpr SDL_Color MUTED{166, 181, 202, 255};
        constexpr SDL_Color ACCENT{91, 190, 170, 255};
        constexpr SDL_Color SELECTED{50, 91, 111, 255};
        constexpr SDL_Color YELLOW{255, 220, 92, 255};
        constexpr SDL_Color ERROR{255, 163, 145, 255};
        constexpr char FONT_PATH[] = "romfs:/fonts/Roboto.ttf";
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
                        length = ((static_cast<size_t>(first & 0x0F) << 4) | (second >> 4)) +
                                 0x11;
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
                        distance =
                            ((static_cast<size_t>(first & 0x0F) << 8) | input[cursor++]) + 1;
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

        void removeLastUtf8Character(std::string& text)
        {
            if (text.empty())
            {
                return;
            }
            size_t index = text.size() - 1;
            while (index > 0 && (static_cast<unsigned char>(text[index]) & 0xC0) == 0x80)
            {
                --index;
            }
            text.erase(index);
        }
    }

    SwitchUi::~SwitchUi()
    {
        if (pokemonSpritesheet_)
        {
            SDL_DestroyTexture(pokemonSpritesheet_);
        }
        if (ballSpritesheet_)
        {
            SDL_DestroyTexture(ballSpritesheet_);
        }
        for (const auto& entry : textCache_)
        {
            SDL_DestroyTexture(entry.second);
        }
        if (headingFont_)
        {
            TTF_CloseFont(headingFont_);
        }
        if (bodyFont_)
        {
            TTF_CloseFont(bodyFont_);
        }
        if (renderer_)
        {
            SDL_DestroyRenderer(renderer_);
        }
        if (window_)
        {
            SDL_DestroyWindow(window_);
        }
        TTF_Quit();
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

        if (TTF_Init() != 0)
        {
            error = std::string("No se pudo iniciar SDL_ttf: ") + TTF_GetError();
            return false;
        }
        headingFont_ = TTF_OpenFont(FONT_PATH, 36);
        bodyFont_ = TTF_OpenFont(FONT_PATH, 24);
        if (!headingFont_ || !bodyFont_)
        {
            error = std::string("No se pudo cargar la fuente: ") + TTF_GetError();
            return false;
        }
        TTF_SetFontHinting(headingFont_, TTF_HINTING_NORMAL);
        TTF_SetFontHinting(bodyFont_, TTF_HINTING_NORMAL);
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

        const size_t tableOffset = 5;
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

            const auto toPixelX = [width](u16 coordinate) {
                return static_cast<int>(static_cast<size_t>(coordinate) * width / 1024);
            };
            const auto toPixelY = [height](u16 coordinate) {
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
            sprites.push_back({{sourceX, sourceY, sourceWidth, sourceHeight}, spriteWidth,
                spriteHeight, rotated});
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
                const size_t source =
                    ((tileY * tilesPerRow + tileX) * 64 + morton) * 4;
                const size_t destination = (y * width + x) * 4;
                pixels[destination] = textureData[source + 3];
                pixels[destination + 1] = textureData[source + 2];
                pixels[destination + 2] = textureData[source + 1];
                pixels[destination + 3] = textureData[source];
            }
        }

        SDL_Texture* texture =
            SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC,
                static_cast<int>(width), static_cast<int>(height));
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

    void SwitchUi::drawPokemonSprite(size_t index, const SDL_Rect& destination)
    {
        if (!pokemonSpritesheet_ || index == 0 || index >= pokemonSprites_.size())
        {
            return;
        }
        const PokemonSprite& sprite = pokemonSprites_[index];
        if (sprite.rotated)
        {
            const SDL_Rect rotatedDestination{destination.x + (destination.w - destination.h) / 2,
                destination.y + (destination.h - destination.w) / 2, destination.h,
                destination.w};
            SDL_RenderCopyEx(renderer_, pokemonSpritesheet_, &sprite.source, &rotatedDestination,
                90.0, nullptr, SDL_FLIP_NONE);
        }
        else
        {
            SDL_RenderCopy(renderer_, pokemonSpritesheet_, &sprite.source, &destination);
        }
    }

    void SwitchUi::drawBallSprite(size_t index, const SDL_Rect& destination)
    {
        if (!ballSpritesheet_)
        {
            return;
        }
        const size_t spriteIndex = BALL_SPRITE_BASE + std::min(index, MAX_BALL_SPRITE);
        if (spriteIndex >= ballSprites_.size())
        {
            return;
        }
        const PokemonSprite& sprite = ballSprites_[spriteIndex];
        if (sprite.rotated)
        {
            const SDL_Rect rotatedDestination{destination.x + (destination.w - destination.h) / 2,
                destination.y + (destination.h - destination.w) / 2, destination.h,
                destination.w};
            SDL_RenderCopyEx(renderer_, ballSpritesheet_, &sprite.source, &rotatedDestination,
                90.0, nullptr, SDL_FLIP_NONE);
        }
        else
        {
            SDL_RenderCopy(renderer_, ballSpritesheet_, &sprite.source, &destination);
        }
    }

    Action SwitchUi::pollAction()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
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

    SDL_Texture* SwitchUi::textTexture(const std::string& text, int fontSize, SDL_Color color)
    {
        const std::string key = std::to_string(fontSize) + ":" +
                                std::to_string(static_cast<unsigned int>(color.r) << 24 |
                                               static_cast<unsigned int>(color.g) << 16 |
                                               static_cast<unsigned int>(color.b) << 8 |
                                               color.a) +
                                ":" + text;
        const auto found = textCache_.find(key);
        if (found != textCache_.end())
        {
            return found->second;
        }

        TTF_Font* font = fontSize == 36 ? headingFont_ : bodyFont_;
        SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
        if (!surface)
        {
            return nullptr;
        }
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
        SDL_FreeSurface(surface);
        if (!texture)
        {
            return nullptr;
        }
        if (textCache_.size() >= 512)
        {
            const auto oldest = textCache_.begin();
            SDL_DestroyTexture(oldest->second);
            textCache_.erase(oldest);
        }
        textCache_.emplace(key, texture);
        return texture;
    }

    void SwitchUi::drawText(
        const std::string& text, int x, int y, int fontSize, SDL_Color color, int maxWidth)
    {
        const std::string displayText =
            maxWidth > 0 ? fitText(text, fontSize, maxWidth) : text;
        SDL_Texture* texture = textTexture(displayText, fontSize, color);
        if (!texture)
        {
            return;
        }
        int width = 0;
        int height = 0;
        if (SDL_QueryTexture(texture, nullptr, nullptr, &width, &height) != 0)
        {
            return;
        }
        const SDL_Rect destination{x, y, width, height};
        SDL_RenderCopy(renderer_, texture, nullptr, &destination);
    }

    void SwitchUi::drawFieldValue(
        const Field& field, int x, int y, int fontSize, SDL_Color color, int maxWidth)
    {
        if (field.hasBallSprite)
        {
            const int size = fontSize + 8;
            drawBallSprite(field.ballSpriteIndex, {x, y - 1, size, size});
            const int textX = x + size + 10;
            drawText(field.value, textX, y, fontSize, color,
                maxWidth > 0 ? std::max(0, maxWidth - size - 10) : 0);
            return;
        }
        if (!field.checkbox)
        {
            drawText(field.value, x, y, fontSize, color, maxWidth);
            return;
        }

        const int size = fontSize;
        const int boxY = y + 3;
        const SDL_Rect box{x, boxY, size, size};
        drawRect(box, PANEL, field.checked ? ACCENT : PANEL_BORDER);
        if (field.checked)
        {
            SDL_SetRenderDrawColor(renderer_, ACCENT.r, ACCENT.g, ACCENT.b, ACCENT.a);
            SDL_RenderDrawLine(renderer_, x + size / 5, boxY + size / 2,
                x + size * 2 / 5, boxY + size * 4 / 5);
            SDL_RenderDrawLine(renderer_, x + size * 2 / 5, boxY + size * 4 / 5,
                x + size * 4 / 5, boxY + size / 5);
        }
        const int textX = x + size + 10;
        drawText(field.value, textX, y, fontSize, color,
            maxWidth > 0 ? std::max(0, maxWidth - size - 10) : 0);
    }

    void SwitchUi::drawHeader(const std::string& title, const std::string& subtitle)
    {
        drawText(title, 52, 32, 36, TEXT);
        drawText(subtitle, 54, 84, 24, MUTED, SCREEN_WIDTH - 108);
        SDL_Rect divider{52, 127, SCREEN_WIDTH - 104, 2};
        SDL_SetRenderDrawColor(renderer_, PANEL_BORDER.r, PANEL_BORDER.g, PANEL_BORDER.b, 255);
        SDL_RenderFillRect(renderer_, &divider);
    }

    void SwitchUi::drawFooter(const std::string& text)
    {
        drawText(text, 54, SCREEN_HEIGHT - 47, 24, MUTED, SCREEN_WIDTH - 108);
    }

    void SwitchUi::drawRect(const SDL_Rect& rect, SDL_Color fill, SDL_Color border)
    {
        SDL_SetRenderDrawColor(renderer_, fill.r, fill.g, fill.b, fill.a);
        SDL_RenderFillRect(renderer_, &rect);
        SDL_SetRenderDrawColor(renderer_, border.r, border.g, border.b, border.a);
        SDL_RenderDrawRect(renderer_, &rect);
    }

    std::string SwitchUi::fitText(const std::string& text, int fontSize, int maxWidth)
    {
        std::string result = text;
        TTF_Font* font = fontSize == 36 ? headingFont_ : bodyFont_;
        int width = 0;
        int height = 0;
        if (TTF_SizeUTF8(font, result.c_str(), &width, &height) != 0 || width <= maxWidth)
        {
            return result;
        }
        constexpr char ELLIPSIS[] = "...";
        while (!result.empty())
        {
            removeLastUtf8Character(result);
            const std::string candidate = result + ELLIPSIS;
            if (TTF_SizeUTF8(font, candidate.c_str(), &width, &height) == 0 && width <= maxWidth)
            {
                return candidate;
            }
        }
        return ELLIPSIS;
    }

    void SwitchUi::drawFiles(
        const std::vector<std::string>& files, const std::vector<std::string>& gameNames,
        size_t selected, const std::string& error)
    {
        SDL_SetRenderDrawColor(renderer_, BACKGROUND.r, BACKGROUND.g, BACKGROUND.b, 255);
        SDL_RenderClear(renderer_);
        drawHeader("PKSM / SWITCH", "Respaldos de Sword/Shield y Rojo Fuego/Verde Hoja");

        const SDL_Rect panel{52, 151, SCREEN_WIDTH - 104, 465};
        drawRect(panel, PANEL, PANEL_BORDER);
        if (!error.empty())
        {
            drawText(error, 78, 181, 24, ERROR, panel.w - 52);
        }
        else if (files.empty())
        {
            drawText("No se encontraron respaldos compatibles.", 78, 181, 24, TEXT);
            drawText("Copia tus archivos a /switch/pksm/ en la tarjeta SD.", 78, 220, 24, MUTED);
        }
        else
        {
            const size_t visibleRows = 11;
            const size_t first = selected >= visibleRows ? selected - visibleRows + 1 : 0;
            const size_t last = std::min(files.size(), first + visibleRows);
            for (size_t i = first; i < last; ++i)
            {
                const int y = 172 + static_cast<int>(i - first) * 38;
                if (i == selected)
                {
                    const SDL_Rect row{68, y - 4, panel.w - 32, 34};
                    drawRect(row, SELECTED, SELECTED);
                }
                const std::string label =
                    i < gameNames.size() ? gameNames[i] + "  /  " + files[i] : files[i];
                drawText(label, 82, y, 24, i == selected ? TEXT : MUTED, panel.w - 64);
            }
            const std::string count = std::to_string(selected + 1) + " / " +
                                      std::to_string(files.size());
            drawText(count, SCREEN_WIDTH - 140, 582, 24, MUTED);
        }
        drawText("Los archivos se buscan en subcarpetas. El guardado original no se modifica.",
            70, 636, 24, MUTED, SCREEN_WIDTH - 140);
        drawFooter("A: abrir    +: salir");
        SDL_RenderPresent(renderer_);
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

        SDL_SetRenderDrawColor(renderer_, BACKGROUND.r, BACKGROUND.g, BACKGROUND.b, 255);
        SDL_RenderClear(renderer_);
        drawHeader(game, path);

        const SDL_Rect detailsPanel{52, 151, SCREEN_WIDTH - 104, 70};
        drawRect(detailsPanel, PANEL, PANEL_BORDER);
        drawText(details, 76, 174, 24, TEXT, detailsPanel.w - 48);

        constexpr int columns = 2;
        constexpr int cardWidth = (SCREEN_WIDTH - 120) / columns;
        constexpr int cardHeight = 92;
        constexpr int horizontalGap = 16;
        constexpr int verticalGap = 14;
        constexpr int startY = 238;
        for (size_t i = 0; i < sizeof(options) / sizeof(options[0]); ++i)
        {
            const int column = static_cast<int>(i % columns);
            const int row = static_cast<int>(i / columns);
            const SDL_Rect card{52 + column * (cardWidth + horizontalGap),
                startY + row * (cardHeight + verticalGap), cardWidth, cardHeight};
            const SDL_Color fill = i == selected ? SELECTED : PANEL;
            const SDL_Color border =
                i == selected && options[i].available ? ACCENT : PANEL_BORDER;
            const SDL_Color labelColor = options[i].available ? TEXT : MUTED;
            const SDL_Color descriptionColor = options[i].available ? MUTED : PANEL_BORDER;
            drawRect(card, fill, border);
            drawText(options[i].label, card.x + 24, card.y + 15, 24, labelColor);
            drawText(options[i].description, card.x + 24, card.y + 51, 24, descriptionColor,
                card.w - 48);
            if (!options[i].available)
            {
                drawText("Próximamente", card.x + card.w - 184, card.y + 15, 24, PANEL_BORDER);
            }
        }

        if (!message.empty())
        {
            drawText(message, 70, 590, 24, ACCENT, SCREEN_WIDTH - 140);
        }
        drawFooter("A: abrir    B: volver    Y: exportar    X: descartar    +: salir");
        SDL_RenderPresent(renderer_);
    }

    void SwitchUi::drawPokemonSlots(const std::string& game, const std::string& path,
        const std::string& section, const std::vector<Field>& fields, size_t selected, bool dirty,
        const std::string& error)
    {
        SDL_SetRenderDrawColor(renderer_, BACKGROUND.r, BACKGROUND.g, BACKGROUND.b, 255);
        SDL_RenderClear(renderer_);
        drawHeader("Editor de Pokémon", game + "  /  " + path + "  /  " + section);

        const SDL_Rect panel{52, 151, 846, 444};
        const SDL_Rect summaryPanel{916, 151, 312, 444};
        drawRect(panel, PANEL, PANEL_BORDER);
        drawRect(summaryPanel, PANEL, PANEL_BORDER);
        const size_t columns = fields.size() <= 6 ? 3 : 6;
        constexpr int horizontalGap = 8;
        constexpr int tileHeight = 72;
        constexpr int verticalGap = 8;
        const int gridX = panel.x + 16;
        const int tileWidth =
            (panel.w - 32 - static_cast<int>(columns - 1) * horizontalGap) /
            static_cast<int>(columns);
        for (size_t i = 0; i < fields.size(); ++i)
        {
            const size_t rowIndex = i / columns;
            const size_t column = i % columns;
            const int x = gridX + static_cast<int>(column) * (tileWidth + horizontalGap);
            const int y = 163 + static_cast<int>(rowIndex) * (tileHeight + verticalGap);
            const SDL_Rect tile{x, y, tileWidth, tileHeight};
            if (i == selected)
            {
                drawRect(tile, SELECTED, SELECTED);
            }
            else if (rowIndex % 2 == 1)
            {
                SDL_SetRenderDrawColor(renderer_, ROW_ALT.r, ROW_ALT.g, ROW_ALT.b, ROW_ALT.a);
                SDL_RenderFillRect(renderer_, &tile);
            }
            if (i != selected)
            {
                SDL_SetRenderDrawColor(
                    renderer_, PANEL_BORDER.r, PANEL_BORDER.g, PANEL_BORDER.b, PANEL_BORDER.a);
                SDL_RenderDrawRect(renderer_, &tile);
            }
            drawText(std::to_string(i + 1), x + 8, y + 5, 24,
                i == selected ? TEXT : MUTED);
            drawPokemonSprite(fields[i].spriteIndex, {x + 5, y + 18, 36, 32});
            drawText(fields[i].value, x + 46, y + 23, 24, TEXT, tileWidth - 54);
            SDL_SetRenderDrawColor(
                renderer_, ROW_DIVIDER.r, ROW_DIVIDER.g, ROW_DIVIDER.b, ROW_DIVIDER.a);
            SDL_RenderDrawLine(renderer_, x + 4, y + tileHeight - 1, x + tileWidth - 4,
                y + tileHeight - 1);
        }
        drawText("Pokémon seleccionado", summaryPanel.x + 20, 176, 24, ACCENT,
            summaryPanel.w - 40);
        if (selected < fields.size())
        {
            drawText(fields[selected].label, summaryPanel.x + 20, 230, 24, MUTED,
                summaryPanel.w - 40);
            drawFieldValue(fields[selected], summaryPanel.x + 20, 270, 36, TEXT,
                summaryPanel.w - 40);
            if (fields[selected].hasBallSprite)
            {
                drawBallSprite(fields[selected].ballSpriteIndex,
                    {summaryPanel.x + 126, 385, 72, 72});
            }
            else
            {
                drawPokemonSprite(fields[selected].spriteIndex,
                    {summaryPanel.x + 76, 348, 160, 140});
            }
        }
        drawText("A: abrir editor", summaryPanel.x + 20, 522, 24, MUTED,
            summaryPanel.w - 40);
        if (dirty)
        {
            drawText("Cambios pendientes de exportar", 82, 562, 24, ACCENT);
        }
        if (!error.empty())
        {
            drawText(error, 70, 612, 24, ERROR, SCREEN_WIDTH - 140);
        }
        drawFooter(
            "D-pad: mover    L/R: cajas/equipo    A: editar    Y: exportar    B: volver");
        SDL_RenderPresent(renderer_);
    }

    void SwitchUi::drawPokemonEditor(const std::string& game, const std::string& path,
        const std::string& pokemon, size_t speciesIndex, const std::vector<Field>& fields,
        size_t selected, bool dirty, const std::string& error)
    {
        SDL_SetRenderDrawColor(renderer_, BACKGROUND.r, BACKGROUND.g, BACKGROUND.b, 255);
        SDL_RenderClear(renderer_);
        drawHeader("Editor de Pokémon", game + "  /  " + path + "  /  " + pokemon);

        const SDL_Rect panel{52, 151, 820, 444};
        const SDL_Rect summaryPanel{890, 151, 338, 444};
        drawRect(panel, PANEL, PANEL_BORDER);
        drawRect(summaryPanel, PANEL, PANEL_BORDER);
        constexpr size_t visibleRows = 7;
        const size_t first = selected >= visibleRows ? selected - visibleRows + 1 : 0;
        const size_t last = std::min(fields.size(), first + visibleRows);
        for (size_t i = first; i < last; ++i)
        {
            const int y = 164 + static_cast<int>(i - first) * 58;
            if (i == selected)
            {
                const SDL_Rect row{68, y - 5, panel.w - 32, 50};
                drawRect(row, SELECTED, SELECTED);
            }
            else if ((i - first) % 2 == 1)
            {
                const SDL_Rect row{68, y - 5, panel.w - 32, 50};
                SDL_SetRenderDrawColor(renderer_, ROW_ALT.r, ROW_ALT.g, ROW_ALT.b, ROW_ALT.a);
                SDL_RenderFillRect(renderer_, &row);
            }
            drawText(fields[i].label, 88, y, 24, i == selected ? TEXT : MUTED, 320);
            drawFieldValue(fields[i], 430, y, 24, TEXT, panel.w - 400);
            SDL_SetRenderDrawColor(
                renderer_, ROW_DIVIDER.r, ROW_DIVIDER.g, ROW_DIVIDER.b, ROW_DIVIDER.a);
            SDL_RenderDrawLine(renderer_, 82, y + 43, panel.x + panel.w - 16, y + 43);
        }

        const auto valueFor = [&fields](const char* label) -> std::string
        {
            const auto field = std::find_if(fields.begin(), fields.end(),
                [label](const Field& candidate) { return candidate.label == label; });
            return field == fields.end() ? std::string{} : field->value;
        };
        drawText("Resumen del Pokémon", 914, 174, 24, ACCENT, summaryPanel.w - 48);
        drawPokemonSprite(speciesIndex, {summaryPanel.x + 204, 206, 110, 100});
        drawText(valueFor("Especie"), 914, 224, 24, TEXT, summaryPanel.w - 48);
        drawText(valueFor("Apodo"), 914, 264, 24, MUTED, summaryPanel.w - 48);
        const char* summaryLabels[] = {"Nivel", "Naturaleza", "Género", "Variocolor"};
        for (size_t i = 0; i < sizeof(summaryLabels) / sizeof(summaryLabels[0]); ++i)
        {
            const int y = 328 + static_cast<int>(i) * 48;
            drawText(summaryLabels[i], 914, y, 24, MUTED, 154);
            const auto field = std::find_if(fields.begin(), fields.end(),
                [label = summaryLabels[i]](const Field& candidate)
                {
                    return candidate.label == label;
                });
            if (field != fields.end())
            {
                drawFieldValue(*field, 1070, y, 24, TEXT, 132);
            }
            SDL_SetRenderDrawColor(
                renderer_, ROW_DIVIDER.r, ROW_DIVIDER.g, ROW_DIVIDER.b, ROW_DIVIDER.a);
            SDL_RenderDrawLine(renderer_, 914, y + 35, 1204, y + 35);
        }
        if (dirty)
        {
            drawText("Cambios pendientes de exportar", 82, 606, 24, ACCENT);
        }
        if (!error.empty())
        {
            drawText(error, 70, 636, 24, ERROR, SCREEN_WIDTH - 140);
        }
        drawFooter("D-pad: mover/cambiar    A: editar    Y: exportar    B: volver    X: descartar");
        SDL_RenderPresent(renderer_);
    }

    void SwitchUi::drawTrainer(const std::string& game, const std::string& path,
        const std::vector<Field>& fields, size_t selected, bool dirty, const std::string& error)
    {
        SDL_SetRenderDrawColor(renderer_, BACKGROUND.r, BACKGROUND.g, BACKGROUND.b, 255);
        SDL_RenderClear(renderer_);
        drawHeader("Ficha del entrenador", game + "  /  " + path);

        const SDL_Rect panel{52, 151, SCREEN_WIDTH - 104, 444};
        drawRect(panel, PANEL, PANEL_BORDER);
        for (size_t i = 0; i < fields.size(); ++i)
        {
            const int y = 166 + static_cast<int>(i) * 48;
            if (i == selected)
            {
                const SDL_Rect row{68, y - 4, panel.w - 32, 43};
                drawRect(row, SELECTED, SELECTED);
            }
            else if (i % 2 == 1)
            {
                const SDL_Rect row{68, y - 4, panel.w - 32, 43};
                SDL_SetRenderDrawColor(renderer_, ROW_ALT.r, ROW_ALT.g, ROW_ALT.b, ROW_ALT.a);
                SDL_RenderFillRect(renderer_, &row);
            }
            drawText(fields[i].label, 88, y, 24, i == selected ? TEXT : MUTED);
            drawText(fields[i].value, 530, y, 24, TEXT, panel.w - 610);
        }
        if (dirty)
        {
            drawText("Cambios pendientes de exportar", 82, 562, 24, ACCENT);
        }
        if (!error.empty())
        {
            drawText(error, 70, 612, 24, ERROR, SCREEN_WIDTH - 140);
        }
        drawFooter(
            "D-pad: mover/cambiar    A: nombre    Y: exportar    B: menu    X: descartar");
        SDL_RenderPresent(renderer_);
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
            SDL_SetRenderDrawColor(
                renderer_, BACKGROUND.r, BACKGROUND.g, BACKGROUND.b, BACKGROUND.a);
            SDL_RenderClear(renderer_);
            drawHeader(title, "Selecciona una opción");
            const SDL_Rect panel{52, 151, SCREEN_WIDTH - 104, 444};
            drawRect(panel, PANEL, PANEL_BORDER);
            const size_t last = std::min(options.size(), first + visibleRows);
            for (size_t i = first; i < last; ++i)
            {
                const int y = 171 + static_cast<int>(i - first) * 44;
                if (i == index)
                {
                    const SDL_Rect row{68, y - 5, panel.w - 32, 40};
                    drawRect(row, SELECTED, SELECTED);
                }
                if (i < spriteIndices.size())
                {
                    drawBallSprite(spriteIndices[i], {88, y - 3, 32, 32});
                    drawText(options[i], 130, y, 24, i == index ? TEXT : MUTED,
                        panel.w - 114);
                }
                else
                {
                    drawText(options[i], 88, y, 24, i == index ? TEXT : MUTED, panel.w - 72);
                }
            }
            drawText(std::to_string(index + 1) + " / " + std::to_string(options.size()),
                SCREEN_WIDTH - 170, 556, 24, MUTED);
            drawFooter("Flechas: mover    L/R: avanzar 100 opciones    A: seleccionar    B: volver");
            SDL_RenderPresent(renderer_);
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

        constexpr int panelX = 52;
        constexpr int panelY = 151;
        constexpr int panelWidth = SCREEN_WIDTH - 104;
        constexpr int panelHeight = 444;
        constexpr int labelWidth = 180;
        constexpr int cellWidth = (panelWidth - 24 - labelWidth) / 5;
        constexpr int headerHeight = 58;
        constexpr int cellHeight = 60;
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

            SDL_SetRenderDrawColor(
                renderer_, BACKGROUND.r, BACKGROUND.g, BACKGROUND.b, BACKGROUND.a);
            SDL_RenderClear(renderer_);
            drawHeader("Naturaleza", "Selecciona una naturaleza según sus estadísticas");
            const SDL_Rect panel{panelX, panelY, panelWidth, panelHeight};
            drawRect(panel, PANEL, PANEL_BORDER);

            const int gridX = panelX + 12;
            const int gridY = panelY + 14;
            const SDL_Rect corner{gridX, gridY, labelWidth, headerHeight};
            drawRect(corner, TABLE_HEADER, PANEL_BORDER);
            drawText(columnLabels[0], corner.x + 8, corner.y + 17, 24, YELLOW,
                corner.w - 16);
            for (size_t x = 0; x < 5; ++x)
            {
                const SDL_Rect header{gridX + labelWidth + static_cast<int>(x) * cellWidth,
                    gridY, cellWidth, headerHeight};
                drawRect(header, TABLE_HEADER, PANEL_BORDER);
                drawText(columnLabels[x + 1], header.x + 4, header.y + 17, 24, TEXT,
                    header.w - 8);
            }
            for (size_t y = 0; y < 5; ++y)
            {
                const int cellY = gridY + headerHeight + static_cast<int>(y) * cellHeight;
                const SDL_Rect rowHeader{gridX, cellY, labelWidth, cellHeight};
                drawRect(rowHeader, TABLE_HEADER, PANEL_BORDER);
                drawText(rowLabels[y], rowHeader.x + 8, rowHeader.y + 18, 24, TEXT,
                    rowHeader.w - 16);
                for (size_t x = 0; x < 5; ++x)
                {
                    const SDL_Rect cell{
                        gridX + labelWidth + static_cast<int>(x) * cellWidth, cellY, cellWidth,
                        cellHeight};
                    const size_t natureIndex = natureGrid[y][x];
                    const bool isSelected = row == y && column == x;
                    drawRect(cell, isSelected ? SELECTED : PANEL,
                        isSelected ? ACCENT : PANEL_BORDER);
                    const SDL_Color color = natureIndex % 6 == 0 ? YELLOW : TEXT;
                    drawText(natures[natureIndex], cell.x + 5, cell.y + 18, 24, color,
                        cell.w - 10);
                }
            }
            drawFooter("Flechas: mover    A: seleccionar    B: volver");
            SDL_RenderPresent(renderer_);
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
