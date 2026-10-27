#pragma once

#include <initializer_list>
#include <string>
#include <string_view>

// The key of every text the Switch app shows. The texts themselves live in one JSON file per
// language in switch/strings (en.json, es.json, ...), keyed by these names. To add a text, add
// its key here and to every file. A "{}" marks where tr() inserts a value, in order; every
// language must use the same number of them.
#define PKSM_SWITCH_TEXTS(X)                                                                       \
    /* Shared */                                                                                   \
    X(Yes)                                                                                         \
    X(No)                                                                                          \
    X(Empty)                                                                                       \
    X(NotAvailable)                                                                                \
    X(WithId)                                                                                      \
    X(PendingChanges)                                                                              \
    X(ExportedTo)                                                                                  \
    X(UnsavedBeforeQuit)                                                                           \
    X(UnsavedBeforeBack)                                                                           \
    /* Games */                                                                                    \
    X(GameSword)                                                                                   \
    X(GameShield)                                                                                  \
    X(GameFireRed)                                                                                 \
    X(GameLeafGreen)                                                                               \
    X(GroupGen3Gba)                                                                                \
    X(GroupGen8SwordShield)                                                                        \
    /* Start-up */                                                                                \
    X(SdlInitFailed)                                                                               \
    X(WindowFailed)                                                                                \
    X(RendererFailed)                                                                              \
    X(ImGuiPlatformFailed)                                                                         \
    X(ImGuiRendererFailed)                                                                         \
    X(PokemonSheet)                                                                                \
    X(BallSheet)                                                                                   \
    X(BallSheetIncomplete)                                                                         \
    X(SheetOpenFailed)                                                                             \
    X(SheetIncomplete)                                                                             \
    X(SheetBadHeader)                                                                              \
    X(SheetUnsupported)                                                                            \
    X(SheetTableIncomplete)                                                                        \
    X(SheetDecompressFailed)                                                                       \
    X(SheetBadCoordinates)                                                                         \
    X(TextureCreateFailed)                                                                         \
    X(TextureLoadFailed)                                                                           \
    X(FontLoadFailed)                                                                              \
    X(IconLoadFailed)                                                                              \
    /* SD card and files */                                                                        \
    X(SdAccessFailed)                                                                              \
    X(SaveDirectoryFailed)                                                                         \
    X(DirectoryTooDeep)                                                                            \
    X(DirectoryReadFailed)                                                                         \
    X(DirectoryListFailed)                                                                         \
    X(DirectoryCloseFailed)                                                                        \
    X(FileInspectFailed)                                                                           \
    X(FileOpenFailed)                                                                              \
    X(FileSizeFailed)                                                                              \
    X(FileSizeInvalid)                                                                             \
    X(FileReadIncomplete)                                                                          \
    X(SaveParseFailed)                                                                             \
    X(SaveUnsupported)                                                                             \
    X(ExportNoFreeName)                                                                            \
    X(ExportCreateFailed)                                                                          \
    X(ExportOpenFailed)                                                                            \
    X(ExportEmptyNotRemoved)                                                                       \
    X(ExportPartialRemoved)                                                                        \
    X(ExportPartialKept)                                                                           \
    /* Game list */                                                                                \
    X(HintOpen)                                                                                    \
    X(GameListPrompt)                                                                              \
    X(HintQuit)                                                                                    \
    X(HintBack)                                                                                    \
    X(HintExport)                                                                                  \
    X(HintDiscard)                                                                                 \
    X(HintEdit)                                                                                    \
    X(HintChange)                                                                                  \
    X(SaveCountNone)                                                                               \
    X(SaveCountOne)                                                                                \
    X(SaveCountMany)                                                                               \
    /* Save list */                                                                                \
    X(SaveListTitle)                                                                               \
    X(NoSavesFound)                                                                                \
    X(NoSavesHint)                                                                                 \
    /* Game menu */                                                                                \
    X(TrainerSummary)                                                                              \
    X(MenuStorage)                                                                                 \
    X(MenuStorageHint)                                                                             \
    X(MenuPokemonEditor)                                                                           \
    X(MenuPokemonEditorHint)                                                                       \
    X(MenuEvents)                                                                                  \
    X(MenuEventsHint)                                                                              \
    X(MenuOther)                                                                                   \
    X(MenuOtherHint)                                                                               \
    X(MenuBag)                                                                                     \
    X(MenuBagHint)                                                                                 \
    X(MenuTrainerEditor)                                                                           \
    X(MenuTrainerEditorHint)                                                                       \
    X(ComingSoon)                                                                                  \
    X(ChooseAction)                                                                                \
    X(NoBoxes)                                                                                     \
    X(OptionUnavailable)                                                                           \
    /* Boxes and party */                                                                          \
    X(PokemonEditorHeader)                                                                         \
    X(BoxSection)                                                                                  \
    X(PartySection)                                                                                \
    X(BoxSlot)                                                                                     \
    X(PartySlot)                                                                                   \
    X(SlotReadFailed)                                                                              \
    X(PokemonReadFailed)                                                                           \
    X(SelectedPokemon)                                                                             \
    X(OpenEditorHint)                                                                              \
    X(SlotsControls)                                                                               \
    /* Pokémon editor */                                                                          \
    X(PokemonSummary)                                                                              \
    X(PokemonInBox)                                                                                \
    X(PokemonInParty)                                                                              \
    X(PokemonSummaryTitle)                                                                         \
    X(PokemonEditorControls)                                                                       \
    X(FieldSpecies)                                                                                \
    X(FieldNickname)                                                                               \
    X(FieldLevel)                                                                                  \
    X(FieldNature)                                                                                 \
    X(FieldAbility)                                                                                \
    X(AbilityValue)                                                                                \
    X(AbilityChoice)                                                                               \
    X(FieldItem)                                                                                   \
    X(FieldShiny)                                                                                  \
    X(FieldPokerus)                                                                                \
    X(FieldOriginalTrainer)                                                                        \
    X(FieldGender)                                                                                 \
    X(GenderMale)                                                                                  \
    X(GenderFemale)                                                                                \
    X(GenderNone)                                                                                  \
    X(GenderUnknown)                                                                               \
    X(FieldFriendship)                                                                             \
    X(FriendshipTitle)                                                                             \
    X(FieldBall)                                                                                   \
    X(FieldForm)                                                                                   \
    X(FieldEgg)                                                                                    \
    X(FieldFatefulEncounter)                                                                       \
    X(FieldMetLevel)                                                                               \
    X(FieldMetDay)                                                                                 \
    X(FieldMetMonth)                                                                               \
    X(FieldMetYear)                                                                                \
    X(FieldMetLocation)                                                                            \
    X(FieldEggDay)                                                                                 \
    X(FieldEggMonth)                                                                               \
    X(FieldEggYear)                                                                                \
    X(FieldEggLocation)                                                                            \
    X(LocationValue)                                                                               \
    X(FieldEnjoyment)                                                                              \
    X(FieldFullness)                                                                               \
    X(FieldIv)                                                                                     \
    X(FieldEv)                                                                                     \
    X(StatHp)                                                                                      \
    X(StatAttack)                                                                                  \
    X(StatDefense)                                                                                 \
    X(StatSpAttack)                                                                                \
    X(StatSpDefense)                                                                               \
    X(StatSpeed)                                                                                   \
    X(FieldMove)                                                                                   \
    X(FieldRelearnMove)                                                                            \
    X(MoveTitle)                                                                                   \
    X(RelearnUnavailable)                                                                          \
    X(January)                                                                                     \
    X(February)                                                                                    \
    X(March)                                                                                       \
    X(April)                                                                                       \
    X(May)                                                                                         \
    X(June)                                                                                        \
    X(July)                                                                                        \
    X(August)                                                                                      \
    X(September)                                                                                   \
    X(October)                                                                                     \
    X(November)                                                                                    \
    X(December)                                                                                    \
    /* Trainer editor */                                                                           \
    X(TrainerName)                                                                                 \
    X(TrainerNameTitle)                                                                            \
    X(TrainerTid)                                                                                  \
    X(TrainerSid)                                                                                  \
    X(TrainerMoney)                                                                                \
    X(TrainerBattlePoints)                                                                         \
    X(TrainerPlayTime)                                                                             \
    X(TrainerPlayTimeTitle)                                                                        \
    X(MoneyValue)                                                                                  \
    X(ThousandsSeparator)                                                                          \
    X(InvalidPlayTime)                                                                             \
    X(TrainerGender)                                                                               \
    X(TrainerMale)                                                                                 \
    X(TrainerFemale)                                                                               \
    X(TrainerLanguage)                                                                             \
    X(LanguageJapanese)                                                                            \
    X(LanguageEnglish)                                                                             \
    X(LanguageFrench)                                                                              \
    X(LanguageItalian)                                                                             \
    X(LanguageGerman)                                                                              \
    X(LanguageSpanish)                                                                             \
    X(LanguageKorean)                                                                              \
    X(LanguageChineseSimplified)                                                                   \
    X(LanguageChineseTraditional)                                                                  \
    X(LanguageUnknown)                                                                             \
    /* Pickers and keyboard */                                                                     \
    X(ChooseOption)                                                                                \
    X(OptionPickerControls)                                                                        \
    X(NaturePickerHint)                                                                            \
    X(NaturePickerControls)                                                                        \
    X(NatureNeutral)                                                                               \
    X(EnterNumber)                                                                                 \
    X(NumberOutOfRange)

namespace pksm::switchui
{
    enum class Text
    {
#define PKSM_SWITCH_TEXT_KEY(key) key,
        PKSM_SWITCH_TEXTS(PKSM_SWITCH_TEXT_KEY)
#undef PKSM_SWITCH_TEXT_KEY
            Count
    };

    // The languages the app is shown in. English is the default.
    enum class TextLanguage
    {
        English,
        Spanish
    };

    // Where the app finds its language files.
    inline constexpr std::string_view TEXT_FOLDER = "romfs:/strings";

    // The file name of `language` without ".json", such as "en".
    const char* textLanguageCode(TextLanguage language);

    // The name of `key` in the language files, such as "Yes".
    const char* textKey(Text key);

    // Loads the texts of `language` from `folder`/<code>.json for every later tr() call. Any
    // text that file lacks, or that inserts a different number of values, comes from English;
    // without English either, tr() returns the key's name. Returns false when a file could not
    // be read or is not valid JSON.
    bool loadTexts(TextLanguage language, std::string_view folder = TEXT_FOLDER);
    TextLanguage textLanguage();

    // The text for `key` in the loaded language.
    const char* tr(Text key);

    // The text with each "{}" replaced by the next of `values`.
    std::string tr(Text key, std::initializer_list<std::string_view> values);
}
