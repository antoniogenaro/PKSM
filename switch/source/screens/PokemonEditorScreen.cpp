#include "screens/PokemonEditorScreen.hpp"

#include "SwitchUi.hpp"
#include "app/App.hpp"
#include "app/Session.hpp"
#include "pkx/PKX.hpp"
#include "sav/Sav.hpp"
#include "screens/PokemonFields.hpp"
#include "utils/i18n.hpp"

namespace pksm::switchui
{
    PokemonEditorScreen::PokemonEditorScreen(Session& session, std::string game, std::string path,
        std::unique_ptr<pksm::PKX> pokemon, bool party, u8 box, u8 slot)
        : session_(session),
          game_(std::move(game)),
          path_(std::move(path)),
          pokemon_(std::move(pokemon)),
          party_(party),
          box_(box),
          slot_(slot)
    {
        i18n::init(pokemon_->language());
    }

    PokemonEditorScreen::~PokemonEditorScreen() = default;

    void PokemonEditorScreen::update(App& app, Action action)
    {
        std::vector<EditorField> fields = makeFields(*pokemon_);
        switch (action)
        {
            case Action::Quit:
                if (!session_.dirty())
                {
                    app.quit();
                    return;
                }
                message_ = "Exporta con Y o descarta con X antes de salir.";
                break;
            case Action::Up:
                selected_ = selected_ == 0 ? fields.size() - 1 : selected_ - 1;
                message_.clear();
                break;
            case Action::Down:
                selected_ = (selected_ + 1) % fields.size();
                message_.clear();
                break;
            case Action::Back:
                app.pop();
                break;
            case Action::Discard:
                app.closeSession();
                break;
            case Action::Save:
                if (session_.dirty())
                {
                    session_.exportWithMessage(message_);
                }
                break;
            case Action::Left:
            case Action::Right:
            case Action::Confirm:
            {
                const int direction = action == Action::Left ? -1 : action == Action::Right ? 1 : 0;
                if (changeField(fields[selected_], *pokemon_, session_.save(), app.ui(), direction,
                        message_))
                {
                    savePokemon(session_.save(), *pokemon_, party_, box_, slot_);
                    session_.markDirty();
                    message_.clear();
                }
                break;
            }
            default:
                break;
        }
    }

    void PokemonEditorScreen::draw(SwitchUi& ui)
    {
        const std::vector<EditorField> fields = makeFields(*pokemon_);
        std::vector<Field> displayFields;
        displayFields.reserve(fields.size());
        for (const auto& field : fields)
        {
            displayFields.push_back(field.display);
        }
        const std::string title =
            pokemonSummary(*pokemon_) + "  /  " +
            std::string(party_ ? "Equipo " : "Caja " + numericValue(box_ + 1)) + "  /  " +
            numericValue(slot_ + 1);
        ui.drawPokemonEditor(game_, path_, title, static_cast<size_t>(pokemon_->species()),
            displayFields, selected_, session_.dirty(), message_);
    }
}
