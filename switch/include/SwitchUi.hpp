#pragma once

#include <SDL.h>
#include <switch.h>
#include <cstddef>
#include <string>
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
        bool checkbox = false;
        bool checked = false;
        bool hasBallSprite = false;
        size_t ballSpriteIndex = 0;
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
            const std::vector<std::string>& files, const std::vector<std::string>& gameNames,
            const std::vector<size_t>& gameMascots, size_t selected, const std::string& error);
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
        bool selectOption(const std::string& title, const std::vector<std::string>& options,
            size_t current, size_t& selected,
            const std::vector<size_t>& spriteIndices = {});
        bool selectNature(const std::vector<std::string>& natures, size_t current,
            size_t& selected);
        bool editTrainerName(
            const std::string& initial, std::string& result, size_t maxLength = 12);

    private:
        struct PokemonSprite
        {
            SDL_Rect source;
            int width;
            int height;
            bool rotated;
            int sheetWidth;
            int sheetHeight;
        };

        // Every screen draws inside the same full-screen window, between these two calls.
        void beginFrame();
        void endFrame();
        void drawStatusLine(bool dirty, const std::string& error);

        bool loadSpriteSheet(const char* path, const std::string& name, SDL_Texture*& texture,
            std::vector<PokemonSprite>& sprites, std::string& error);
        void drawSprite(SDL_Texture* texture, const std::vector<PokemonSprite>& sprites,
            size_t index, size_t baseIndex, const SDL_Rect& destination);
        void drawPokemonSprite(size_t index, const SDL_Rect& destination);
        void drawBallSprite(size_t index, const SDL_Rect& destination);

        bool imguiInitialized_ = false;
        SDL_Window* window_ = nullptr;
        SDL_Renderer* renderer_ = nullptr;
        SDL_Texture* pokemonSpritesheet_ = nullptr;
        SDL_Texture* ballSpritesheet_ = nullptr;
        std::vector<PokemonSprite> pokemonSprites_;
        std::vector<PokemonSprite> ballSprites_;
        PadState pad_{};
    };
}
