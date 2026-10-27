#pragma once

#include <initializer_list>
#include <string>
#include <string_view>

// Every text the Switch app shows, with its key. To translate the app, add a language column
// here and pick it in Text.cpp. A "{}" marks where tr() inserts a value, in order.
#define PKSM_SWITCH_TEXTS(X)                                                                       \
    /* Shared */                                                                                   \
    X(Yes, "Sí")                                                                                   \
    X(No, "No")                                                                                    \
    X(Empty, "Vacío")                                                                              \
    X(NotAvailable, "No disponible")                                                               \
    X(WithId, "{} (#{})")                                                                          \
    X(PendingChanges, "Cambios pendientes de exportar")                                            \
    X(ExportedTo, "Exportado a {}")                                                                \
    X(UnsavedBeforeQuit, "Exporta con Y o descarta con X antes de salir.")                         \
    X(UnsavedBeforeBack, "Exporta con Y o descarta con X antes de volver.")                        \
    /* Games */                                                                                    \
    X(GameSword, "Pokemon Espada")                                                                 \
    X(GameShield, "Pokemon Escudo")                                                                \
    X(GameFireRed, "Pokemon Rojo Fuego")                                                           \
    X(GameLeafGreen, "Pokemon Verde Hoja")                                                         \
    /* Start-up */                                                                                 \
    X(SdlInitFailed, "No se pudo iniciar SDL: {}")                                                 \
    X(WindowFailed, "No se pudo crear la ventana: {}")                                             \
    X(RendererFailed, "No se pudo crear el renderizador: {}")                                      \
    X(ImGuiPlatformFailed, "No se pudo iniciar el backend SDL2 de Dear ImGui.")                    \
    X(ImGuiRendererFailed, "No se pudo iniciar el backend de renderizado de Dear ImGui.")          \
    X(PokemonSheet, "Pokémon")                                                                     \
    X(BallSheet, "Poké Ball")                                                                      \
    X(BallSheetIncomplete, "El spritesheet de Poké Ball no contiene todos los iconos.")            \
    X(SheetOpenFailed, "No se pudo abrir el spritesheet de {}.")                                   \
    X(SheetIncomplete, "El spritesheet de {} está incompleto.")                                    \
    X(SheetBadHeader, "El spritesheet de {} no tiene un encabezado válido.")                       \
    X(SheetUnsupported, "El formato del spritesheet de {} no es compatible.")                      \
    X(SheetTableIncomplete, "La tabla de sprites de {} está incompleta.")                          \
    X(SheetDecompressFailed, "No se pudo descomprimir el spritesheet de {}.")                      \
    X(SheetBadCoordinates, "La tabla de sprites de {} contiene coordenadas inválidas.")            \
    X(TextureCreateFailed, "No se pudo crear la textura de {}: {}")                                \
    X(TextureLoadFailed, "No se pudo cargar la textura de {}: {}")                                 \
    /* SD card and files */                                                                        \
    X(SdAccessFailed, "No se pudo acceder a la SD: {}")                                            \
    X(SaveDirectoryFailed, "No se pudo preparar la carpeta de respaldos: {}")                      \
    X(DirectoryTooDeep, "La carpeta de respaldos supera la profundidad maxima de 32 niveles.")     \
    X(DirectoryReadFailed, "No se pudo leer una carpeta de respaldos: {}")                         \
    X(DirectoryListFailed, "Error al enumerar una carpeta de respaldos: {}")                       \
    X(DirectoryCloseFailed, "No se pudo cerrar una carpeta de respaldos: {}")                      \
    X(FileInspectFailed, "No se pudo inspeccionar un archivo de respaldo: {}")                     \
    X(FileOpenFailed, "No se pudo abrir el archivo.")                                              \
    X(FileSizeFailed, "No se pudo leer el tamano del archivo.")                                    \
    X(FileSizeInvalid, "Tamano de archivo vacio, invalido o superior a 16 MiB.")                   \
    X(FileReadIncomplete, "Lectura incompleta del archivo.")                                       \
    X(SaveParseFailed, "No se pudo interpretar el guardado: {}")                                   \
    X(SaveUnsupported,                                                                             \
        "El archivo no es un guardado compatible de Sword/Shield o Rojo Fuego/Verde Hoja.")        \
    X(ExportNoFreeName, "No se encontro un nombre libre para exportar.")                           \
    X(ExportCreateFailed, "No se pudo crear la salida: {}")                                        \
    X(ExportOpenFailed, "No se pudo abrir la salida: {}")                                          \
    X(ExportEmptyNotRemoved, "; tampoco se pudo eliminar el archivo vacio: {}")                    \
    X(ExportPartialRemoved, "Escritura incompleta; se elimino la salida parcial.")                 \
    X(ExportPartialKept, "Escritura incompleta y no se pudo eliminar la salida parcial: {}")       \
    /* Save list */                                                                                \
    X(SaveListTitle, "Administrador de respaldos Pokémon")                                         \
    X(SaveListControls, "A: abrir   |   +: salir")                                                 \
    X(NoSavesFound, "No se encontraron respaldos compatibles.")                                    \
    X(NoSavesHint, "Copia tus archivos a /switch/pksm/ en la tarjeta SD.")                         \
    X(SavesFound, "{} respaldos encontrados")                                                      \
    X(OriginalsUntouched, "Los guardados originales no se modifican hasta guardar los cambios.")   \
    /* Game menu */                                                                                \
    X(TrainerSummary, "Entrenador: {}    TID/SID: {}/{}")                                          \
    X(MenuStorage, "Almacenamiento")                                                               \
    X(MenuStorageHint, "Gestiona tus cajas y Pokémon")                                             \
    X(MenuPokemonEditor, "Editor Pokémon")                                                         \
    X(MenuPokemonEditorHint, "Edita los datos de tus Pokémon")                                     \
    X(MenuEvents, "Eventos")                                                                       \
    X(MenuEventsHint, "Gestiona regalos misteriosos")                                              \
    X(MenuScripts, "Scripts")                                                                      \
    X(MenuScriptsHint, "Ejecuta scripts para el guardado")                                         \
    X(MenuBag, "Bolsa")                                                                            \
    X(MenuBagHint, "Edita los objetos de la bolsa")                                                \
    X(MenuTrainerEditor, "Editor entrenador")                                                      \
    X(MenuTrainerEditorHint, "Edita los datos del entrenador")                                     \
    X(ComingSoon, "Próximamente")                                                                  \
    X(GameMenuControls, "A: abrir    B: volver    Y: exportar    X: descartar    +: salir")        \
    X(NoBoxes, "Este guardado no contiene cajas de Pokémon.")                                      \
    X(OptionUnavailable, "Esta opción todavía no está disponible.")                                \
    /* Boxes and party */                                                                          \
    X(PokemonEditorHeader, "Editor de Pokémon  /  {}  /  {}  /  {}")                               \
    X(BoxSection, "Caja {}/{}  /  {}")                                                             \
    X(PartySection, "Equipo ({}/6)")                                                               \
    X(BoxSlot, "Espacio {}")                                                                       \
    X(PartySlot, "Equipo {}")                                                                      \
    X(SlotReadFailed, "Error al leer")                                                             \
    X(PokemonReadFailed, "No se pudo leer el Pokémon seleccionado.")                               \
    X(SelectedPokemon, "Pokémon seleccionado")                                                     \
    X(OpenEditorHint, "A: abrir editor")                                                           \
    X(SlotsControls,                                                                               \
        "D-pad: mover    L/R: cajas/equipo    A: editar    Y: exportar    B: volver")              \
    /* Pokémon editor */                                                                           \
    X(PokemonSummary, "{}  Nv. {}")                                                                \
    X(PokemonInBox, "{}  /  Caja {}  /  {}")                                                       \
    X(PokemonInParty, "{}  /  Equipo  /  {}")                                                      \
    X(PokemonSummaryTitle, "Resumen del Pokémon")                                                  \
    X(PokemonEditorControls,                                                                       \
        "D-pad: mover/cambiar    A: editar    Y: exportar    B: volver    X: descartar")           \
    X(FieldSpecies, "Especie")                                                                     \
    X(FieldNickname, "Apodo")                                                                      \
    X(FieldLevel, "Nivel")                                                                         \
    X(FieldNature, "Naturaleza")                                                                   \
    X(FieldAbility, "Habilidad")                                                                   \
    X(AbilityValue, "{} [{}]")                                                                     \
    X(AbilityChoice, "{} [Ranura {}]")                                                             \
    X(FieldItem, "Objeto")                                                                         \
    X(FieldShiny, "Variocolor")                                                                    \
    X(FieldPokerus, "Pokérus")                                                                     \
    X(FieldOriginalTrainer, "Entrenador original")                                                 \
    X(FieldGender, "Género")                                                                       \
    X(GenderMale, "Macho")                                                                         \
    X(GenderFemale, "Hembra")                                                                      \
    X(GenderNone, "Sin genero")                                                                    \
    X(GenderUnknown, "Desconocido")                                                                \
    X(FieldFriendship, "Amistad")                                                                  \
    X(FriendshipTitle, "Amistad (0-255)")                                                          \
    X(FieldBall, "Poké Ball")                                                                      \
    X(FieldForm, "Forma")                                                                          \
    X(FieldEgg, "Huevo")                                                                           \
    X(FieldFatefulEncounter, "Encuentro fatídico")                                                 \
    X(FieldMetLevel, "Nivel de encuentro")                                                         \
    X(FieldMetDay, "Día de encuentro")                                                             \
    X(FieldMetMonth, "Mes de encuentro")                                                           \
    X(FieldMetYear, "Año de encuentro")                                                            \
    X(FieldMetLocation, "Lugar de encuentro")                                                      \
    X(FieldEggDay, "Día de obtención del huevo")                                                   \
    X(FieldEggMonth, "Mes de obtención del huevo")                                                 \
    X(FieldEggYear, "Año de obtención del huevo")                                                  \
    X(FieldEggLocation, "Lugar de obtención del huevo")                                            \
    X(LocationValue, "#{}")                                                                        \
    X(FieldEnjoyment, "Satisfacción")                                                              \
    X(FieldFullness, "Saciedad")                                                                   \
    X(FieldIv, "IV {}")                                                                            \
    X(FieldEv, "EV {}")                                                                            \
    X(StatHp, "PS")                                                                                \
    X(StatAttack, "Ataque")                                                                        \
    X(StatDefense, "Defensa")                                                                      \
    X(StatSpAttack, "Ataque Esp.")                                                                 \
    X(StatSpDefense, "Defensa Esp.")                                                               \
    X(StatSpeed, "Velocidad")                                                                      \
    X(FieldMove, "Movimiento {}")                                                                  \
    X(FieldRelearnMove, "Movimiento recuerdo {}")                                                  \
    X(MoveTitle, "Movimiento")                                                                     \
    X(RelearnUnavailable, "No disponible en esta generación")                                      \
    X(January, "Enero")                                                                            \
    X(February, "Febrero")                                                                         \
    X(March, "Marzo")                                                                              \
    X(April, "Abril")                                                                              \
    X(May, "Mayo")                                                                                 \
    X(June, "Junio")                                                                               \
    X(July, "Julio")                                                                               \
    X(August, "Agosto")                                                                            \
    X(September, "Septiembre")                                                                     \
    X(October, "Octubre")                                                                          \
    X(November, "Noviembre")                                                                       \
    X(December, "Diciembre")                                                                       \
    /* Trainer editor */                                                                           \
    X(TrainerHeader, "Ficha del entrenador  /  {}  /  {}")                                         \
    X(TrainerControls,                                                                             \
        "D-pad: mover/cambiar    A: nombre    Y: exportar    B: menú    X: descartar")             \
    X(TrainerName, "Nombre")                                                                       \
    X(TrainerNameTitle, "Nombre del entrenador")                                                   \
    X(TrainerTid, "TID")                                                                           \
    X(TrainerSid, "SID")                                                                           \
    X(TrainerMoney, "Dinero")                                                                      \
    X(TrainerBattlePoints, "BP")                                                                   \
    X(TrainerHours, "Horas jugadas")                                                               \
    X(TrainerMinutes, "Minutos jugados")                                                           \
    X(TrainerSeconds, "Segundos jugados")                                                          \
    /* Pickers and keyboard */                                                                     \
    X(ChooseOption, "Selecciona una opción")                                                       \
    X(OptionPickerControls,                                                                        \
        "Flechas: mover    L/R: avanzar 100 opciones    A: seleccionar    B: volver")              \
    X(NaturePickerHint, "Selecciona una naturaleza según sus estadísticas")                        \
    X(NaturePickerControls, "Flechas: mover    A: seleccionar    B: volver")                       \
    X(NatureNeutral, "Neutra")                                                                     \
    X(LowerAttack, "-Ataque")                                                                      \
    X(LowerDefense, "-Defensa")                                                                    \
    X(LowerSpeed, "-Velocidad")                                                                    \
    X(LowerSpAttack, "-At. Esp.")                                                                  \
    X(LowerSpDefense, "-Def. Esp.")                                                                \
    X(RaiseAttack, "+Ataque")                                                                      \
    X(RaiseDefense, "+Defensa")                                                                    \
    X(RaiseSpeed, "+Velocidad")                                                                    \
    X(RaiseSpAttack, "+At. Esp.")                                                                  \
    X(RaiseSpDefense, "+Def. Esp.")                                                                \
    X(EnterNumber, "Introduce un número.")                                                         \
    X(NumberOutOfRange, "El número está fuera del rango permitido.")

namespace pksm::switchui
{
    enum class Text
    {
#define PKSM_SWITCH_TEXT_KEY(key, spanish) key,
        PKSM_SWITCH_TEXTS(PKSM_SWITCH_TEXT_KEY)
#undef PKSM_SWITCH_TEXT_KEY
            Count
    };

    // The text for `key` in the app's language.
    const char* tr(Text key);

    // The text with each "{}" replaced by the next of `values`.
    std::string tr(Text key, std::initializer_list<std::string_view> values);
}
