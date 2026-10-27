#pragma once

#include <initializer_list>
#include <string>
#include <string_view>

// Every text the Switch app shows: its key, then English and Spanish. To add a language, add a
// column here and to TextLanguage. A "{}" marks where tr() inserts a value, in order; every
// language must use the same number of them.
#define PKSM_SWITCH_TEXTS(X)                                                                        \
    /* Shared */                                                                                    \
    X(Yes, "Yes", "Sí")                                                                             \
    X(No, "No", "No")                                                                               \
    X(Empty, "Empty", "Vacío")                                                                      \
    X(NotAvailable, "Not available", "No disponible")                                               \
    X(WithId, "{} (#{})", "{} (#{})")                                                               \
    X(PendingChanges, "Changes not exported yet", "Cambios pendientes de exportar")                 \
    X(ExportedTo, "Exported to {}", "Exportado a {}")                                               \
    X(UnsavedBeforeQuit,                                                                            \
        "Export with Y or discard with X before quitting.",                                         \
        "Exporta con Y o descarta con X antes de salir.")                                           \
    X(UnsavedBeforeBack,                                                                            \
        "Export with Y or discard with X before going back.",                                       \
        "Exporta con Y o descarta con X antes de volver.")                                          \
    /* Games */                                                                                     \
    X(GameSword, "Pokémon Sword", "Pokemon Espada")                                                 \
    X(GameShield, "Pokémon Shield", "Pokemon Escudo")                                               \
    X(GameFireRed, "Pokémon FireRed", "Pokemon Rojo Fuego")                                         \
    X(GameLeafGreen, "Pokémon LeafGreen", "Pokemon Verde Hoja")                                     \
    /* Start-up */                                                                                  \
    X(SdlInitFailed, "Could not start SDL: {}", "No se pudo iniciar SDL: {}")                       \
    X(WindowFailed, "Could not create the window: {}", "No se pudo crear la ventana: {}")           \
    X(RendererFailed, "Could not create the renderer: {}", "No se pudo crear el renderizador: {}")  \
    X(ImGuiPlatformFailed,                                                                          \
        "Could not start the Dear ImGui SDL2 backend.",                                             \
        "No se pudo iniciar el backend SDL2 de Dear ImGui.")                                        \
    X(ImGuiRendererFailed,                                                                          \
        "Could not start the Dear ImGui rendering backend.",                                        \
        "No se pudo iniciar el backend de renderizado de Dear ImGui.")                              \
    X(PokemonSheet, "Pokémon", "Pokémon")                                                           \
    X(BallSheet, "Poké Ball", "Poké Ball")                                                          \
    X(BallSheetIncomplete,                                                                          \
        "The Poké Ball spritesheet is missing some icons.",                                         \
        "El spritesheet de Poké Ball no contiene todos los iconos.")                                \
    X(SheetOpenFailed,                                                                              \
        "Could not open the {} spritesheet.",                                                       \
        "No se pudo abrir el spritesheet de {}.")                                                   \
    X(SheetIncomplete,                                                                              \
        "The {} spritesheet is incomplete.",                                                        \
        "El spritesheet de {} está incompleto.")                                                    \
    X(SheetBadHeader,                                                                               \
        "The {} spritesheet has an invalid header.",                                                \
        "El spritesheet de {} no tiene un encabezado válido.")                                      \
    X(SheetUnsupported,                                                                             \
        "The {} spritesheet format is not supported.",                                              \
        "El formato del spritesheet de {} no es compatible.")                                       \
    X(SheetTableIncomplete,                                                                         \
        "The {} sprite table is incomplete.",                                                       \
        "La tabla de sprites de {} está incompleta.")                                               \
    X(SheetDecompressFailed,                                                                        \
        "Could not decompress the {} spritesheet.",                                                 \
        "No se pudo descomprimir el spritesheet de {}.")                                            \
    X(SheetBadCoordinates,                                                                          \
        "The {} sprite table has invalid coordinates.",                                             \
        "La tabla de sprites de {} contiene coordenadas inválidas.")                                \
    X(TextureCreateFailed,                                                                          \
        "Could not create the {} texture: {}",                                                      \
        "No se pudo crear la textura de {}: {}")                                                    \
    X(TextureLoadFailed,                                                                            \
        "Could not load the {} texture: {}",                                                        \
        "No se pudo cargar la textura de {}: {}")                                                   \
    /* SD card and files */                                                                         \
    X(SdAccessFailed, "Could not access the SD card: {}", "No se pudo acceder a la SD: {}")         \
    X(SaveDirectoryFailed,                                                                          \
        "Could not prepare the backup folder: {}",                                                  \
        "No se pudo preparar la carpeta de respaldos: {}")                                          \
    X(DirectoryTooDeep,                                                                             \
        "The backup folder is nested deeper than 32 levels.",                                       \
        "La carpeta de respaldos supera la profundidad maxima de 32 niveles.")                      \
    X(DirectoryReadFailed,                                                                          \
        "Could not read a backup folder: {}",                                                       \
        "No se pudo leer una carpeta de respaldos: {}")                                             \
    X(DirectoryListFailed,                                                                          \
        "Could not list a backup folder: {}",                                                       \
        "Error al enumerar una carpeta de respaldos: {}")                                           \
    X(DirectoryCloseFailed,                                                                         \
        "Could not close a backup folder: {}",                                                      \
        "No se pudo cerrar una carpeta de respaldos: {}")                                           \
    X(FileInspectFailed,                                                                            \
        "Could not inspect a backup file: {}",                                                      \
        "No se pudo inspeccionar un archivo de respaldo: {}")                                       \
    X(FileOpenFailed, "Could not open the file.", "No se pudo abrir el archivo.")                   \
    X(FileSizeFailed, "Could not read the file size.", "No se pudo leer el tamano del archivo.")    \
    X(FileSizeInvalid,                                                                              \
        "The file is empty, invalid or larger than 16 MiB.",                                        \
        "Tamano de archivo vacio, invalido o superior a 16 MiB.")                                   \
    X(FileReadIncomplete,                                                                           \
        "The file could not be read completely.",                                                   \
        "Lectura incompleta del archivo.")                                                          \
    X(SaveParseFailed, "Could not read the save: {}", "No se pudo interpretar el guardado: {}")     \
    X(SaveUnsupported,                                                                              \
        "The file is not a supported Sword/Shield or FireRed/LeafGreen save.",                      \
        "El archivo no es un guardado compatible de Sword/Shield o Rojo Fuego/Verde Hoja.")         \
    X(ExportNoFreeName,                                                                             \
        "No free file name was found for the export.",                                              \
        "No se encontro un nombre libre para exportar.")                                            \
    X(ExportCreateFailed, "Could not create the output file: {}", "No se pudo crear la salida: {}") \
    X(ExportOpenFailed, "Could not open the output file: {}", "No se pudo abrir la salida: {}")     \
    X(ExportEmptyNotRemoved,                                                                        \
        "; the empty file could not be removed either: {}",                                         \
        "; tampoco se pudo eliminar el archivo vacio: {}")                                          \
    X(ExportPartialRemoved,                                                                         \
        "The file was not fully written; the partial output was removed.",                          \
        "Escritura incompleta; se elimino la salida parcial.")                                      \
    X(ExportPartialKept,                                                                            \
        "The file was not fully written and the partial output could not be removed: {}",           \
        "Escritura incompleta y no se pudo eliminar la salida parcial: {}")                         \
    /* Save list */                                                                                 \
    X(SaveListTitle, "Pokémon backup manager", "Administrador de respaldos Pokémon")                \
    X(SaveListControls, "A: open   |   +: quit", "A: abrir   |   +: salir")                         \
    X(NoSavesFound, "No supported backups were found.", "No se encontraron respaldos compatibles.") \
    X(NoSavesHint,                                                                                  \
        "Copy your files to /switch/pksm/ on the SD card.",                                         \
        "Copia tus archivos a /switch/pksm/ en la tarjeta SD.")                                     \
    X(SavesFound, "{} backups found", "{} respaldos encontrados")                                   \
    X(OriginalsUntouched,                                                                           \
        "Your original saves are not modified until you export the changes.",                       \
        "Los guardados originales no se modifican hasta guardar los cambios.")                      \
    /* Game menu */                                                                                 \
    X(TrainerSummary, "Trainer: {}    TID/SID: {}/{}", "Entrenador: {}    TID/SID: {}/{}")          \
    X(MenuStorage, "Storage", "Almacenamiento")                                                     \
    X(MenuStorageHint, "Manage your boxes and Pokémon", "Gestiona tus cajas y Pokémon")             \
    X(MenuPokemonEditor, "Pokémon editor", "Editor Pokémon")                                        \
    X(MenuPokemonEditorHint, "Edit your Pokémon's data", "Edita los datos de tus Pokémon")          \
    X(MenuEvents, "Events", "Eventos")                                                              \
    X(MenuEventsHint, "Manage Mystery Gifts", "Gestiona regalos misteriosos")                       \
    X(MenuScripts, "Scripts", "Scripts")                                                            \
    X(MenuScriptsHint, "Run scripts on the save", "Ejecuta scripts para el guardado")               \
    X(MenuBag, "Bag", "Bolsa")                                                                      \
    X(MenuBagHint, "Edit the items in the bag", "Edita los objetos de la bolsa")                    \
    X(MenuTrainerEditor, "Trainer editor", "Editor entrenador")                                     \
    X(MenuTrainerEditorHint, "Edit the trainer's data", "Edita los datos del entrenador")           \
    X(ComingSoon, "Coming soon", "Próximamente")                                                    \
    X(GameMenuControls,                                                                             \
        "A: open    B: back    Y: export    X: discard    +: quit",                                 \
        "A: abrir    B: volver    Y: exportar    X: descartar    +: salir")                         \
    X(NoBoxes, "This save has no Pokémon boxes.", "Este guardado no contiene cajas de Pokémon.")    \
    X(OptionUnavailable,                                                                            \
        "This option is not available yet.",                                                        \
        "Esta opción todavía no está disponible.")                                                  \
    /* Boxes and party */                                                                           \
    X(PokemonEditorHeader,                                                                          \
        "Pokémon editor  /  {}  /  {}  /  {}",                                                      \
        "Editor de Pokémon  /  {}  /  {}  /  {}")                                                   \
    X(BoxSection, "Box {}/{}  /  {}", "Caja {}/{}  /  {}")                                          \
    X(PartySection, "Party ({}/6)", "Equipo ({}/6)")                                                \
    X(BoxSlot, "Slot {}", "Espacio {}")                                                             \
    X(PartySlot, "Party {}", "Equipo {}")                                                           \
    X(SlotReadFailed, "Could not read", "Error al leer")                                            \
    X(PokemonReadFailed,                                                                            \
        "Could not read the selected Pokémon.",                                                     \
        "No se pudo leer el Pokémon seleccionado.")                                                 \
    X(SelectedPokemon, "Selected Pokémon", "Pokémon seleccionado")                                  \
    X(OpenEditorHint, "A: open editor", "A: abrir editor")                                          \
    X(SlotsControls,                                                                                \
        "D-pad: move    L/R: boxes/party    A: edit    Y: export    B: back",                       \
        "D-pad: mover    L/R: cajas/equipo    A: editar    Y: exportar    B: volver")               \
    /* Pokémon editor */                                                                            \
    X(PokemonSummary, "{}  Lv. {}", "{}  Nv. {}")                                                   \
    X(PokemonInBox, "{}  /  Box {}  /  {}", "{}  /  Caja {}  /  {}")                                \
    X(PokemonInParty, "{}  /  Party  /  {}", "{}  /  Equipo  /  {}")                                \
    X(PokemonSummaryTitle, "Pokémon summary", "Resumen del Pokémon")                                \
    X(PokemonEditorControls,                                                                        \
        "D-pad: move/change    A: edit    Y: export    B: back    X: discard",                      \
        "D-pad: mover/cambiar    A: editar    Y: exportar    B: volver    X: descartar")            \
    X(FieldSpecies, "Species", "Especie")                                                           \
    X(FieldNickname, "Nickname", "Apodo")                                                           \
    X(FieldLevel, "Level", "Nivel")                                                                 \
    X(FieldNature, "Nature", "Naturaleza")                                                          \
    X(FieldAbility, "Ability", "Habilidad")                                                         \
    X(AbilityValue, "{} [{}]", "{} [{}]")                                                           \
    X(AbilityChoice, "{} [Slot {}]", "{} [Ranura {}]")                                              \
    X(FieldItem, "Held item", "Objeto")                                                             \
    X(FieldShiny, "Shiny", "Variocolor")                                                            \
    X(FieldPokerus, "Pokérus", "Pokérus")                                                           \
    X(FieldOriginalTrainer, "Original trainer", "Entrenador original")                              \
    X(FieldGender, "Gender", "Género")                                                              \
    X(GenderMale, "Male", "Macho")                                                                  \
    X(GenderFemale, "Female", "Hembra")                                                             \
    X(GenderNone, "Genderless", "Sin genero")                                                       \
    X(GenderUnknown, "Unknown", "Desconocido")                                                      \
    X(FieldFriendship, "Friendship", "Amistad")                                                     \
    X(FriendshipTitle, "Friendship (0-255)", "Amistad (0-255)")                                     \
    X(FieldBall, "Poké Ball", "Poké Ball")                                                          \
    X(FieldForm, "Form", "Forma")                                                                   \
    X(FieldEgg, "Egg", "Huevo")                                                                     \
    X(FieldFatefulEncounter, "Fateful encounter", "Encuentro fatídico")                             \
    X(FieldMetLevel, "Met level", "Nivel de encuentro")                                             \
    X(FieldMetDay, "Met day", "Día de encuentro")                                                   \
    X(FieldMetMonth, "Met month", "Mes de encuentro")                                               \
    X(FieldMetYear, "Met year", "Año de encuentro")                                                 \
    X(FieldMetLocation, "Met location", "Lugar de encuentro")                                       \
    X(FieldEggDay, "Egg received day", "Día de obtención del huevo")                                \
    X(FieldEggMonth, "Egg received month", "Mes de obtención del huevo")                            \
    X(FieldEggYear, "Egg received year", "Año de obtención del huevo")                              \
    X(FieldEggLocation, "Egg received location", "Lugar de obtención del huevo")                    \
    X(LocationValue, "#{}", "#{}")                                                                  \
    X(FieldEnjoyment, "Enjoyment", "Satisfacción")                                                  \
    X(FieldFullness, "Fullness", "Saciedad")                                                        \
    X(FieldIv, "{} IV", "IV {}")                                                                    \
    X(FieldEv, "{} EV", "EV {}")                                                                    \
    X(StatHp, "HP", "PS")                                                                           \
    X(StatAttack, "Attack", "Ataque")                                                               \
    X(StatDefense, "Defense", "Defensa")                                                            \
    X(StatSpAttack, "Sp. Atk", "Ataque Esp.")                                                       \
    X(StatSpDefense, "Sp. Def", "Defensa Esp.")                                                     \
    X(StatSpeed, "Speed", "Velocidad")                                                              \
    X(FieldMove, "Move {}", "Movimiento {}")                                                        \
    X(FieldRelearnMove, "Relearn move {}", "Movimiento recuerdo {}")                                \
    X(MoveTitle, "Move", "Movimiento")                                                              \
    X(RelearnUnavailable, "Not available in this generation", "No disponible en esta generación")   \
    X(January, "January", "Enero")                                                                  \
    X(February, "February", "Febrero")                                                              \
    X(March, "March", "Marzo")                                                                      \
    X(April, "April", "Abril")                                                                      \
    X(May, "May", "Mayo")                                                                           \
    X(June, "June", "Junio")                                                                        \
    X(July, "July", "Julio")                                                                        \
    X(August, "August", "Agosto")                                                                   \
    X(September, "September", "Septiembre")                                                         \
    X(October, "October", "Octubre")                                                                \
    X(November, "November", "Noviembre")                                                            \
    X(December, "December", "Diciembre")                                                            \
    /* Trainer editor */                                                                            \
    X(TrainerHeader, "Trainer card  /  {}  /  {}", "Ficha del entrenador  /  {}  /  {}")            \
    X(TrainerControls,                                                                              \
        "D-pad: move/change    A: name    Y: export    B: menu    X: discard",                      \
        "D-pad: mover/cambiar    A: nombre    Y: exportar    B: menú    X: descartar")              \
    X(TrainerName, "Name", "Nombre")                                                                \
    X(TrainerNameTitle, "Trainer name", "Nombre del entrenador")                                    \
    X(TrainerTid, "TID", "TID")                                                                     \
    X(TrainerSid, "SID", "SID")                                                                     \
    X(TrainerMoney, "Money", "Dinero")                                                              \
    X(TrainerBattlePoints, "BP", "BP")                                                              \
    X(TrainerHours, "Hours played", "Horas jugadas")                                                \
    X(TrainerMinutes, "Minutes played", "Minutos jugados")                                          \
    X(TrainerSeconds, "Seconds played", "Segundos jugados")                                         \
    /* Pickers and keyboard */                                                                      \
    X(ChooseOption, "Choose an option", "Selecciona una opción")                                    \
    X(OptionPickerControls,                                                                         \
        "Arrows: move    L/R: jump 100 options    A: select    B: back",                            \
        "Flechas: mover    L/R: avanzar 100 opciones    A: seleccionar    B: volver")               \
    X(NaturePickerHint,                                                                             \
        "Choose a nature by the stats it changes",                                                  \
        "Selecciona una naturaleza según sus estadísticas")                                         \
    X(NaturePickerControls,                                                                         \
        "Arrows: move    A: select    B: back",                                                     \
        "Flechas: mover    A: seleccionar    B: volver")                                            \
    X(NatureNeutral, "Neutral", "Neutra")                                                           \
    X(LowerAttack, "-Attack", "-Ataque")                                                            \
    X(LowerDefense, "-Defense", "-Defensa")                                                         \
    X(LowerSpeed, "-Speed", "-Velocidad")                                                           \
    X(LowerSpAttack, "-Sp. Atk", "-At. Esp.")                                                       \
    X(LowerSpDefense, "-Sp. Def", "-Def. Esp.")                                                     \
    X(RaiseAttack, "+Attack", "+Ataque")                                                            \
    X(RaiseDefense, "+Defense", "+Defensa")                                                         \
    X(RaiseSpeed, "+Speed", "+Velocidad")                                                           \
    X(RaiseSpAttack, "+Sp. Atk", "+At. Esp.")                                                       \
    X(RaiseSpDefense, "+Sp. Def", "+Def. Esp.")                                                     \
    X(EnterNumber, "Enter a number.", "Introduce un número.")                                       \
    X(NumberOutOfRange, "The number is out of range.", "El número está fuera del rango permitido.")

namespace pksm::switchui
{
    enum class Text
    {
#define PKSM_SWITCH_TEXT_KEY(key, english, spanish) key,
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

    // Chooses the language of every later tr() call.
    void setTextLanguage(TextLanguage language);
    TextLanguage textLanguage();

    // The text for `key` in the app's language.
    const char* tr(Text key);

    // The text with each "{}" replaced by the next of `values`.
    std::string tr(Text key, std::initializer_list<std::string_view> values);
}
