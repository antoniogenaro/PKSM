#include "domain/LetsGoStorage.hpp"

#include "enums/Generation.hpp"
#include "sav/SavLGPE.hpp"
#include <algorithm>

namespace pksm::switchui::letsgo
{
    namespace
    {
        constexpr int BOX_SLOTS = 30;

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
        const auto place = lgpe->partyPlace(static_cast<u16>(index(box, slot)));
        return place ? *place + 1 : 0;
    }

    void swapped(pksm::Sav& save, int boxA, size_t slotA, int boxB, size_t slotB)
    {
        if (pksm::SavLGPE* lgpe = letsGo(save))
        {
            lgpe->slotsSwapped(
                static_cast<u16>(index(boxA, slotA)), static_cast<u16>(index(boxB, slotB)));
        }
    }

    void dropEmptyPartyPlaces(pksm::Sav& save)
    {
        if (pksm::SavLGPE* lgpe = letsGo(save))
        {
            lgpe->removeEmptyFromParty();
        }
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
        copy.packBoxes();
        copy.finishEditing();
        return bytes;
    }
}
