#include "screens/EventsScreen.hpp"

#include "SwitchUi.hpp"
#include "app/App.hpp"
#include "app/Session.hpp"
#include "domain/EventLibrary.hpp"
#include "domain/FieldValues.hpp"
#include "domain/FormSprites.hpp"
#include "pkx/PK8.hpp"
#include "pkx/PKX.hpp"
#include "sav/Sav.hpp"
#include "screens/PokemonFields.hpp"
#include "screens/SaveSummary.hpp"
#include "ui/Text.hpp"
#include "utils/i18n.hpp"
#include "utils/random.hpp"
#include <algorithm>
#include <array>
#include <sstream>

namespace pksm::switchui
{
    namespace
    {
        constexpr size_t BOX_SLOTS = 30;

        // Every language a Sword and Shield Pokémon can have, in the order of
        // Text::EventLanguageCodes.
        constexpr std::array<pksm::Language, 9> LANGUAGES{pksm::Language::JPN,
            pksm::Language::ENG, pksm::Language::FRE, pksm::Language::ITA, pksm::Language::GER,
            pksm::Language::SPA, pksm::Language::KOR, pksm::Language::CHS, pksm::Language::CHT};

        // How a card says the Pokémon's PID is made, as the game reads it.
        enum class ShinyType : u8
        {
            Never        = 0,
            Random       = 1,
            AlwaysStar   = 2,
            AlwaysSquare = 3,
            Fixed        = 4
        };

        // Captions are written in capitals; game names are plain ASCII.
        std::string upper(std::string text)
        {
            for (char& c : text)
            {
                if (c >= 'a' && c <= 'z')
                {
                    c = static_cast<char>(c - 'a' + 'A');
                }
            }
            return text;
        }

        std::string gameName(EventLibrary::Game game)
        {
            switch (game)
            {
                case EventLibrary::Game::Sword:
                    return tr(Text::GameSword);
                case EventLibrary::Game::Shield:
                    return tr(Text::GameShield);
                default:
                    return std::string(tr(Text::GameSword)) + " / " + tr(Text::GameShield);
            }
        }

        std::string kindName(const pksm::WC8& card)
        {
            if (card.pokemon())
            {
                return tr(Text::EventKindPokemon);
            }
            if (card.BP())
            {
                return tr(Text::EventKindBattlePoints);
            }
            if (card.clothing())
            {
                return tr(Text::EventKindClothing);
            }
            return tr(Text::EventKindItem);
        }

        std::string cardNumber(const pksm::WC8& card)
        {
            return "#" + padDigits(card.ID(), 4);
        }

        EventsView::Card describeCard(const EventLibrary& library, size_t index)
        {
            const auto card = library.card(index);
            EventsView::Card result;
            result.name    = library.entry(index).name;
            result.number  = cardNumber(*card);
            result.kind    = kindName(*card);
            result.pokemon = card->pokemon();
            if (result.pokemon)
            {
                result.species = static_cast<size_t>(card->species());
                // The form sprite is picked from a Pokémon, so one is made with the card's
                // species, form and gender.
                auto pokemon = pksm::PKX::getPKM<pksm::Generation::EIGHT>(
                    nullptr, pksm::PK8::BOX_LENGTH);
                pokemon->species(card->species());
                pokemon->alternativeForm(card->alternativeForm());
                if (card->gender() < pksm::Gender::Genderless)
                {
                    pokemon->gender(card->gender());
                }
                result.formSprite = formSpriteIndex(*pokemon);
            }
            return result;
        }

        // The short names of LANGUAGES, in the app's language.
        std::vector<std::string> languageCodes()
        {
            std::vector<std::string> codes;
            std::stringstream list(tr(Text::EventLanguageCodes));
            std::string code;
            while (std::getline(list, code, ','))
            {
                codes.push_back(code);
            }
            codes.resize(LANGUAGES.size());
            return codes;
        }
    }

    EventsScreen::EventsScreen(Session& session, std::unique_ptr<EventLibrary> library)
        : session_(session), library_(std::move(library))
    {
        static_assert(PAGE_SIZE == EventsView::COLUMNS * EventsView::ROWS);
        cards_.reserve(library_->size());
        for (size_t i = 0; i < library_->size(); ++i)
        {
            cards_.push_back(describeCard(*library_, i));
        }
        const pksm::Language own = session_.save().language();
        if (std::find(LANGUAGES.begin(), LANGUAGES.end(), own) != LANGUAGES.end())
        {
            languages_.push_back(own);
        }
        for (const pksm::Language language : LANGUAGES)
        {
            if (language != own)
            {
                languages_.push_back(language);
            }
        }
    }

    EventsScreen::~EventsScreen() = default;

    size_t EventsScreen::pageCount() const
    {
        return std::max<size_t>(1, (library_->size() + PAGE_SIZE - 1) / PAGE_SIZE);
    }

    void EventsScreen::select(size_t card)
    {
        if (card == selected_ || card >= library_->size())
        {
            return;
        }
        selected_ = card;
        language_ = 0;
        message_.clear();
    }

    void EventsScreen::move(Action action)
    {
        constexpr size_t columns = EventsView::COLUMNS;
        const size_t count       = library_->size();
        const size_t onPage      = selected_ % PAGE_SIZE;
        switch (action)
        {
            case Action::Up:
                if (selected_ >= columns)
                {
                    select(selected_ - columns);
                }
                break;
            case Action::Down:
                if (selected_ + columns < count)
                {
                    select(selected_ + columns);
                }
                else if (selected_ / columns + 1 < (count + columns - 1) / columns)
                {
                    // Onto the last card when the row below is shorter.
                    select(count - 1);
                }
                break;
            case Action::Left:
                if (onPage % columns != 0)
                {
                    select(selected_ - 1);
                }
                else if (page() > 0)
                {
                    // Onto the same row of the page before.
                    select(selected_ - PAGE_SIZE + columns - 1);
                }
                break;
            case Action::Right:
                if (onPage % columns != columns - 1 && selected_ + 1 < count)
                {
                    select(selected_ + 1);
                }
                else if (onPage % columns == columns - 1 && page() + 1 < pageCount())
                {
                    select(std::min(selected_ + PAGE_SIZE - (columns - 1), count - 1));
                }
                break;
            default:
                break;
        }
    }

    void EventsScreen::changePage(int direction)
    {
        const size_t pages  = pageCount();
        const size_t onPage = selected_ % PAGE_SIZE;
        const size_t next   = direction > 0 ? (page() + 1) % pages : (page() + pages - 1) % pages;
        select(std::min(next * PAGE_SIZE + onPage, library_->size() - 1));
    }

    void EventsScreen::stepLanguage(int direction)
    {
        if (languages_.empty() || !library_->card(selected_)->pokemon())
        {
            return;
        }
        const size_t count = languages_.size();
        language_ = direction > 0 ? (language_ + 1) % count : (language_ + count - 1) % count;
        message_.clear();
    }

    void EventsScreen::add()
    {
        pksm::Sav& save = session_.save();
        const auto card = library_->card(selected_);
        int position    = 0;
        if (card->pokemon())
        {
            // The save puts the Pokémon in the first free box slot, and adds nothing when there
            // is none; look for it first, to say where it went.
            int slot = 0;
            for (; slot < save.maxSlot(); ++slot)
            {
                const auto pokemon = save.pkm(static_cast<u8>(slot / BOX_SLOTS),
                    static_cast<u8>(slot % BOX_SLOTS));
                if (pokemon && pokemon->species() == pksm::Species::None)
                {
                    break;
                }
            }
            if (slot == save.maxSlot())
            {
                message_ = tr(Text::EventBoxesFull);
                return;
            }
            save.mysteryGift(*card, position);
            finishPokemon(selected_, slot);
            message_ = tr(Text::EventAddedToBox, {std::to_string(slot / BOX_SLOTS + 1),
                                                     std::to_string(slot % BOX_SLOTS + 1)});
        }
        else
        {
            save.mysteryGift(*card, position);
            message_ = tr(Text::EventAdded);
        }
        session_.markDirty();
    }

    void EventsScreen::finishPokemon(size_t index, int slot)
    {
        pksm::Sav& save   = session_.save();
        const u8 box      = static_cast<u8>(slot / BOX_SLOTS);
        const u8 boxSlot  = static_cast<u8>(slot % BOX_SLOTS);
        auto pokemon      = save.pkm(box, boxSlot);
        const auto card   = library_->card(index);
        if (!pokemon || pokemon->species() == pksm::Species::None)
        {
            return;
        }

        // The save builds the Pokémon in its own language; give it the chosen one's names.
        const pksm::Language chosen = languages_.empty() ? save.language() : languages_[language_];
        if (chosen != save.language())
        {
            const pksm::Language named = card->nicknameLanguage(chosen);
            const pksm::Language language = named != pksm::Language(0) ? named : chosen;
            pokemon->language(language);
            if (card->egg())
            {
                pokemon->nicknamed(true);
                pokemon->nickname(i18n::species(language, pksm::Species::None));
            }
            else
            {
                pokemon->nicknamed(card->nicknamed(language));
                pokemon->nickname(pokemon->nicknamed() ? card->nickname(language)
                                                       : pokemon->species().localize(language));
            }
            const std::string trainer = card->otName(chosen);
            if (!trainer.empty())
            {
                pokemon->otName(trainer);
            }
        }

        // The save reads the card's shiny type with other numbers than the game; make the PID
        // as the game does.
        switch (static_cast<ShinyType>(card->PIDType()))
        {
            case ShinyType::Never:
                pokemon->PID(pksm::randomNumber(0, 0xFFFFFFFF));
                pokemon->shiny(false);
                break;
            case ShinyType::Random:
                pokemon->PID(pksm::randomNumber(0, 0xFFFFFFFF));
                break;
            case ShinyType::AlwaysStar:
            case ShinyType::AlwaysSquare:
            {
                const u32 low  = pksm::randomNumber(0, 0xFFFF);
                const u32 xorValue = card->PIDType() == u8(ShinyType::AlwaysStar) ? 1 : 0;
                const u32 high = (pokemon->TID() ^ pokemon->SID() ^ low ^ xorValue) & 0xFFFF;
                pokemon->PID((high << 16) | low);
                break;
            }
            case ShinyType::Fixed:
                pokemon->PID(card->PID());
                break;
        }
        savePokemon(save, *pokemon, false, box, boxSlot);
    }

    void EventsScreen::update(App& app, Action action)
    {
        switch (action)
        {
            case Action::Quit:
                if (!session_.dirty())
                {
                    app.quit();
                    return;
                }
                message_ = tr(Text::UnsavedBeforeQuit);
                break;
            case Action::Back:
                // The game menu below keeps the save open and warns about unsaved changes.
                app.pop();
                break;
            case Action::Save:
                if (session_.dirty())
                {
                    session_.saveWithMessage(message_);
                }
                break;
            case Action::Up:
            case Action::Down:
            case Action::Left:
            case Action::Right:
                move(action);
                break;
            case Action::PreviousPage:
            case Action::NextPage:
                changePage(action == Action::NextPage ? 1 : -1);
                break;
            case Action::PreviousTab:
            case Action::NextTab:
                stepLanguage(action == Action::NextTab ? 1 : -1);
                break;
            case Action::Confirm:
                add();
                break;
            default:
                break;
        }
    }

    void EventsScreen::describe(EventsView& view) const
    {
        const pksm::Sav& save         = session_.save();
        const pksm::Language language = save.language();
        const auto card               = library_->card(selected_);
        view.caption = tr(Text::EventCaption,
            {padDigits(card->ID(), 4), upper(gameName(library_->entry(selected_).game))});

        if (card->pokemon())
        {
            if (card->egg())
            {
                view.marks.push_back(tr(Text::FieldEgg));
            }
            if (card->canGigantamax())
            {
                view.marks.push_back(tr(Text::EventGigantamax));
            }
            const auto shinyType = static_cast<ShinyType>(card->PIDType());
            if (shinyType == ShinyType::AlwaysStar || shinyType == ShinyType::AlwaysSquare)
            {
                view.marks.push_back(tr(Text::EventShiny));
            }

            const unsigned int species = static_cast<unsigned int>(card->species());
            const std::string level    = card->level() != 0
                                             ? tr(Text::LevelShort, {numericValue(card->level())})
                                             : std::string(tr(Text::EventRandomLevel));
            view.info.push_back({tr(Text::FieldSpecies),
                tr(Text::DexNumber, {padDigits(species, 3)}) + " " +
                    card->species().localize(language) + " · " + level});

            const pksm::Ball ball = card->ball() != pksm::Ball::None ? card->ball()
                                                                     : pksm::Ball::Poke;
            const size_t ballIndex = static_cast<size_t>(ball);
            view.ball              = ballIndex <= 25 ? ballIndex : 0;
            view.info.push_back({tr(Text::FieldBall), ball.localize(language), true});

            const pksm::Language chosen =
                languages_.empty() ? language : languages_[language_];
            const std::string trainer = card->otName(chosen);
            std::string otId          = tr(Text::EventYourTrainer);
            if (!trainer.empty() && card->otGender() < pksm::Gender::Genderless)
            {
                otId = trainer + " · " +
                       padDigits(showTrainerIds(card->TID(), card->SID()).tid, 6);
            }
            view.info.push_back({tr(Text::EventOtId), otId});

            const u16 item = card->heldItem();
            view.info.push_back({tr(Text::FieldItem),
                item != 0 ? i18n::item(language, item) : std::string(tr(Text::DetailNone))});

            for (u8 i = 0; i < 4; ++i)
            {
                const pksm::Move move = card->move(i);
                view.moves.push_back(
                    move != pksm::Move::None ? i18n::move(language, move) : std::string("—"));
            }

            const std::vector<std::string> codes = languageCodes();
            for (const pksm::Language shown : languages_)
            {
                const size_t at = static_cast<size_t>(
                    std::find(LANGUAGES.begin(), LANGUAGES.end(), shown) - LANGUAGES.begin());
                view.languages.push_back(codes[at]);
            }
            view.language    = language_;
            view.destination = tr(Text::EventToBoxes);
        }
        else if (card->BP())
        {
            view.info.push_back(
                {tr(Text::EventBattlePoints), numericValue(card->objectQuantity(0))});
            view.destination = tr(Text::EventToPoints);
        }
        else if (card->clothing())
        {
            view.info.push_back(
                {tr(Text::EventClothingPieces), numericValue(static_cast<unsigned>(card->items()))});
            view.destination = tr(Text::EventToWardrobe);
        }
        else
        {
            for (int i = 0; i < card->items(); ++i)
            {
                view.info.push_back({tr(Text::FieldItem),
                    tr(Text::EventItemCount, {i18n::item(language, card->object(i)),
                                                 numericValue(card->objectQuantity(i))})});
            }
            view.destination = tr(Text::EventToBag);
        }
    }

    void EventsScreen::draw(SwitchUi& ui)
    {
        EventsView view;
        const size_t first = page() * PAGE_SIZE;
        const size_t last  = std::min(library_->size(), first + PAGE_SIZE);
        view.cards.assign(cards_.begin() + static_cast<std::ptrdiff_t>(first),
            cards_.begin() + static_cast<std::ptrdiff_t>(last));
        view.selected  = selected_ - first;
        view.page      = page();
        view.pageCount = pageCount();
        describe(view);
        view.message = sessionMessage(session_, message_);
        ui.drawEvents(view);

        // A touch on a card, a language or the add button.
        if (view.selected != selected_ - first)
        {
            select(first + view.selected);
        }
        else if (view.language != language_ && view.language < languages_.size())
        {
            language_ = view.language;
            message_.clear();
        }
        else if (view.addRequest)
        {
            add();
        }
    }
}
