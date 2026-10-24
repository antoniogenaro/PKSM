#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <switch.h>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace pksm::switchui
{
    enum class Action
    {
        None,
        Up,
        Down,
        Left,
        Right,
        PreviousPage,
        NextPage,
        Confirm,
        Back,
        Discard,
        Save,
        Quit
    };

    struct Field
    {
        std::string label;
        std::string value;
        size_t spriteIndex = 0;
    };

    class SwitchUi
    {
    public:
        SwitchUi() = default;
        SwitchUi(const SwitchUi&) = delete;
        SwitchUi& operator=(const SwitchUi&) = delete;
        ~SwitchUi();

        bool initialize(std::string& error);
        Action pollAction();
        void drawFiles(
            const std::vector<std::string>& files, size_t selected, const std::string& error);
        void drawGameMenu(const std::string& game, const std::string& path,
            const std::string& details, size_t selected, const std::string& message);
        void drawPokemonSlots(const std::string& game, const std::string& path,
            const std::string& section, const std::vector<Field>& fields, size_t selected,
            bool dirty, const std::string& error);
        void drawPokemonEditor(const std::string& game, const std::string& path,
            const std::string& pokemon, size_t speciesIndex, const std::vector<Field>& fields,
            size_t selected, bool dirty, const std::string& error);
        void drawTrainer(const std::string& game, const std::string& path,
            const std::vector<Field>& fields, size_t selected, bool dirty, const std::string& error);
        bool editText(const std::string& title, const std::string& initial, std::string& result,
            size_t maxLength);
        bool editTrainerName(
            const std::string& initial, std::string& result, size_t maxLength = 12);

    private:
        SDL_Texture* textTexture(const std::string& text, int fontSize, SDL_Color color);
        void drawText(
            const std::string& text, int x, int y, int fontSize, SDL_Color color, int maxWidth = 0);
        void drawHeader(const std::string& title, const std::string& subtitle);
        void drawFooter(const std::string& text);
        void drawRect(const SDL_Rect& rect, SDL_Color fill, SDL_Color border);
        bool loadPokemonSprites(std::string& error);
        void drawPokemonSprite(size_t index, const SDL_Rect& destination);
        std::string fitText(const std::string& text, int fontSize, int maxWidth);

        struct PokemonSprite
        {
            SDL_Rect source;
            int width;
            int height;
            bool rotated;
        };

        SDL_Window* window_ = nullptr;
        SDL_Renderer* renderer_ = nullptr;
        PadState pad_{};
        TTF_Font* headingFont_ = nullptr;
        TTF_Font* bodyFont_ = nullptr;
        SDL_Texture* pokemonSpritesheet_ = nullptr;
        std::vector<PokemonSprite> pokemonSprites_;
        std::unordered_map<std::string, SDL_Texture*> textCache_;
    };
}
