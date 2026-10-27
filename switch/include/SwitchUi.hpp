#pragma once

#include <SDL.h>
#include <switch.h>
#include "imgui.h"
#include <cstddef>
#include <initializer_list>
#include <string>
#include <vector>

namespace pksm::switchui
{
    class GameGrid;

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

    // A game as the game and save lists show it.
    struct GameCard
    {
        std::string name;
        size_t mascotSpecies = 0;
        size_t saveCount = 0;
        unsigned artColor = 0; // 0xRRGGBB, from the game's profile.
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
        // Every supported game, laid out by `grid`; `games` follows the grid's numbering.
        // A touch on a game moves the selection, so it is passed by reference.
        void drawGameList(const GameGrid& grid, const std::vector<GameCard>& games,
            size_t& selected, const std::string& error);
        // The saves of one game, as paths relative to the save folder.
        void drawSaveList(const GameCard& game, const std::string& group,
            const std::vector<std::string>& files, size_t& selected, const std::string& error);
        // The actions for an open save: `file` is its name, `details` the trainer and IDs and
        // `path` its full path. `message` is shown under the game card.
        void drawGameMenu(const GameCard& game, const std::string& group, const std::string& file,
            const std::string& details, const std::string& path, size_t selected,
            const std::string& message);
        void drawPokemonSlots(const std::string& game, const std::string& path,
            const std::string& section, const std::vector<Field>& fields, size_t selected,
            bool dirty, const std::string& error);
        void drawPokemonEditor(const std::string& game, const std::string& path,
            const std::string& pokemon, size_t speciesIndex, const std::vector<Field>& fields,
            size_t selected, bool dirty, const std::string& error);
        void drawTrainer(const std::string& game, const std::string& path,
            const std::vector<Field>& fields, size_t selected, bool dirty, const std::string& error);
        // Pickers. A touch on an entry moves the selection, so it is passed by reference.
        void drawOptionPicker(const std::string& title, const std::vector<std::string>& options,
            const std::vector<size_t>& spriteIndices, size_t first, size_t visibleRows,
            size_t& index);
        // `natures` holds the 25 natures in their usual order: row * 5 + column.
        void drawNaturePicker(const std::vector<std::string>& natures, size_t& row, size_t& column);

        // Shows the system keyboard and waits for it. Returns false when cancelled or empty.
        bool editText(const std::string& title, const std::string& initial, std::string& result,
            size_t maxLength);

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
        // A game's mascot over its name and how many saves it has. Returns true when touched.
        bool drawGameTile(const GameCard& game, float width, float height, bool selected);
        // The square art of a game, `size` wide, at `min`: its colour and its mascot.
        void drawGameArt(const GameCard& game, const ImVec2& min, float size);

        // One line under the game's name on the side panel. A non-zero `dotColor` (0xRRGGBB)
        // puts a status dot before it.
        struct CardLine
        {
            std::string text;
            float size;
            unsigned color;
            bool bold;
            unsigned dotColor;
        };
        // The panel on the left of the save list and the game menu: `group` as its caption, the
        // game's card with `lines` under its name, and `message` under the card.
        void drawGamePanel(const GameCard& game, const std::string& group,
            std::initializer_list<CardLine> lines, const std::string& message = {},
            unsigned messageColor = 0);

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
