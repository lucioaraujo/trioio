cmake_minimum_required(VERSION 3.22)

# O padrão multiplataforma do RASGO (RASGO_DOCUMENTATION/
# PADRAO_DISTRIBUICAO_MULTIPLATAFORMA.md), desde o primeiro empacotamento
# (2026-10-18), antes de qualquer alvo:
# - Windows: o runtime do Visual C++ estático (/MT), dentro do .exe (o
#   Antitotem, o Rasgo Modular e o Navalha 2 já não abriam sem o
#   Redistributable);
# - macOS: um binário Universal 2 (x86_64 e arm64) e o alvo de implantação
#   explícito, antes do project(): sem ele, o binário herda o macOS do
#   runner de CI e recusa abrir num Mac mais antigo. 10.15: o C++20 da
#   libc++ da Apple que o Trioio usa (std::optional sem exceções novas,
#   std::thread) já está lá; o JUCE 9 pede 10.11.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "macOS mínimo" FORCE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Universal 2" FORCE)
endif()
# C também: o JUCE exige o compilador C habilitado no projeto que o inclui
project(Trioio VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(trioio_core
    src/core/TrioioBrain.cpp
    src/core/Provenance.cpp
    src/core/Descriptors.cpp
    src/core/Structure.cpp
    src/core/Strategy.cpp
    src/core/EnergySynth.cpp
    src/core/Composer.cpp
    src/core/ThirdEye.cpp
    src/core/ScoreSynth.cpp
    src/core/ModularVoice.cpp
    src/core/RasgoSynthVoice.cpp
    src/core/AntitotemVoice.cpp
    src/core/AquorbiumVoice.cpp
    src/core/AcousticVoice.cpp
    src/core/Ensemble.cpp
    src/core/StyleFigures.cpp
    src/core/FilmEffects.cpp
    src/core/ModularRasgo.cpp
    src/core/Production.cpp
    src/core/HarmonyWriting.cpp
    src/core/SomDaAura.cpp
    src/core/Intent.cpp
    src/core/Versions.cpp
    src/core/ShortMemory.cpp
    src/core/Influences.cpp
    src/core/Listening.cpp
    src/core/OracleMemory.cpp
    src/core/Constraints.cpp
    src/core/RhythmField.cpp
    src/core/Mosaic.cpp
    src/core/ScaleBank.cpp
    src/core/Harmony.cpp
    src/core/ScaleArchive.cpp
    src/core/ScaleMachine.cpp
    src/core/ScaleAffect.cpp
    src/core/ScaleIdiom.cpp
    src/core/Recombination.cpp
    src/core/RhythmCycles.cpp
    src/core/Mashup.cpp
    src/core/Voicing.cpp
    src/core/Reading.cpp
    src/core/BandSplit.cpp
    src/core/Lines.cpp
    src/core/Master.cpp
    src/core/Groove.cpp
    src/core/Dramaturgy.cpp
    src/core/Dialogue.cpp
    src/core/Game.cpp
    # cópia fixada do Aquorbium (fontes .cpp, compiladas aqui)
    third_party/aquorbium_dsp/core/BiomaModalResonator.cpp
    third_party/aquorbium_dsp/core/BiomaPhysicalString.cpp
    third_party/aquorbium_dsp/core/BiomaGranularStriker.cpp
)
target_include_directories(trioio_core PUBLIC src/core)
# a síntese dos barramentos em paralelo (ScoreSynth.cpp, std::thread)
find_package(Threads REQUIRED)
target_link_libraries(trioio_core PUBLIC Threads::Threads)

# ---- DSP aproveitado de outros instrumentos RASGO --------------------------
# O Trioio é autônomo (decisão do autor, 2026-10-07): o código aproveitado
# do Rasgo Modular e do Rasgo Synth é uma CÓPIA fixada em third_party/, com
# commit de origem e checksums em PROVENANCE.md. Nenhum caminho aponta para
# fora deste diretório. SYSTEM: os avisos desse código são tratados na
# origem, não aqui.
target_include_directories(trioio_core SYSTEM PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/rasgo_modular_dsp"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/rasgo_synth_dsp"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/antitotem_dsp"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/aquorbium_dsp")
# os .cpp copiados do Aquorbium: avisos tratados na origem, não aqui
set_source_files_properties(
    third_party/aquorbium_dsp/core/BiomaModalResonator.cpp
    third_party/aquorbium_dsp/core/BiomaPhysicalString.cpp
    third_party/aquorbium_dsp/core/BiomaGranularStriker.cpp
    PROPERTIES COMPILE_OPTIONS "$<IF:$<CXX_COMPILER_ID:MSVC>,/w,-w>")
target_compile_definitions(trioio_core PRIVATE TRIOIO_HAS_MODULAR=1)
target_compile_features(trioio_core PUBLIC cxx_std_20)

if (MSVC)
    target_compile_options(trioio_core PRIVATE /W4)
else()
    target_compile_options(trioio_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---- Biblioteca (JUCE) --------------------------------------------------
# O núcleo acima não depende de nada. A camada da Biblioteca usa o juce_core
# (JSON dos manifestos), sob AGPLv3 — ver docs/LICENSE_STATUS.md. O JUCE não
# é versionado aqui: é um checkout local, como nos outros instrumentos RASGO:
#   cmake -B build -DTRIOIO_JUCE_PATH=/caminho/para/JUCE
set(TRIOIO_JUCE_PATH "" CACHE PATH "Path to a JUCE source checkout")
if (TRIOIO_JUCE_PATH AND EXISTS "${TRIOIO_JUCE_PATH}/CMakeLists.txt")
    add_subdirectory("${TRIOIO_JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
    set(TRIOIO_HAS_JUCE ON)
    # fontes da Biblioteca, compiladas dentro de cada alvo JUCE que as usa
    # (módulos JUCE não devem ir para uma biblioteca estática compartilhada)
    set(TRIOIO_LIBRARY_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/library/ManifestJson.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/library/LibraryImporter.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/library/Declaration.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/library/Analysis.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/library/Response.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/library/ListeningStore.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/library/AudioSource.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/library/Retention.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/library/Prompts.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/library/DataFiles.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/library/SemanticBridge.cpp")
    set(TRIOIO_LIBRARY_MODULES juce::juce_core juce::juce_audio_formats juce::juce_cryptography)
    message(STATUS "Trioio: camada da Biblioteca habilitada (JUCE em ${TRIOIO_JUCE_PATH})")
else()
    set(TRIOIO_HAS_JUCE OFF)
    message(STATUS "Trioio: Biblioteca pulada — defina -DTRIOIO_JUCE_PATH=/caminho/para/JUCE")
endif()

if (TRIOIO_HAS_JUCE)
    # ferramenta de linha de comando do importador (docs/IMPORTADOR_DA_BIBLIOTECA.md)
    juce_add_console_app(trioio_biblioteca PRODUCT_NAME "trioio_biblioteca")
    target_sources(trioio_biblioteca PRIVATE src/tools/BibliotecaCli.cpp ${TRIOIO_LIBRARY_SOURCES})
    target_include_directories(trioio_biblioteca PRIVATE src/library)
    target_compile_definitions(trioio_biblioteca PRIVATE JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0 JUCE_USE_MP3AUDIOFORMAT=1
        TRIOIO_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        TRIOIO_VERSION="${PROJECT_VERSION}")
    target_link_libraries(trioio_biblioteca PRIVATE
        trioio_core ${TRIOIO_LIBRARY_MODULES} juce::juce_recommended_config_flags)
    if (NOT MSVC)
        target_compile_options(trioio_biblioteca PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

if (TRIOIO_HAS_JUCE)
    # o aplicativo (docs/design.md): a interface sobre a mesma Biblioteca
    # o ícone: a logo dos três olhos do autor (packaging/icon/, PROVENIENCIA.md);
    # o juceaide o embute no .exe e no .app, e o atalho do instalador o herda
    juce_add_gui_app(trioio_app PRODUCT_NAME "Trioio"
        COMPANY_NAME "RASGO"
        BUNDLE_ID "org.rasgo.trioio"
        ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon/trioio-1024.png"
        ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon/trioio-32.png")
    target_sources(trioio_app PRIVATE src/app/Main.cpp src/app/MainView.cpp src/app/AppModel.cpp src/app/WindowPolicy.cpp
        ${TRIOIO_LIBRARY_SOURCES})
    target_include_directories(trioio_app PRIVATE src/app src/library)
    target_compile_definitions(trioio_app PRIVATE JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0 JUCE_USE_MP3AUDIOFORMAT=1
        JUCE_APPLICATION_NAME_STRING="Trioio" JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"
        TRIOIO_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}" TRIOIO_VERSION="${PROJECT_VERSION}")
    target_link_libraries(trioio_app PRIVATE trioio_core ${TRIOIO_LIBRARY_MODULES}
        juce::juce_gui_basics juce::juce_audio_utils juce::juce_audio_devices juce::juce_recommended_config_flags)
    # os arquivos de dados (escalas, ritmos, cadências) junto do binário:
    # data/ ao lado do executável e, no .app do macOS, em Contents/Resources/data
    # (src/library/DataFiles.h). O código-fonte deixa de ser o único lugar.
    foreach(alvo trioio_app trioio_biblioteca)
        if (APPLE AND alvo STREQUAL "trioio_app")
            add_custom_command(TARGET ${alvo} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/data" "$<TARGET_BUNDLE_CONTENT_DIR:${alvo}>/Resources/data")
        else()
            add_custom_command(TARGET ${alvo} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/data" "$<TARGET_FILE_DIR:${alvo}>/data")
        endif()
    endforeach()
    # regra UTF-8 do RASGO (AGENTS.md §4): fonte e execução em UTF-8
    if (MSVC)
        target_compile_options(trioio_app PRIVATE /utf-8)
    else()
        target_compile_options(trioio_app PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8 -Wall -Wextra)
    endif()
endif()

# A instalação e os pacotes (o padrão multiplataforma do RASGO; os scripts
# Linux copiados do Antitotem, packaging/PROVENIENCIA.md). Os dados vão onde
# src/library/DataFiles.h os procura: ao lado do .exe (Windows), dentro do
# .app (macOS, já pelo POST_BUILD) e em ../share/trioio/data (Linux).
if (TRIOIO_HAS_JUCE)
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        install(PROGRAMS "$<TARGET_FILE:trioio_app>" DESTINATION bin RENAME trioio)
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data/" DESTINATION share/trioio/data)
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/trioio.desktop" DESTINATION share/applications)
        foreach(tam 16 24 32 48 64 128 256 512)
            install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon/trioio-${tam}.png"
                DESTINATION share/icons/hicolor/${tam}x${tam}/apps RENAME trioio.png)
        endforeach()
    elseif (WIN32)
        install(TARGETS trioio_app RUNTIME DESTINATION .)
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data" DESTINATION .)
    elseif (APPLE)
        install(TARGETS trioio_app BUNDLE DESTINATION .)
    endif()

    set(CPACK_PACKAGE_NAME "trioio")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_VENDOR "RASGO")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Trioio: listens to a recording and composes a synthesised response")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # o .deb (o AppImage e o .tar.gz saem de packaging/linux/empacotar.sh,
        # da mesma árvore instalada). As bibliotecas que o JUCE abre em tempo
        # de execução (dlopen) não aparecem no shlibdeps: declaradas à mão,
        # com as alternativas de nome das distribuições (lição do Antitotem)
        set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
        set(CPACK_GENERATOR "DEB")
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Lúcio Araújo <rasgo.instruments@gmail.com>")
        set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
        set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/lucioaraujo/trioio")
        set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "libx11-6, libxext6, libxss1, libglib2.0-0t64 | libglib2.0-0 | libgio-2.0-0")
    elseif (WIN32)
        # NSIS: o atalho do Menu Iniciar só existe com CPACK_PACKAGE_EXECUTABLES;
        # o .exe na raiz (EXECUTABLES_DIRECTORY "."), senão o atalho aponta para
        # bin\ e não abre; pasta e registro sem a versão, para a nova remover a
        # anterior (as três lições do Antitotem, do Navalha 2 e do Rasgo Modular)
        set(CPACK_GENERATOR "NSIS")
        set(CPACK_NSIS_PACKAGE_NAME "Trioio")
        set(CPACK_NSIS_DISPLAY_NAME "Trioio")
        set(CPACK_NSIS_CONTACT "rasgo.instruments@gmail.com")
        set(CPACK_NSIS_MODIFY_PATH OFF)
        set(CPACK_PACKAGE_INSTALL_DIRECTORY "Trioio")
        set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "Trioio")
        set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
        set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/lucioaraujo/trioio")
        set(CPACK_PACKAGE_EXECUTABLES "Trioio" "Trioio")
        set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
        set(CPACK_CREATE_DESKTOP_LINKS "Trioio")
        set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon/trioio.ico")
        set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon/trioio.ico")
        set(CPACK_NSIS_INSTALLED_ICON_NAME "Trioio.exe")
        set(CPACK_NSIS_MUI_FINISHPAGE_RUN "Trioio.exe")
    elseif (APPLE)
        set(CPACK_GENERATOR "DragNDrop")
        set(CPACK_DMG_VOLUME_NAME "Trioio")
    endif()
    include(CPack)
endif()

option(TRIOIO_BUILD_TESTS "Build Trioio core tests" ON)
if (TRIOIO_BUILD_TESTS)
    enable_testing()
    add_executable(trioio_brain_tests tests/TrioioBrainTests.cpp)
    target_link_libraries(trioio_brain_tests PRIVATE trioio_core)
    add_test(NAME trioio_brain_tests COMMAND trioio_brain_tests)

    add_executable(trioio_provenance_tests tests/ProvenanceTests.cpp)
    target_link_libraries(trioio_provenance_tests PRIVATE trioio_core)
    add_test(NAME trioio_provenance_tests COMMAND trioio_provenance_tests)

    add_executable(trioio_descriptors_tests tests/DescriptorsTests.cpp)
    target_link_libraries(trioio_descriptors_tests PRIVATE trioio_core)
    add_test(NAME trioio_descriptors_tests COMMAND trioio_descriptors_tests)

    add_executable(trioio_game_tests tests/GameTests.cpp)
    target_link_libraries(trioio_game_tests PRIVATE trioio_core)
    add_test(NAME trioio_game_tests COMMAND trioio_game_tests)

    add_executable(trioio_film_effects_tests tests/FilmEffectsTests.cpp)
    target_link_libraries(trioio_film_effects_tests PRIVATE trioio_core)
    add_test(NAME trioio_film_effects_tests COMMAND trioio_film_effects_tests)

    add_executable(trioio_modular_rasgo_tests tests/ModularRasgoTests.cpp)
    target_link_libraries(trioio_modular_rasgo_tests PRIVATE trioio_core)
    add_test(NAME trioio_modular_rasgo_tests COMMAND trioio_modular_rasgo_tests)

    add_executable(trioio_production_tests tests/ProductionTests.cpp)
    target_link_libraries(trioio_production_tests PRIVATE trioio_core)
    add_test(NAME trioio_production_tests COMMAND trioio_production_tests)

    add_executable(trioio_harmony_writing_tests tests/HarmonyWritingTests.cpp)
    target_link_libraries(trioio_harmony_writing_tests PRIVATE trioio_core)
    target_compile_definitions(trioio_harmony_writing_tests PRIVATE TRIOIO_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME trioio_harmony_writing_tests COMMAND trioio_harmony_writing_tests)
    add_executable(trioio_intent_tests tests/IntentTests.cpp)
    target_link_libraries(trioio_intent_tests PRIVATE trioio_core)
    add_test(NAME trioio_intent_tests COMMAND trioio_intent_tests)
    add_executable(trioio_versions_tests tests/VersionsTests.cpp)
    target_link_libraries(trioio_versions_tests PRIVATE trioio_core)
    add_test(NAME trioio_versions_tests COMMAND trioio_versions_tests)
    add_executable(trioio_short_memory_tests tests/ShortMemoryTests.cpp)
    target_link_libraries(trioio_short_memory_tests PRIVATE trioio_core)
    add_test(NAME trioio_short_memory_tests COMMAND trioio_short_memory_tests)
    add_executable(trioio_influences_tests tests/InfluencesTests.cpp)
    target_link_libraries(trioio_influences_tests PRIVATE trioio_core)
    target_compile_definitions(trioio_influences_tests PRIVATE TRIOIO_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME trioio_influences_tests COMMAND trioio_influences_tests)

    add_executable(trioio_ensemble_tests tests/EnsembleTests.cpp)
    target_link_libraries(trioio_ensemble_tests PRIVATE trioio_core)
    add_test(NAME trioio_ensemble_tests COMMAND trioio_ensemble_tests)

    add_executable(trioio_dialogue_tests tests/DialogueTests.cpp)
    target_link_libraries(trioio_dialogue_tests PRIVATE trioio_core)
    add_test(NAME trioio_dialogue_tests COMMAND trioio_dialogue_tests)

    add_executable(trioio_dramaturgy_tests tests/DramaturgyTests.cpp)
    target_link_libraries(trioio_dramaturgy_tests PRIVATE trioio_core)
    add_test(NAME trioio_dramaturgy_tests COMMAND trioio_dramaturgy_tests)

    add_executable(trioio_groove_tests tests/GrooveTests.cpp)
    target_link_libraries(trioio_groove_tests PRIVATE trioio_core)
    add_test(NAME trioio_groove_tests COMMAND trioio_groove_tests)

    add_executable(trioio_master_tests tests/MasterTests.cpp)
    target_link_libraries(trioio_master_tests PRIVATE trioio_core)
    add_test(NAME trioio_master_tests COMMAND trioio_master_tests)

    add_executable(trioio_band_split_tests tests/BandSplitTests.cpp)
    target_link_libraries(trioio_band_split_tests PRIVATE trioio_core)
    add_test(NAME trioio_band_split_tests COMMAND trioio_band_split_tests)

    add_executable(trioio_structure_tests tests/StructureTests.cpp)
    target_link_libraries(trioio_structure_tests PRIVATE trioio_core)
    add_test(NAME trioio_structure_tests COMMAND trioio_structure_tests)

    add_executable(trioio_composer_tests tests/ComposerTests.cpp)
    target_link_libraries(trioio_composer_tests PRIVATE trioio_core)
    add_test(NAME trioio_composer_tests COMMAND trioio_composer_tests)

    add_executable(trioio_third_eye_tests tests/ThirdEyeTests.cpp)
    target_link_libraries(trioio_third_eye_tests PRIVATE trioio_core)
    add_test(NAME trioio_third_eye_tests COMMAND trioio_third_eye_tests)

    add_executable(trioio_rhythm_field_tests tests/RhythmFieldTests.cpp)
    target_link_libraries(trioio_rhythm_field_tests PRIVATE trioio_core)
    add_test(NAME trioio_rhythm_field_tests COMMAND trioio_rhythm_field_tests)

    add_executable(trioio_constraints_tests tests/ConstraintsTests.cpp)
    target_link_libraries(trioio_constraints_tests PRIVATE trioio_core)
    add_test(NAME trioio_constraints_tests COMMAND trioio_constraints_tests)

    add_executable(trioio_oracle_memory_tests tests/OracleMemoryTests.cpp)
    target_link_libraries(trioio_oracle_memory_tests PRIVATE trioio_core)
    add_test(NAME trioio_oracle_memory_tests COMMAND trioio_oracle_memory_tests)

    add_executable(trioio_voicing_tests tests/VoicingTests.cpp)
    target_link_libraries(trioio_voicing_tests PRIVATE trioio_core)
    add_test(NAME trioio_voicing_tests COMMAND trioio_voicing_tests)

    add_executable(trioio_mashup_tests tests/MashupTests.cpp)
    target_link_libraries(trioio_mashup_tests PRIVATE trioio_core)
    add_test(NAME trioio_mashup_tests COMMAND trioio_mashup_tests)

    add_executable(trioio_rhythm_cycles_tests tests/RhythmCyclesTests.cpp)
    target_link_libraries(trioio_rhythm_cycles_tests PRIVATE trioio_core)
    target_compile_definitions(trioio_rhythm_cycles_tests PRIVATE TRIOIO_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME trioio_rhythm_cycles_tests COMMAND trioio_rhythm_cycles_tests)

    add_executable(trioio_recombination_tests tests/RecombinationTests.cpp)
    target_link_libraries(trioio_recombination_tests PRIVATE trioio_core)
    target_compile_definitions(trioio_recombination_tests PRIVATE TRIOIO_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME trioio_recombination_tests COMMAND trioio_recombination_tests)

    add_executable(trioio_scale_idiom_tests tests/ScaleIdiomTests.cpp)
    target_link_libraries(trioio_scale_idiom_tests PRIVATE trioio_core)
    target_compile_definitions(trioio_scale_idiom_tests PRIVATE TRIOIO_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME trioio_scale_idiom_tests COMMAND trioio_scale_idiom_tests)

    add_executable(trioio_scale_affect_tests tests/ScaleAffectTests.cpp)
    target_link_libraries(trioio_scale_affect_tests PRIVATE trioio_core)
    add_test(NAME trioio_scale_affect_tests COMMAND trioio_scale_affect_tests)

    add_executable(trioio_scale_machine_tests tests/ScaleMachineTests.cpp)
    target_link_libraries(trioio_scale_machine_tests PRIVATE trioio_core)
    add_test(NAME trioio_scale_machine_tests COMMAND trioio_scale_machine_tests)

    add_executable(trioio_scale_archive_tests tests/ScaleArchiveTests.cpp)
    target_link_libraries(trioio_scale_archive_tests PRIVATE trioio_core)
    target_compile_definitions(trioio_scale_archive_tests PRIVATE TRIOIO_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME trioio_scale_archive_tests COMMAND trioio_scale_archive_tests)

    add_executable(trioio_harmony_tests tests/HarmonyTests.cpp)
    target_link_libraries(trioio_harmony_tests PRIVATE trioio_core)
    add_test(NAME trioio_harmony_tests COMMAND trioio_harmony_tests)

    add_executable(trioio_scale_bank_tests tests/ScaleBankTests.cpp)
    target_link_libraries(trioio_scale_bank_tests PRIVATE trioio_core)
    add_test(NAME trioio_scale_bank_tests COMMAND trioio_scale_bank_tests)

    add_executable(trioio_mosaic_tests tests/MosaicTests.cpp)
    target_link_libraries(trioio_mosaic_tests PRIVATE trioio_core)
    add_test(NAME trioio_mosaic_tests COMMAND trioio_mosaic_tests)

    # bancada de andamentos: fora da suíte (minutos); docs/LEITURA_E_GABARITO.md
    add_executable(trioio_bancada_andamento tools/BancadaAndamento.cpp)
    target_link_libraries(trioio_bancada_andamento PRIVATE trioio_core)

    add_executable(trioio_window_policy_tests tests/WindowPolicyTests.cpp src/app/WindowPolicy.cpp)
    target_include_directories(trioio_window_policy_tests PRIVATE src/app)
    target_compile_features(trioio_window_policy_tests PRIVATE cxx_std_20)
    add_test(NAME trioio_window_policy_tests COMMAND trioio_window_policy_tests)

    add_executable(trioio_reading_tests tests/ReadingTests.cpp)
    target_link_libraries(trioio_reading_tests PRIVATE trioio_core)
    add_test(NAME trioio_reading_tests COMMAND trioio_reading_tests)

    add_executable(trioio_listening_tests tests/ListeningTests.cpp)
    target_link_libraries(trioio_listening_tests PRIVATE trioio_core)
    add_test(NAME trioio_listening_tests COMMAND trioio_listening_tests)

    add_executable(trioio_synth_families_tests tests/SynthFamiliesTests.cpp)
    target_link_libraries(trioio_synth_families_tests PRIVATE trioio_core)
    add_test(NAME trioio_synth_families_tests COMMAND trioio_synth_families_tests)

    add_executable(trioio_chain_tests tests/ChainTests.cpp)
    target_link_libraries(trioio_chain_tests PRIVATE trioio_core)
    add_test(NAME trioio_chain_tests COMMAND trioio_chain_tests)

    # Os testes usam `assert`/checagens próprias; em Release o NDEBUG
    # desligaria o `assert` e o teste passaria sem verificar nada
    # (achado em 2026-10-05). Mantém as asserções em qualquer configuração.
    set(TRIOIO_TEST_TARGETS trioio_brain_tests trioio_provenance_tests trioio_descriptors_tests
        trioio_chain_tests trioio_structure_tests trioio_composer_tests trioio_third_eye_tests
        trioio_synth_families_tests trioio_listening_tests trioio_oracle_memory_tests
        trioio_constraints_tests trioio_rhythm_field_tests trioio_mosaic_tests trioio_scale_bank_tests trioio_harmony_tests trioio_scale_archive_tests trioio_scale_machine_tests trioio_scale_affect_tests trioio_scale_idiom_tests trioio_recombination_tests trioio_rhythm_cycles_tests trioio_mashup_tests trioio_voicing_tests)
    if (TRIOIO_HAS_JUCE)
        foreach(par "manifest_json;ManifestJsonTests" "library_importer;LibraryImporterTests"
                "declaration;DeclarationTests"
                "analysis;AnalysisTests"
                "response;ResponseTests"
                "retention;RetentionTests"
                "prompts;PromptsTests"
                "semantic_bridge;SemanticBridgeTests")
            list(GET par 0 nome)
            list(GET par 1 arquivo)
            set(alvo trioio_${nome}_tests)
            juce_add_console_app(${alvo} PRODUCT_NAME "${alvo}")
            target_sources(${alvo} PRIVATE tests/${arquivo}.cpp ${TRIOIO_LIBRARY_SOURCES})
            target_include_directories(${alvo} PRIVATE src/library)
            target_compile_definitions(${alvo} PRIVATE
                TRIOIO_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
                TRIOIO_VERSION="${PROJECT_VERSION}"
                JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0 JUCE_USE_MP3AUDIOFORMAT=1)
            target_link_libraries(${alvo} PRIVATE
                trioio_core ${TRIOIO_LIBRARY_MODULES} juce::juce_recommended_config_flags)
            add_test(NAME ${alvo} COMMAND ${alvo})
            list(APPEND TRIOIO_TEST_TARGETS ${alvo})
        endforeach()
    endif()

    foreach(t ${TRIOIO_TEST_TARGETS})
        if (MSVC)
            target_compile_options(${t} PRIVATE /UNDEBUG /W4)
        else()
            target_compile_options(${t} PRIVATE -UNDEBUG -Wall -Wextra -Wpedantic)
        endif()
    endforeach()
endif()
