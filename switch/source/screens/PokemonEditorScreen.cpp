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
        const std::vector<EditableField> fields = pokemonFields(*pokemon_, session_.save());
        const auto onChanged = [this]
        {
            savePokemon(session_.save(), *pokemon_, party_, box_, slot_);
            session_.markDirty();
            message_.clear();
        };
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
                if (stepField(fields[selected_], action == Action::Right ? 1 : -1))
                {
                    onChanged();
                }
                break;
            case Action::Confirm:
                // A picker calls onChanged later, while this screen is still below it.
                openField(fields[selected_], app, message_, onChanged);
                break;
            default:
                break;
        }
    }

    void PokemonEditorScreen::draw(SwitchUi& ui)
    {
        const std::vector<Field> fields = displayFields(pokemonFields(*pokemon_, session_.save()));
        const std::string title =
            pokemonSummary(*pokemon_) + "  /  " +
            std::string(party_ ? "Equipo " : "Caja " + numericValue(box_ + 1)) + "  /  " +
            numericValue(slot_ + 1);
        ui.drawPokemonEditor(game_, path_, title, static_cast<size_t>(pokemon_->species()),
            fields, selected_, session_.dirty(), message_);
    }
}
