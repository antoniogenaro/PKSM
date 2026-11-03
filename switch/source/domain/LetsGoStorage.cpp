#include "domain/LetsGoStorage.hpp"

#include "enums/Generation.hpp"
#include "enums/Species.hpp"
#include "pkx/PKX.hpp"
#include "sav/SavLGPE.hpp"
#include <algorithm>

namespace pksm::switchui::letsgo
{
    namespace
    {
        constexpr int BOX_SLOTS = 30;
        constexpr u8 PARTY_SLOTS = 6;
        // What the party list holds for an empty place.
        constexpr u16 NO_SLOT = 1001;

        const pksm::SavLGPE* letsGo(const pksm::Sav& save)
        {
            return save.generation() == pksm::Generation::LGPE
                       ? static_cast<const pksm::SavLGPE*>(&save)
                       : nullptr;
        }

        pksm::SavLGPE* letsGo(pksm::Sav& save)
        {
            return save.generation() == pksm::Generation::LGPE ? static_cast<pksm::SavLGPE*>(&save)
                                                               : nullptr;
        }

        int index(int box, size_t slot)
        {
            return box * BOX_SLOTS + static_cast<int>(slot);
        }
    }

    bool slotExists(const pksm::Sav& save, int box, size_t slot)
    {
        return box >= 0 && slot < BOX_SLOTS && index(box, slot) < save.maxSlot();
    }

    size_t partyPlace(const pksm::Sav& save, int box, size_t slot)
    {
        const pksm::SavLGPE* lgpe = letsGo(save);
        if (!lgpe)
        {
            return 0;
        }
        for (u8 place = 0; place < PARTY_SLOTS; ++place)
        {
            if (lgpe->partyBoxSlot(place) == index(box, slot))
            {
                return place + 1;
            }
        }
        return 0;
    }

    void swapped(pksm::Sav& save, int boxA, size_t slotA, int boxB, size_t slotB)
    {
        pksm::SavLGPE* lgpe = letsGo(save);
        if (!lgpe)
        {
            return;
        }
        const u16 a       = static_cast<u16>(index(boxA, slotA));
        const u16 b       = static_cast<u16>(index(boxB, slotB));
        const auto follow = [a, b](u16 at) { return at == a ? b : at == b ? a : at; };
        for (u8 place = 0; place < PARTY_SLOTS; ++place)
        {
            lgpe->partyBoxSlot(place, follow(lgpe->partyBoxSlot(place)));
        }
        lgpe->followPkm(follow(lgpe->followPkm()));
    }

    void dropEmptyPartyPlaces(pksm::Sav& save)
    {
        pksm::SavLGPE* lgpe = letsGo(save);
        if (!lgpe)
        {
            return;
        }
        for (u8 place = 0; place < PARTY_SLOTS; ++place)
        {
            const u16 at = lgpe->partyBoxSlot(place);
            if (at == NO_SLOT)
            {
                continue;
            }
            const auto pokemon = lgpe->pkm(static_cast<u8>(at / BOX_SLOTS),
                static_cast<u8>(at % BOX_SLOTS));
            if (!pokemon || pokemon->species() == pksm::Species::None)
            {
                lgpe->partyBoxSlot(place, NO_SLOT);
            }
        }
        lgpe->fixParty();
    }

    std::shared_ptr<u8[]> packedForWriting(const pksm::Sav& save)
    {
        if (!letsGo(save))
        {
            return nullptr;
        }
        // The whole file, with any wrapper it came in.
        const size_t length = save.getEntireLengthIncludingFooter();
        auto bytes          = std::shared_ptr<u8[]>(new u8[length]);
        std::copy_n(save.rawData().get(), length, bytes.get());

        pksm::SavLGPE copy(bytes, save.getLength());
        copy.fixParty();
        copy.compressBox();
        // The game reads how many Pokémon the packed list holds.
        u16 stored = 0;
        while (stored < copy.maxSlot())
        {
            const auto pokemon = copy.pkm(
                static_cast<u8>(stored / BOX_SLOTS), static_cast<u8>(stored % BOX_SLOTS));
            if (!pokemon || pokemon->species() == pksm::Species::None)
            {
                break;
            }
            ++stored;
        }
        copy.boxedPkm(stored);
        copy.finishEditing();
        return bytes;
    }
}
