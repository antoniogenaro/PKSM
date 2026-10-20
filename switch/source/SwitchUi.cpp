#include "SwitchUi.hpp"

#include <switch.h>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <string>

namespace pksm::switchui
{
    namespace
    {
        constexpr int SCREEN_WIDTH = 1280;
        constexpr int SCREEN_HEIGHT = 720;
        constexpr SDL_Color BACKGROUND{20, 27, 43, 255};
        constexpr SDL_Color PANEL{31, 42, 63, 255};
        constexpr SDL_Color PANEL_BORDER{61, 82, 112, 255};
        constexpr SDL_Color TEXT{236, 241, 248, 255};
        constexpr SDL_Color MUTED{166, 181, 202, 255};
        constexpr SDL_Color ACCENT{91, 190, 170, 255};
        constexpr SDL_Color SELECTED{50, 91, 111, 255};
        constexpr SDL_Color ERROR{255, 163, 145, 255};
        constexpr char FONT_PATH[] = "romfs:/fonts/Roboto.ttf";

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
        if (joystick_)
        {
            SDL_JoystickClose(joystick_);
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
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0)
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
        if (SDL_NumJoysticks() > 0)
        {
            joystick_ = SDL_JoystickOpen(0);
            if (!joystick_)
            {
                error = std::string("No se pudo abrir el mando: ") + SDL_GetError();
                return false;
            }
        }
        return true;
    }

    Action SwitchUi::pollAction()
    {
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

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                return Action::Quit;
            }
            if (event.type == SDL_JOYHATMOTION && event.jhat.which == 0)
            {
                if (event.jhat.value & SDL_HAT_UP)
                {
                    return Action::Up;
                }
                if (event.jhat.value & SDL_HAT_DOWN)
                {
                    return Action::Down;
                }
                if (event.jhat.value & SDL_HAT_LEFT)
                {
                    return Action::Left;
                }
                if (event.jhat.value & SDL_HAT_RIGHT)
                {
                    return Action::Right;
                }
            }
            if (event.type == SDL_JOYAXISMOTION && event.jaxis.which == 0 &&
                std::abs(event.jaxis.value) > 16000)
            {
                if (event.jaxis.axis == 0)
                {
                    return event.jaxis.value < 0 ? Action::Left : Action::Right;
                }
                if (event.jaxis.axis == 1)
                {
                    return event.jaxis.value < 0 ? Action::Up : Action::Down;
                }
            }
            if (event.type == SDL_JOYBUTTONDOWN && event.jbutton.which == 0)
            {
                switch (event.jbutton.button)
                {
                    case 0:
                        return Action::Confirm;
                    case 1:
                        return Action::Back;
                    case 2:
                        return Action::Discard;
                    case 3:
                        return Action::Save;
                    case 10:
                        return Action::Quit;
                    default:
                        break;
                }
            }
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
        const std::vector<std::string>& files, size_t selected, const std::string& error)
    {
        SDL_SetRenderDrawColor(renderer_, BACKGROUND.r, BACKGROUND.g, BACKGROUND.b, 255);
        SDL_RenderClear(renderer_);
        drawHeader("PKSM / SWITCH", "Respaldos de Pokemon Sword y Shield");

        const SDL_Rect panel{52, 151, SCREEN_WIDTH - 104, 465};
        drawRect(panel, PANEL, PANEL_BORDER);
        if (!error.empty())
        {
            drawText(error, 78, 181, 24, ERROR, panel.w - 52);
        }
        else if (files.empty())
        {
            drawText("No se encontraron respaldos.", 78, 181, 24, TEXT);
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
                drawText(files[i], 82, y, 24, i == selected ? TEXT : MUTED, panel.w - 64);
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
        drawFooter("Arriba/abajo: campo    Izq/der: cambiar    A: editar nombre    Y: exportar copia");
        SDL_RenderPresent(renderer_);
    }

    bool SwitchUi::editTrainerName(const std::string& initial, std::string& result)
    {
        SwkbdConfig config;
        if (R_FAILED(swkbdCreate(&config, 0)))
        {
            return false;
        }
        swkbdConfigMakePresetDefault(&config);
        swkbdConfigSetHeaderText(&config, "Nombre del entrenador");
        swkbdConfigSetInitialText(&config, initial.c_str());
        swkbdConfigSetStringLenMax(&config, 12);
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
}
