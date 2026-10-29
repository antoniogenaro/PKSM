#pragma once

#include <SDL.h>
#include <switch.h>
#include "imgui.h"
#include "platform/SystemStatus.hpp"
#include "ui/Icons.hpp"
#include <cstddef>
#include <initializer_list>
#include <string>
#include <utility>
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
        PreviousPage, // L
        NextPage,     // R
        PreviousTab,  // ZL
        NextTab,      // ZR
        Confirm,
        Back,
        Discard,
        Save,
        // The − button: a second way to edit the selection, such as typing an item's quantity.
        Alternate,
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
        // False when the save cannot change it; editors then show no pencil.
        bool editable = true;
        // 1 for male, 2 for female: the gender mark shown beside the value.
        int genderMark = 0;
    };

    // The editor's stats table and moves panel.
    struct PokemonStats
    {
        struct Row
        {
            std::string name; // "Atq"
            std::string iv;
            std::string ev;
            std::string total;
            // +1 when the nature raises the stat, -1 when it lowers it.
            int nature = 0;
        };
        struct MoveRow
        {
            std::string name;
            std::string pp; // "PP 15/15", or empty without a move.
        };

        std::vector<Row> rows;
        unsigned int evTotal = 0;
        unsigned int evMax   = 0;
        std::vector<MoveRow> moves;
    };

    // A game as the game and save lists show it.
    struct GameCard
    {
        std::string name;
        size_t mascotSpecies = 0;
        size_t saveCount = 0;
        unsigned artColor = 0; // 0xRRGGBB, from the game's profile.
        // Whether the selected user has a save of the game on the console.
        bool consoleSave = false;
    };

    // The open save, as the side panel of the game menu and the editors shows it.
    struct SaveSummary
    {
        GameCard game;
        std::string group;
        // The file name, the trainer with their IDs, and the full path.
        std::string file;
        std::string details;
        std::string path;
    };

    // One box or party slot: the species sprite, or nothing when it is empty.
    struct PokemonSlot
    {
        bool empty     = true;
        size_t species = 0;
        // The types sheet's sprite for the form, or 0 for the species' own sprite.
        size_t formSprite = 0;
    };

    // What the box screen shows of the selected Pokémon.
    struct PokemonDetail
    {
        struct Info
        {
            std::string label;
            std::string value;
        };
        struct Iv
        {
            std::string label;
            std::string value;
            // +1 when the nature raises the stat, -1 when it lowers it.
            int nature = 0;
        };

        bool empty     = true;
        size_t species = 0;
        // The types sheet's sprite for the form, or 0 for the species' own sprite.
        size_t formSprite = 0;
        std::string number;   // "N.º 025"
        std::string name;     // The species.
        std::string level;    // "Nv. 50"
        std::string nickname; // "Apodo: …", or empty without one.
        bool male      = false;
        bool female    = false;
        bool shiny     = false;
        size_t ball    = 0;
        // Type names with their index in pksm::Type, for the colour.
        std::vector<std::pair<std::string, size_t>> types;
        std::vector<Info> info; // Trainer, ID, item, ability, nature, origin game.
        std::vector<Iv> ivs;    // HP, Attack, Defense, Sp. Atk, Sp. Def, Speed.
        std::vector<std::string> moves;
    };

    // Everything the Pokémon editor draws.
    struct EditorView
    {
        // Where the selection is: a field card, an IV or EV cell, or a move.
        enum class Area
        {
            Fields,
            Iv,
            Ev,
            Moves
        };
        static constexpr size_t COLUMNS   = 3;
        static constexpr size_t STAT_ROWS = 6;

        PokemonDetail header;
        // The fields of the shown page.
        std::vector<Field> fields;
        size_t page      = 0;
        size_t pageCount = 2;
        PokemonStats stats;
        Area area       = Area::Fields;
        size_t selected = 0;
        std::string message;
    };

    // Everything the box screen draws.
    struct BoxView
    {
        static constexpr size_t COLUMNS = 6;

        std::string boxName;
        size_t boxFilled = 0;
        std::vector<PokemonSlot> box;
        // Always six; `partyCount` of them are filled.
        std::vector<PokemonSlot> party;
        size_t partyCount = 0;
        bool inParty      = false;
        size_t selected   = 0;
        PokemonDetail detail;
        std::string message;
        // A Pokémon picked up to move: it is drawn lifted over the selected slot, and the slot it
        // came from (`carriedInParty`, `carriedSlot`) is dimmed when `carriedShown`.
        bool carrying       = false;
        bool carriedShown   = false;
        bool carriedInParty = false;
        size_t carriedSlot  = 0;
        PokemonSlot carried;
        // Set by drawBoxes() when an arrow is touched: -1 for the box before, 1 for the next.
        int pageRequest = 0;
    };

    // What the date picker draws: a column for each part of the date, in the order shown, with
    // the values one step up and one step down beside the current one.
    struct DatePickerView
    {
        struct Column
        {
            std::string header; // "Día"
            std::string value;
            std::string up;   // The value Up would pick.
            std::string down; // The value Down would pick.
        };

        std::string title;
        // The date before picking, shown in the title row.
        std::string current;
        std::vector<Column> columns;
        size_t column = 0;
        // Set by drawDatePicker() when a value beside the current one is touched: 1 for the one
        // above, -1 for the one below.
        int stepRequest = 0;
    };

    // Everything the bag screen draws: the pouches on the left and the shown pouch's items as a
    // grid of COLUMNS cards, ROWS rows at a time from `firstRow`.
    struct BagView
    {
        static constexpr size_t COLUMNS = 2;
        static constexpr size_t ROWS    = 8;
        static constexpr size_t NONE    = static_cast<size_t>(-1);

        struct Pouch
        {
            std::string name;
            std::string count; // How many items it holds.
            icons::Icon icon = icons::Icon::Bag;
        };
        struct Item
        {
            // Empty for the slot after the last item, which adds one.
            std::string name;
            // The quantity, or empty for items that do not stack.
            std::string count;
        };

        std::string caption; // "BOLSILLOS · ESPADA"
        std::vector<Pouch> pouches;
        size_t pouch = 0;
        std::string itemCount; // "16 objetos"
        std::vector<Item> items;
        size_t firstRow = 0;
        size_t selected = 0;
        // The item picked up with Y, or NONE.
        size_t carried = NONE;
        std::string message;
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
        // Every frame's screens are drawn between these two calls: a screen and the overlays
        // above it, which then show on top of it.
        void beginScreens();
        void endScreens();
        // The console user the header shows.
        void setUserName(std::string name) { status_.setUser(std::move(name)); }
        // Every supported game, laid out by `grid`; `games` follows the grid's numbering.
        // A touch on a game moves the selection, so it is passed by reference. With
        // `canSwitchUser` the legend offers X to choose another console user.
        void drawGameList(const GameGrid& grid, const std::vector<GameCard>& games,
            size_t& selected, const std::string& error, bool canSwitchUser);
        // The saves of one game: first `consoleUser`'s console save when the game card says
        // there is one, then `files`, as paths relative to the save folder. `selected` counts
        // the console save as the first row.
        void drawSaveList(const GameCard& game, const std::string& group,
            const std::string& consoleUser, const std::vector<std::string>& files,
            size_t& selected, const std::string& error);
        // The console users as cards with their names; `current` is the one in use.
        void drawUserPicker(const std::vector<std::string>& users, size_t current, size_t& selected);
        // The actions for an open save. `message` is shown under the game card.
        void drawGameMenu(const SaveSummary& save, size_t selected, const std::string& message);
        // The boxes and party with the selected Pokémon's detail. Touching a slot selects it.
        void drawBoxes(BoxView& view);
        // The Pokémon editor: its header, a page of field cards, the stats table and the moves.
        // Touching a card or cell selects it.
        void drawPokemonEditor(EditorView& view);
        // The trainer's fields as a grid of TRAINER_COLUMNS cards, filled row by row. `message`
        // is shown under the game card.
        static constexpr size_t TRAINER_COLUMNS = 2;
        void drawTrainer(const SaveSummary& save, const std::vector<Field>& fields,
            size_t selected, const std::string& message);
        // The bag: the pouches and the items of the selected one. Touching a pouch or an item
        // selects it.
        void drawBag(BagView& view);
        // Pickers, as dialogs over the screen below. A touch on an entry moves the selection, so
        // it is passed by reference. The option picker shows PICKER_ROWS rows of PICKER_COLUMNS
        // from `firstRow`, with `current` (the value before picking) in its title.
        static constexpr size_t PICKER_COLUMNS = 4;
        static constexpr size_t PICKER_ROWS    = 10;
        void drawOptionPicker(const std::string& title, const std::string& current,
            const std::vector<std::string>& options, const std::vector<size_t>& spriteIndices,
            size_t firstRow, size_t& index);
        // `natures` holds the 25 natures in their usual order: row * 5 + column. `current` is
        // the Pokémon's nature, shown in the header.
        void drawNaturePicker(
            const std::vector<std::string>& natures, size_t current, size_t& row, size_t& column);
        // A date a part at a time. Touching a column selects it.
        void drawDatePicker(DatePickerView& view);

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
        // A dialog over the dimmed screen below, `width` wide and `height` tall plus a row for
        // its button legend, with the cursor at its top left inside the padding. Draw the legend
        // with widgets::drawButtonLegend before endModal(), which must always follow.
        void beginModal(float width, float height);
        void endModal();
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
        // drawGamePanel() for an open save: its file, trainer and path under the name.
        void drawSavePanel(const SaveSummary& save, const std::string& message);
        // One box or party slot, `size` wide, at `min`.
        void drawSlotTile(const PokemonSlot& slot, const ImVec2& min, float size, bool selected);
        // The detail panel of the box screen, filling the rest of the current panel.
        void drawPokemonDetail(const PokemonDetail& detail);

        bool loadSpriteSheet(const char* path, const std::string& name, SDL_Texture*& texture,
            std::vector<PokemonSprite>& sprites, std::string& error);
        void drawSprite(SDL_Texture* texture, const std::vector<PokemonSprite>& sprites,
            size_t index, size_t baseIndex, const SDL_Rect& destination);
        // The species' sprite, or `formSprite` from the types sheet when it is not 0.
        void drawPokemonSprite(
            size_t index, const SDL_Rect& destination, size_t formSprite = 0);
        void drawBallSprite(size_t index, const SDL_Rect& destination);

        bool imguiInitialized_ = false;
        SDL_Window* window_ = nullptr;
        SDL_Renderer* renderer_ = nullptr;
        SDL_Texture* pokemonSpritesheet_ = nullptr;
        SDL_Texture* ballSpritesheet_ = nullptr;
        std::vector<PokemonSprite> pokemonSprites_;
        std::vector<PokemonSprite> ballSprites_;
        PadState pad_{};
        SystemStatus status_;
    };
}
