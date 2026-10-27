#pragma once

#include "SwitchUi.hpp"
#include "app/Screen.hpp"
#include "utils/coretypes.h"
#include <cstddef>
#include <memory>
#include <string>

namespace pksm
{
    class PKX;
}

namespace pksm::switchui
{
    class Session;
    struct EditableField;
    struct PokemonEditorFields;

    // Every editable field of one Pokémon: two pages of field cards, the IVs and EVs, and the
    // moves. Changes stay in the editor until + writes them to the slot; X goes back to what
    // the slot holds.
    class PokemonEditorScreen : public Screen
    {
    public:
        PokemonEditorScreen(Session& session, std::string game, std::string path,
            std::unique_ptr<pksm::PKX> pokemon, bool party, u8 box, u8 slot);
        ~PokemonEditorScreen() override;

        void update(App& app, Action action) override;
        void draw(SwitchUi& ui) override;

    private:
        using Area = EditorView::Area;

        void move(Action action, size_t pageFields);
        // The field the selection is on, or nullptr outside the fields.
        const EditableField* selectedField(const PokemonEditorFields& fields) const;
        // Writes the edited Pokémon to its slot.
        void saveChanges();
        // Reads the Pokémon from its slot again, dropping the changes.
        void resetChanges();

        Session& session_;
        std::string game_;
        std::string path_;
        std::unique_ptr<pksm::PKX> pokemon_;
        bool party_;
        u8 box_;
        u8 slot_;
        size_t page_     = 0;
        Area area_       = Area::Fields;
        size_t selected_ = 0;
        // Changes not yet written to the slot, and whether B was pressed once with them.
        bool unsaved_     = false;
        bool warnedBack_  = false;
        std::string message_;
    };
}
