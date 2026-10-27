#include "domain/FormSprites.hpp"
#include "pkx/PKX.hpp"
// The types sheet's sprite indices, as tex3ds generated them for the 3DS app.
#include "../../../3ds/include/gui/types_spritesheet.h"
#include <array>
#include <optional>
#include <span>

// Copied from the 3DS app (3ds/source/gui/gui.cpp) so both pick the same sprites.
namespace
{
    std::optional<size_t> multiGenderSprite(pksm::Species s)
    {
        switch (s)
        {
            case pksm::Species::Unfezant:
                return types_spritesheet_521_f_idx;
            case pksm::Species::Frillish:
                return types_spritesheet_592_f_idx;
            case pksm::Species::Jellicent:
                return types_spritesheet_593_f_idx;
            case pksm::Species::Pyroar:
                return types_spritesheet_668_f_idx;
            case pksm::Species::Indeedee:
                return types_spritesheet_876_f_idx;
            default:
                return std::nullopt;
        }
    }

// Creates a form array with proper storage duration
#define FA(...)                                                                                    \
    []<std::array Values = { __VA_ARGS__ }>() consteval                                            \
        ->std::span<const typename decltype(Values)::value_type>                                   \
    {                                                                                              \
        return Values;                                                                             \
    }                                                                                              \
    ()

    std::span<const int> formIndices(pksm::Generation version, pksm::Species species)
    {
        switch (species)
        {
            case pksm::Species::Venusaur:
                return FA(0, types_spritesheet_3_m_idx);
            case pksm::Species::Charizard:
                return FA(0, types_spritesheet_6_m1_idx, types_spritesheet_6_m2_idx);
            case pksm::Species::Blastoise:
                return FA(0, types_spritesheet_9_m_idx);
            case pksm::Species::Beedrill:
                return FA(0, types_spritesheet_15_m_idx);
            case pksm::Species::Pidgeot:
                return FA(0, types_spritesheet_18_m_idx);
            case pksm::Species::Rattata:
                return FA(0, types_spritesheet_19_1_idx);
            case pksm::Species::Raticate:
                return FA(0, types_spritesheet_20_1_idx, types_spritesheet_20_t_idx);
            case pksm::Species::Pikachu:
                switch (version)
                {
                    case pksm::Generation::SIX:
                        return FA(0, types_spritesheet_25_cosplay1_idx,
                            types_spritesheet_25_cosplay2_idx, types_spritesheet_25_cosplay3_idx,
                            types_spritesheet_25_cosplay4_idx, types_spritesheet_25_cosplay5_idx,
                            types_spritesheet_25_cosplay6_idx);
                    case pksm::Generation::SEVEN:
                    case pksm::Generation::LGPE:
                    case pksm::Generation::EIGHT:
                        return FA(0, types_spritesheet_25_cap1_idx, types_spritesheet_25_cap2_idx,
                            types_spritesheet_25_cap3_idx, types_spritesheet_25_cap4_idx,
                            types_spritesheet_25_cap5_idx, types_spritesheet_25_cap6_idx,
                            types_spritesheet_25_cap7_idx, 0, types_spritesheet_25_cap9_idx);
                    default:
                        return FA(0);
                }
            case pksm::Species::Raichu:
                return FA(0, types_spritesheet_26_1_idx);
            case pksm::Species::Sandshrew:
                return FA(0, types_spritesheet_27_1_idx);
            case pksm::Species::Sandslash:
                return FA(0, types_spritesheet_28_1_idx);
            case pksm::Species::Vulpix:
                return FA(0, types_spritesheet_37_1_idx);
            case pksm::Species::Ninetales:
                return FA(0, types_spritesheet_38_1_idx);
            case pksm::Species::Diglett:
                return FA(0, types_spritesheet_50_1_idx);
            case pksm::Species::Dugtrio:
                return FA(0, types_spritesheet_51_1_idx);
            case pksm::Species::Meowth:
                return FA(0, types_spritesheet_52_1_idx, types_spritesheet_52_2_idx);
            case pksm::Species::Persian:
                return FA(0, types_spritesheet_53_1_idx);
            case pksm::Species::Alakazam:
                return FA(0, types_spritesheet_65_m_idx);
            case pksm::Species::Geodude:
                return FA(0, types_spritesheet_74_1_idx);
            case pksm::Species::Graveler:
                return FA(0, types_spritesheet_75_1_idx);
            case pksm::Species::Golem:
                return FA(0, types_spritesheet_76_1_idx);
            case pksm::Species::Ponyta:
                return FA(0, types_spritesheet_77_1_idx);
            case pksm::Species::Rapidash:
                return FA(0, types_spritesheet_78_1_idx);
            case pksm::Species::Slowpoke:
                return FA(0, types_spritesheet_79_1_idx);
            case pksm::Species::Slowbro:
                switch (version)
                {
                    case pksm::Generation::SIX:
                    case pksm::Generation::SEVEN:
                        return FA(0, types_spritesheet_80_m_idx);
                    case pksm::Generation::EIGHT:
                        return FA(0, types_spritesheet_80_1_idx);
                    default:
                        return FA(0);
                }
            case pksm::Species::Slowking:
                return FA(0, types_spritesheet_199_1_idx);
            case pksm::Species::Farfetchd:
                return FA(0, types_spritesheet_83_1_idx);
            case pksm::Species::Grimer:
                return FA(0, types_spritesheet_88_1_idx);
            case pksm::Species::Muk:
                return FA(0, types_spritesheet_89_1_idx);
            case pksm::Species::Gengar:
                return FA(0, types_spritesheet_94_m_idx);
            case pksm::Species::Exeggutor:
                return FA(0, types_spritesheet_103_1_idx);
            case pksm::Species::Marowak:
                return FA(0, types_spritesheet_105_1_idx, types_spritesheet_105_t_idx);
            case pksm::Species::Weezing:
                return FA(0, types_spritesheet_110_1_idx);
            case pksm::Species::Kangaskhan:
                return FA(0, types_spritesheet_115_m_idx);
            case pksm::Species::MrMime:
                return FA(0, types_spritesheet_122_1_idx);
            case pksm::Species::Pinsir:
                return FA(0, types_spritesheet_127_m_idx);
            case pksm::Species::Gyarados:
                return FA(0, types_spritesheet_130_m_idx);
            case pksm::Species::Eevee:
                return FA(0, 0);
            case pksm::Species::Aerodactyl:
                return FA(0, types_spritesheet_142_m_idx);
            case pksm::Species::Articuno:
                return FA(0, types_spritesheet_144_1_idx);
            case pksm::Species::Zapdos:
                return FA(0, types_spritesheet_145_1_idx);
            case pksm::Species::Moltres:
                return FA(0, types_spritesheet_146_1_idx);
            case pksm::Species::Mewtwo:
                return FA(0, types_spritesheet_150_m1_idx, types_spritesheet_150_m2_idx);
            case pksm::Species::Pichu:
                return FA(0, types_spritesheet_172_1_idx);
            case pksm::Species::Ampharos:
                return FA(0, types_spritesheet_181_m_idx);
            case pksm::Species::Unown:
                return FA(0, types_spritesheet_201_1_idx, types_spritesheet_201_2_idx,
                    types_spritesheet_201_3_idx, types_spritesheet_201_4_idx,
                    types_spritesheet_201_5_idx, types_spritesheet_201_6_idx,
                    types_spritesheet_201_7_idx, types_spritesheet_201_8_idx,
                    types_spritesheet_201_9_idx, types_spritesheet_201_10_idx,
                    types_spritesheet_201_11_idx, types_spritesheet_201_12_idx,
                    types_spritesheet_201_13_idx, types_spritesheet_201_14_idx,
                    types_spritesheet_201_15_idx, types_spritesheet_201_16_idx,
                    types_spritesheet_201_17_idx, types_spritesheet_201_18_idx,
                    types_spritesheet_201_19_idx, types_spritesheet_201_20_idx,
                    types_spritesheet_201_21_idx, types_spritesheet_201_22_idx,
                    types_spritesheet_201_23_idx, types_spritesheet_201_24_idx,
                    types_spritesheet_201_25_idx, types_spritesheet_201_26_idx,
                    types_spritesheet_201_27_idx);
            case pksm::Species::Steelix:
                return FA(0, types_spritesheet_208_m_idx);
            case pksm::Species::Scizor:
                return FA(0, types_spritesheet_212_m_idx);
            case pksm::Species::Heracross:
                return FA(0, types_spritesheet_214_m_idx);
            case pksm::Species::Corsola:
                return FA(0, types_spritesheet_222_1_idx);
            case pksm::Species::Houndoom:
                return FA(0, types_spritesheet_229_m_idx);
            case pksm::Species::Tyranitar:
                return FA(0, types_spritesheet_248_m_idx);
            case pksm::Species::Sceptile:
                return FA(0, types_spritesheet_254_m_idx);
            case pksm::Species::Blaziken:
                return FA(0, types_spritesheet_257_m_idx);
            case pksm::Species::Swampert:
                return FA(0, types_spritesheet_260_m_idx);
            case pksm::Species::Zigzagoon:
                return FA(0, types_spritesheet_263_1_idx);
            case pksm::Species::Linoone:
                return FA(0, types_spritesheet_264_1_idx);
            case pksm::Species::Gardevoir:
                return FA(0, types_spritesheet_282_m_idx);
            case pksm::Species::Sableye:
                return FA(0, types_spritesheet_302_m_idx);
            case pksm::Species::Mawile:
                return FA(0, types_spritesheet_303_m_idx);
            case pksm::Species::Aggron:
                return FA(0, types_spritesheet_306_m_idx);
            case pksm::Species::Medicham:
                return FA(0, types_spritesheet_308_m_idx);
            case pksm::Species::Manectric:
                return FA(0, types_spritesheet_310_m_idx);
            case pksm::Species::Sharpedo:
                return FA(0, types_spritesheet_319_m_idx);
            case pksm::Species::Camerupt:
                return FA(0, types_spritesheet_323_m_idx);
            case pksm::Species::Altaria:
                return FA(0, types_spritesheet_334_m_idx);
            case pksm::Species::Castform:
                return FA(0, types_spritesheet_351_1_idx, types_spritesheet_351_2_idx,
                    types_spritesheet_351_3_idx);
            case pksm::Species::Banette:
                return FA(0, types_spritesheet_354_m_idx);
            case pksm::Species::Absol:
                return FA(0, types_spritesheet_359_m_idx);
            case pksm::Species::Glalie:
                return FA(0, types_spritesheet_362_m_idx);
            case pksm::Species::Salamence:
                return FA(0, types_spritesheet_373_m_idx);
            case pksm::Species::Metagross:
                return FA(0, types_spritesheet_376_m_idx);
            case pksm::Species::Latias:
                return FA(0, types_spritesheet_380_m_idx);
            case pksm::Species::Latios:
                return FA(0, types_spritesheet_381_m_idx);
            case pksm::Species::Kyogre:
                return FA(0, types_spritesheet_382_m_idx);
            case pksm::Species::Groudon:
                return FA(0, types_spritesheet_383_m_idx);
            case pksm::Species::Rayquaza:
                return FA(0, types_spritesheet_384_m_idx);
            case pksm::Species::Deoxys:
                return FA(0, types_spritesheet_386_1_idx, types_spritesheet_386_2_idx,
                    types_spritesheet_386_3_idx);
            case pksm::Species::Burmy:
                return FA(0, types_spritesheet_412_1_idx, types_spritesheet_412_2_idx);
            case pksm::Species::Wormadam:
                return FA(0, types_spritesheet_413_1_idx, types_spritesheet_413_2_idx);
            case pksm::Species::Cherrim:
                return FA(0, types_spritesheet_421_1_idx);
            case pksm::Species::Shellos:
                return FA(0, types_spritesheet_422_1_idx);
            case pksm::Species::Gastrodon:
                return FA(0, types_spritesheet_423_1_idx);
            case pksm::Species::Lopunny:
                return FA(0, types_spritesheet_428_m_idx);
            case pksm::Species::Garchomp:
                return FA(0, types_spritesheet_445_m_idx);
            case pksm::Species::Lucario:
                return FA(0, types_spritesheet_448_m_idx);
            case pksm::Species::Abomasnow:
                return FA(0, types_spritesheet_460_m_idx);
            case pksm::Species::Gallade:
                return FA(0, types_spritesheet_475_m_idx);
            case pksm::Species::Rotom:
                return FA(0, types_spritesheet_479_1_idx, types_spritesheet_479_2_idx,
                    types_spritesheet_479_3_idx, types_spritesheet_479_4_idx,
                    types_spritesheet_479_5_idx);
            case pksm::Species::Giratina:
                return FA(0, types_spritesheet_487_1_idx);
            case pksm::Species::Shaymin:
                return FA(0, types_spritesheet_492_1_idx);
            case pksm::Species::Audino:
                return FA(0, types_spritesheet_531_m_idx);
            case pksm::Species::Basculin:
                return FA(0, types_spritesheet_550_1_idx);
            case pksm::Species::Darumaka:
                return FA(0, types_spritesheet_554_1_idx);
            case pksm::Species::Darmanitan:
                return FA(0, types_spritesheet_555_1_idx, types_spritesheet_555_2_idx,
                    types_spritesheet_555_3_idx);
            case pksm::Species::Yamask:
                return FA(0, types_spritesheet_562_1_idx);
            case pksm::Species::Deerling:
                return FA(0, types_spritesheet_585_1_idx, types_spritesheet_585_2_idx,
                    types_spritesheet_585_3_idx);
            case pksm::Species::Sawsbuck:
                return FA(0, types_spritesheet_586_1_idx, types_spritesheet_586_2_idx,
                    types_spritesheet_586_3_idx);
            case pksm::Species::Stunfisk:
                return FA(0, types_spritesheet_618_1_idx);
            case pksm::Species::Tornadus:
                return FA(0, types_spritesheet_641_1_idx);
            case pksm::Species::Thundurus:
                return FA(0, types_spritesheet_642_1_idx);
            case pksm::Species::Landorus:
                return FA(0, types_spritesheet_645_1_idx);
            case pksm::Species::Kyurem:
                return FA(0, types_spritesheet_646_1_idx, types_spritesheet_646_2_idx);
            case pksm::Species::Keldeo:
                return FA(0, types_spritesheet_647_1_idx);
            case pksm::Species::Meloetta:
                return FA(0, types_spritesheet_648_1_idx);
            case pksm::Species::Greninja:
                return FA(0, types_spritesheet_658_1_idx, types_spritesheet_658_2_idx);
            case pksm::Species::Vivillon:
                return FA(0, types_spritesheet_666_1_idx, types_spritesheet_666_2_idx,
                    types_spritesheet_666_3_idx, types_spritesheet_666_4_idx,
                    types_spritesheet_666_5_idx, types_spritesheet_666_6_idx,
                    types_spritesheet_666_7_idx, types_spritesheet_666_8_idx,
                    types_spritesheet_666_9_idx, types_spritesheet_666_10_idx,
                    types_spritesheet_666_11_idx, types_spritesheet_666_12_idx,
                    types_spritesheet_666_13_idx, types_spritesheet_666_14_idx,
                    types_spritesheet_666_15_idx, types_spritesheet_666_16_idx,
                    types_spritesheet_666_17_idx, types_spritesheet_666_18_idx,
                    types_spritesheet_666_19_idx);
            case pksm::Species::Flabebe:
                return FA(0, types_spritesheet_669_1_idx, types_spritesheet_669_2_idx,
                    types_spritesheet_669_3_idx, types_spritesheet_669_4_idx);
            case pksm::Species::Floette:
                return FA(0, types_spritesheet_670_1_idx, types_spritesheet_670_2_idx,
                    types_spritesheet_670_3_idx, types_spritesheet_670_4_idx,
                    types_spritesheet_670_5_idx);
            case pksm::Species::Florges:
                return FA(0, types_spritesheet_671_1_idx, types_spritesheet_671_2_idx,
                    types_spritesheet_671_3_idx, types_spritesheet_671_4_idx);
            case pksm::Species::Furfrou:
                return FA(0, types_spritesheet_676_1_idx, types_spritesheet_676_2_idx,
                    types_spritesheet_676_3_idx, types_spritesheet_676_4_idx,
                    types_spritesheet_676_5_idx, types_spritesheet_676_6_idx,
                    types_spritesheet_676_7_idx, types_spritesheet_676_8_idx,
                    types_spritesheet_676_9_idx);
            case pksm::Species::Aegislash:
                return FA(0, types_spritesheet_681_1_idx);
            case pksm::Species::Zygarde:
                return FA(0, types_spritesheet_718_1_idx, types_spritesheet_718_2_idx,
                    types_spritesheet_718_3_idx, types_spritesheet_718_4_idx);
            case pksm::Species::Diancie:
                return FA(0, types_spritesheet_719_m_idx);
            case pksm::Species::Hoopa:
                return FA(0, types_spritesheet_720_1_idx);
            case pksm::Species::Gumshoos:
                return FA(0, types_spritesheet_735_t_idx);
            case pksm::Species::Vikavolt:
                return FA(0, types_spritesheet_738_t_idx);
            case pksm::Species::Oricorio:
                return FA(0, types_spritesheet_741_1_idx, types_spritesheet_741_2_idx,
                    types_spritesheet_741_3_idx);
            case pksm::Species::Ribombee:
                return FA(0, types_spritesheet_743_t_idx);
            case pksm::Species::Lycanroc:
                return FA(0, types_spritesheet_745_1_idx, types_spritesheet_745_2_idx);
            case pksm::Species::Wishiwashi:
                return FA(0, types_spritesheet_746_1_idx);
            case pksm::Species::Araquanid:
                return FA(0, types_spritesheet_752_t_idx);
            case pksm::Species::Lurantis:
                return FA(0, types_spritesheet_754_t_idx);
            case pksm::Species::Salazzle:
                return FA(0, types_spritesheet_758_t_idx);
            case pksm::Species::Minior:
                return FA(0, 0, 0, 0, 0, 0, 0, types_spritesheet_774_7_idx,
                    types_spritesheet_774_8_idx, types_spritesheet_774_9_idx,
                    types_spritesheet_774_10_idx, types_spritesheet_774_11_idx,
                    types_spritesheet_774_12_idx, types_spritesheet_774_13_idx);
            case pksm::Species::Togedemaru:
                return FA(0, types_spritesheet_777_t_idx);
            case pksm::Species::Mimikyu:
                return FA(0, 0, types_spritesheet_778_t_idx, types_spritesheet_778_t_idx);
            case pksm::Species::Kommoo:
                return FA(0, types_spritesheet_784_t_idx);
            case pksm::Species::Necrozma:
                return FA(0, types_spritesheet_800_1_idx, types_spritesheet_800_2_idx,
                    types_spritesheet_800_3_idx);
            case pksm::Species::Magearna:
                return FA(0, types_spritesheet_801_1_idx);
            case pksm::Species::Cramorant:
                return FA(0, types_spritesheet_845_1_idx, types_spritesheet_845_2_idx);
            case pksm::Species::Toxtricity:
                return FA(0, types_spritesheet_849_1_idx);
            case pksm::Species::Alcremie:
                return FA(0, types_spritesheet_869_1_idx, types_spritesheet_869_2_idx,
                    types_spritesheet_869_3_idx, types_spritesheet_869_4_idx,
                    types_spritesheet_869_5_idx, types_spritesheet_869_6_idx,
                    types_spritesheet_869_7_idx, types_spritesheet_869_8_idx);
            case pksm::Species::Eiscue:
                return FA(0, types_spritesheet_875_1_idx);
            case pksm::Species::Morpeko:
                return FA(0, types_spritesheet_877_1_idx);
            case pksm::Species::Zacian:
                return FA(0, types_spritesheet_888_1_idx);
            case pksm::Species::Zamazenta:
                return FA(0, types_spritesheet_889_1_idx);
            case pksm::Species::Eternatus:
                return FA(0, types_spritesheet_890_g_idx);
            case pksm::Species::Urshifu:
                return FA(0, types_spritesheet_892_1_idx);
            case pksm::Species::Zarude:
                return FA(0, types_spritesheet_893_1_idx);
            case pksm::Species::Calyrex:
                return FA(0, types_spritesheet_898_1_idx, types_spritesheet_898_2_idx);
            default:
                return FA(0);
        }
    }

#undef FA
}

namespace pksm::switchui
{
    size_t formSpriteIndex(const pksm::PKX& pokemon)
    {
        const pksm::Species species = pokemon.species();
        if (species == pksm::Species::None || species > pksm::Species::Calyrex)
        {
            return 0;
        }
        if (auto sprite = multiGenderSprite(species);
            sprite && pokemon.gender() == pksm::Gender::Female)
        {
            return *sprite;
        }
        const auto forms = formIndices(pokemon.generation(), species);
        const size_t form = pokemon.alternativeForm();
        return form < forms.size() ? static_cast<size_t>(forms[form]) : 0;
    }
}
