cmake_minimum_required(VERSION 3.16)

# Borealis options must be set before project()/add_subdirectory.
option(PLATFORM_SWITCH "Build Nintendo Switch .nro" OFF)
option(PLATFORM_DESKTOP "Build desktop target" ON)
option(PKHUB_BUNDLE_BOREALIS "Link Borealis when present" ON)
option(PKHUB_ENABLE_UI "Build Borealis UI (needs X11/Wayland + GL desktop deps)" OFF)
option(PKHUB_BUILD_TESTS "Build host unit tests" ON)

set(BOREALIS_PATH "${CMAKE_SOURCE_DIR}/libs/borealis")
set(BOREALIS_LIBRARY "${BOREALIS_PATH}/library")
set(HAVE_BOREALIS OFF)
set(ENABLE_BOREALIS_UI OFF)

if(PKHUB_BUNDLE_BOREALIS AND PKHUB_ENABLE_UI AND EXISTS "${BOREALIS_LIBRARY}/CMakeLists.txt")
    set(HAVE_BOREALIS ON)
endif()

# Avoid bundled GLFW Wayland scanner requirement on minimal Linux images.
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

if(HAVE_BOREALIS AND PLATFORM_DESKTOP AND NOT PLATFORM_SWITCH)
    # Match borealis demo defaults: desktop prefers SDL3 unless GLFW forced.
    if(NOT USE_GLFW AND NOT USE_SDL2 AND NOT USE_SDL3)
        set(USE_SDL3 ON CACHE BOOL "" FORCE)
    endif()
    include(${BOREALIS_LIBRARY}/cmake/toolchain.cmake)
endif()

include("${CMAKE_SOURCE_DIR}/cmake/PkhubVersion.cmake")

project(PKHub VERSION ${PKHUB_VERSION_MAJOR}.${PKHUB_VERSION_MINOR}.${PKHUB_VERSION_PATCH}
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PKHUB_AUTHOR "PKHub Contributors")
set(PKHUB_TITLEID "010000000000F1B0")
set(PKHUB_ICON "${CMAKE_SOURCE_DIR}/resources/img/icons/pkhub.jpg")

# Keep include/pkhub/app/Version.hpp in sync with cmake/PkhubVersion.cmake.
configure_file(
    "${CMAKE_SOURCE_DIR}/include/pkhub/app/Version.hpp.in"
    "${CMAKE_SOURCE_DIR}/include/pkhub/app/Version.hpp"
    @ONLY
)
message(STATUS "PKHub version label: ${PKHUB_VERSION_LABEL}")

set(PKHUB_SOURCES
    source/main.cpp
    source/app/AppContext.cpp
    source/core/pokemon/GameId.cpp
    source/core/pokemon/Pokemon.cpp
    source/core/pokemon/SpeciesIds.cpp
    source/core/pokemon/SpeciesNames.cpp
    source/core/pokemon/NatureNames.cpp
    source/core/save/Box.cpp
    source/core/save/SaveSession.cpp
    source/core/hub/HubStorage.cpp
    source/core/hub/HubPokemonCodec.cpp
    source/core/backup/BackupService.cpp
    source/core/safety/LegalityService.cpp
    source/core/safety/SafetyPolicy.cpp
    source/core/fs/Paths.cpp
    source/core/fs/FileBrowser.cpp
    source/services/TransferService.cpp
    source/backends/switch/SwitchSaveBackend.cpp
    source/backends/switch/SwitchSaveParser.cpp
    source/backends/switch/SwishCrypto.cpp
    source/backends/switch/PokeCrypto8.cpp
    source/backends/switch/Pk8Codec.cpp
    source/backends/switch/UnsupportedSaveBackend.cpp
    source/backends/raw/RawSaveBackend.cpp
    source/backends/raw/gba/GbaGen3.cpp
    source/backends/discovery/SaveDiscovery.cpp
    source/backends/discovery/SaveProbe.cpp
    source/platform/SaveAccess.cpp
    source/platform/SwitchSaveMount.cpp
    source/ui/UiBootstrap.cpp
    source/ui/SpritePlaceholder.cpp
    source/ui/SpriteService.cpp
    source/ui/views/PokemonSlotView.cpp
    source/ui/views/BoxGridView.cpp
    source/ui/activities/MainActivity.cpp
    source/ui/activities/SaveWorkspaceActivity.cpp
    source/ui/activities/EditorActivity.cpp
    source/ui/activities/FileBrowserActivity.cpp
)

add_executable(PKHub ${PKHUB_SOURCES})

target_include_directories(PKHub PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/json
    ${CMAKE_SOURCE_DIR}/libs/sha256
)

target_compile_options(PKHub PRIVATE
    -Wall -Wextra -Wno-unused-parameter
    -fexceptions -frtti
)

if(PLATFORM_SWITCH AND DEFINED ENV{DEVKITPRO})
    # Apply before Borealis so GLFW/EGL resolve; -isystem avoids tinyxml2 clashes.
    add_compile_options(
        "-isystem$ENV{DEVKITPRO}/portlibs/switch/include"
        "-isystem$ENV{DEVKITPRO}/libnx/include")
    link_directories(
        "$ENV{DEVKITPRO}/portlibs/switch/lib"
        "$ENV{DEVKITPRO}/libnx/lib")
endif()

if(HAVE_BOREALIS)
    message(STATUS "Borealis present — attempting UI link")
    set(BRLS_RESOURCES_DIR "${CMAKE_SOURCE_DIR}" CACHE STRING "" FORCE)
    # Borealis set_target_properties requires an explicit UNITY_BUILD value.
    if(NOT DEFINED BRLS_UNITY_BUILD)
        set(BRLS_UNITY_BUILD OFF CACHE BOOL "" FORCE)
    endif()
    # fmt os.cc uses POSIX APIs unavailable on Switch newlib.
    if(PLATFORM_SWITCH)
        set(FMT_OS OFF CACHE BOOL "" FORCE)
    endif()
    add_subdirectory(${BOREALIS_LIBRARY} EXCLUDE_FROM_ALL)
    if(TARGET borealis)
        target_compile_definitions(PKHub PRIVATE PKHUB_HAS_BOREALIS=1)
        target_link_libraries(PKHub PRIVATE borealis)
        set(ENABLE_BOREALIS_UI ON)
        # Ensure borealis resources are available next to the binary for desktop.
        if(PLATFORM_DESKTOP AND NOT PLATFORM_SWITCH)
            add_custom_command(TARGET PKHub POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:PKHub>/resources"
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${CMAKE_SOURCE_DIR}/resources" "$<TARGET_FILE_DIR:PKHub>/resources"
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${BOREALIS_PATH}/resources" "$<TARGET_FILE_DIR:PKHub>/resources"
                COMMENT "Copy Borealis + PKHub resources for desktop"
                VERBATIM
            )
        endif()
    else()
        message(WARNING "borealis target missing after add_subdirectory")
        set(HAVE_BOREALIS OFF)
    endif()
else()
    message(STATUS "Borealis UI disabled — headless / logic-only build")
endif()

if(PLATFORM_DESKTOP AND NOT PLATFORM_SWITCH)
    target_compile_definitions(PKHub PRIVATE PLATFORM_DESKTOP=1)
endif()

if(PLATFORM_SWITCH)
    target_compile_definitions(PKHub PRIVATE __SWITCH__=1 PLATFORM_SWITCH=1)
    if(EXISTS "${BOREALIS_LIBRARY}/lib/platforms/switch/switch_wrapper.c")
        target_sources(PKHub PRIVATE ${BOREALIS_LIBRARY}/lib/platforms/switch/switch_wrapper.c)
    endif()
    if(DEFINED ENV{DEVKITPRO})
        find_library(NX_LIB nx HINTS "$ENV{DEVKITPRO}/libnx/lib")
        if(NX_LIB)
            set(SWITCH_LIBS nx m)
            if(ENABLE_BOREALIS_UI)
                list(APPEND SWITCH_LIBS glfw3 EGL glapi drm_nouveau)
            endif()
            target_link_libraries(PKHub PRIVATE ${SWITCH_LIBS})
        endif()
        find_program(NX_ELF2NRO elf2nro HINTS "$ENV{DEVKITPRO}/tools/bin" /opt/devkitpro/tools/bin)
        find_program(NX_NACPTOOL nacptool HINTS "$ENV{DEVKITPRO}/tools/bin" /opt/devkitpro/tools/bin)
        if(NX_ELF2NRO AND NX_NACPTOOL)
            # Single shell step avoids generator-expression / quoting issues with nacptool.
            add_custom_command(TARGET PKHub POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E env
                        "NX_NACPTOOL=${NX_NACPTOOL}"
                        "NX_ELF2NRO=${NX_ELF2NRO}"
                        "PKHUB_AUTHOR=${PKHUB_AUTHOR}"
                        "PKHUB_VERSION=${PKHUB_VERSION_LABEL}"
                        "PKHUB_TITLEID=${PKHUB_TITLEID}"
                        "PKHUB_ICON=${PKHUB_ICON}"
                        "PKHUB_RESOURCES=${CMAKE_SOURCE_DIR}/resources"
                        "BOREALIS_RESOURCES=${BOREALIS_PATH}/resources"
                        "OUT_DIR=$<TARGET_FILE_DIR:PKHub>"
                        "ELF_PATH=$<TARGET_FILE:PKHub>"
                        bash "${CMAKE_SOURCE_DIR}/scripts/package_nro.sh"
                COMMENT "Packaging PKHub.nro"
                VERBATIM
            )
        endif()
    else()
        message(STATUS "DEVKITPRO not set — Switch packaging skipped")
    endif()
endif()

if(PKHUB_BUILD_TESTS AND NOT PLATFORM_SWITCH)
    enable_testing()
    set(PKHUB_TEST_COMMON
        source/core/pokemon/GameId.cpp
        source/core/pokemon/Pokemon.cpp
        source/core/save/Box.cpp
        source/core/hub/HubStorage.cpp
        source/core/hub/HubPokemonCodec.cpp
        source/core/fs/Paths.cpp
        source/core/safety/LegalityService.cpp
        source/core/safety/SafetyPolicy.cpp
        source/services/TransferService.cpp
        source/backends/switch/UnsupportedSaveBackend.cpp
        source/ui/SpritePlaceholder.cpp
    )
    add_executable(pkhub_smoke tests/smoke_test.cpp ${PKHUB_TEST_COMMON})
    target_include_directories(pkhub_smoke PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/libs/json)
    target_compile_options(pkhub_smoke PRIVATE -Wall -Wextra -Wno-unused-parameter -fexceptions -frtti)
    add_test(NAME smoke COMMAND pkhub_smoke)

    add_executable(pkhub_hub_persist tests/hub_persist_test.cpp
        source/core/pokemon/GameId.cpp
        source/core/pokemon/Pokemon.cpp
        source/core/save/Box.cpp
        source/core/hub/HubStorage.cpp
        source/core/hub/HubPokemonCodec.cpp
        source/core/fs/Paths.cpp
    )
    target_include_directories(pkhub_hub_persist PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/libs/json)
    target_compile_options(pkhub_hub_persist PRIVATE -Wall -Wextra -Wno-unused-parameter -fexceptions -frtti)
    add_test(NAME hub_persist COMMAND pkhub_hub_persist)

    add_executable(pkhub_gba_roundtrip tests/gba_roundtrip_test.cpp
        source/backends/raw/gba/GbaGen3.cpp
        source/core/pokemon/GameId.cpp
        source/core/pokemon/Pokemon.cpp
        source/core/save/Box.cpp
    )
    target_include_directories(pkhub_gba_roundtrip PRIVATE ${CMAKE_SOURCE_DIR}/include)
    add_test(NAME gba_roundtrip COMMAND pkhub_gba_roundtrip)

    add_executable(pkhub_gba_writeback tests/gba_writeback_test.cpp
        source/backends/raw/gba/GbaGen3.cpp
        source/backends/raw/RawSaveBackend.cpp
        source/backends/discovery/SaveProbe.cpp
        source/backends/switch/SwishCrypto.cpp
        source/core/pokemon/GameId.cpp
        source/core/pokemon/Pokemon.cpp
        source/core/save/Box.cpp
        source/core/fs/Paths.cpp
    )
    target_include_directories(pkhub_gba_writeback PRIVATE
        ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/libs/sha256)
    target_compile_options(pkhub_gba_writeback PRIVATE -Wall -Wextra -Wno-unused-parameter -fexceptions -frtti)
    add_test(NAME gba_writeback COMMAND pkhub_gba_writeback)

    set(PKHUB_SWITCH_CRYPTO_SOURCES
        source/backends/switch/SwitchSaveBackend.cpp
        source/backends/switch/SwitchSaveParser.cpp
        source/backends/switch/SwishCrypto.cpp
        source/backends/switch/PokeCrypto8.cpp
        source/backends/switch/Pk8Codec.cpp
        source/backends/switch/UnsupportedSaveBackend.cpp
        source/platform/SaveAccess.cpp
        source/platform/SwitchSaveMount.cpp
        source/core/pokemon/GameId.cpp
        source/core/pokemon/Pokemon.cpp
        source/core/save/Box.cpp
        source/core/fs/Paths.cpp
    )

    add_executable(pkhub_emu_scan tests/emu_scan_test.cpp
        source/backends/discovery/SaveDiscovery.cpp
        source/backends/discovery/SaveProbe.cpp
        source/backends/raw/RawSaveBackend.cpp
        source/backends/raw/gba/GbaGen3.cpp
        ${PKHUB_SWITCH_CRYPTO_SOURCES}
        source/core/fs/FileBrowser.cpp
    )
    target_include_directories(pkhub_emu_scan PRIVATE
        ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/libs/sha256)
    target_compile_options(pkhub_emu_scan PRIVATE -Wall -Wextra -Wno-unused-parameter -fexceptions -frtti)
    add_test(NAME emu_scan COMMAND pkhub_emu_scan)

    add_executable(pkhub_switch_mount tests/switch_mount_test.cpp ${PKHUB_SWITCH_CRYPTO_SOURCES})
    target_include_directories(pkhub_switch_mount PRIVATE
        ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/libs/sha256)
    target_compile_options(pkhub_switch_mount PRIVATE -Wall -Wextra -Wno-unused-parameter -fexceptions -frtti)
    add_test(NAME switch_mount COMMAND pkhub_switch_mount)

    add_executable(pkhub_swish_roundtrip tests/swish_roundtrip_test.cpp ${PKHUB_SWITCH_CRYPTO_SOURCES})
    target_include_directories(pkhub_swish_roundtrip PRIVATE
        ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/libs/sha256)
    target_compile_options(pkhub_swish_roundtrip PRIVATE -Wall -Wextra -Wno-unused-parameter -fexceptions -frtti)
    add_test(NAME swish_roundtrip COMMAND pkhub_swish_roundtrip)

    add_executable(pkhub_species_ids tests/species_ids_test.cpp
        source/core/pokemon/SpeciesIds.cpp
        source/core/pokemon/Pokemon.cpp
        source/core/pokemon/GameId.cpp)
    target_include_directories(pkhub_species_ids PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_options(pkhub_species_ids PRIVATE -Wall -Wextra -Wno-unused-parameter -fexceptions -frtti)
    add_test(NAME species_ids COMMAND pkhub_species_ids)

    add_executable(pkhub_species_names tests/species_names_test.cpp
        source/core/pokemon/SpeciesNames.cpp
        source/core/pokemon/SpeciesIds.cpp
        source/core/pokemon/NatureNames.cpp
        source/core/pokemon/Pokemon.cpp
        source/core/pokemon/GameId.cpp)
    target_include_directories(pkhub_species_names PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_options(pkhub_species_names PRIVATE -Wall -Wextra -Wno-unused-parameter -fexceptions -frtti)
    add_test(NAME species_names COMMAND pkhub_species_names)
endif()

message(STATUS "PKHub ${PROJECT_VERSION} | BorealisUI: ${ENABLE_BOREALIS_UI} | SWITCH: ${PLATFORM_SWITCH} | DESKTOP: ${PLATFORM_DESKTOP}")
