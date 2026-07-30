cmake_minimum_required(VERSION 3.16)
project(PKHub VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build targets: Switch .nro (default when toolchain present) and/or desktop UI.
option(PLATFORM_SWITCH "Build Nintendo Switch target" OFF)
option(PLATFORM_DESKTOP "Build desktop Borealis / headless host target" ON)
option(PKHUB_BUNDLE_BOREALIS "Build against libs/borealis when present" ON)
option(PKHUB_BUILD_TESTS "Build host unit tests" ON)

set(PKHUB_AUTHOR "PKHub Contributors")
set(PKHUB_TITLEID "010000000000F1B0")  # placeholder homebrew title id

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------
set(PKHUB_SOURCES
    source/main.cpp
    source/core/pokemon/GameId.cpp
    source/core/pokemon/Pokemon.cpp
    source/core/save/Box.cpp
    source/core/save/SaveSession.cpp
    source/core/hub/HubStorage.cpp
    source/core/hub/HubPokemonCodec.cpp
    source/core/backup/BackupService.cpp
    source/core/safety/LegalityService.cpp
    source/core/safety/SafetyPolicy.cpp
    source/core/fs/Paths.cpp
    source/services/TransferService.cpp
    source/backends/switch/SwitchSaveBackend.cpp
    source/backends/switch/UnsupportedSaveBackend.cpp
    source/backends/raw/RawSaveBackend.cpp
    source/backends/raw/gba/GbaGen3.cpp
    source/backends/discovery/SaveDiscovery.cpp
    source/platform/SaveAccess.cpp
    source/ui/UiBootstrap.cpp
    source/ui/SpritePlaceholder.cpp
)

set(BOREALIS_PATH "${CMAKE_SOURCE_DIR}/libs/borealis")
set(BOREALIS_LIBRARY "${BOREALIS_PATH}/library")
set(HAVE_BOREALIS OFF)

if(PKHUB_BUNDLE_BOREALIS AND EXISTS "${BOREALIS_LIBRARY}/CMakeLists.txt")
    set(HAVE_BOREALIS ON)
elseif(PKHUB_BUNDLE_BOREALIS AND EXISTS "${BOREALIS_LIBRARY}/cmake/toolchain.cmake")
    set(HAVE_BOREALIS ON)
elseif(PKHUB_BUNDLE_BOREALIS AND EXISTS "${BOREALIS_PATH}/library/include/borealis.hpp")
    set(HAVE_BOREALIS ON)
endif()

if(PLATFORM_SWITCH AND PLATFORM_DESKTOP)
    message(WARNING "Both PLATFORM_SWITCH and PLATFORM_DESKTOP are ON; prefer separate build dirs.")
endif()

# -----------------------------------------------------------------------------
# Target
# -----------------------------------------------------------------------------
add_executable(PKHub ${PKHUB_SOURCES})

target_include_directories(PKHub PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/json
)

target_compile_options(PKHub PRIVATE
    -Wall -Wextra -Wno-unused-parameter
    -fexceptions -frtti
)

if(HAVE_BOREALIS)
    message(STATUS "Borealis found — enabling UI build")
    target_compile_definitions(PKHub PRIVATE PKHUB_HAS_BOREALIS=1)

    if(EXISTS "${BOREALIS_LIBRARY}/CMakeLists.txt")
        # XITRIX / xfangfang-style CMake
        if(PLATFORM_DESKTOP AND NOT PLATFORM_SWITCH)
            set(PLATFORM_DESKTOP ON CACHE BOOL "" FORCE)
        endif()
        add_subdirectory(${BOREALIS_LIBRARY} EXCLUDE_FROM_ALL)
        target_link_libraries(PKHub PRIVATE borealis)
    else()
        target_include_directories(PKHub PRIVATE
            ${BOREALIS_LIBRARY}/include
            ${BOREALIS_LIBRARY}/lib/extern/fmt/include
            ${BOREALIS_LIBRARY}/lib/extern/yoga
        )
        message(WARNING "Borealis CMakeLists not found; includes only. See docs/BOREALIS_SETUP.md")
    endif()
else()
    message(STATUS "Borealis not present — building headless skeleton (add libs/borealis submodule)")
endif()

# -----------------------------------------------------------------------------
# Desktop host
# -----------------------------------------------------------------------------
if(PLATFORM_DESKTOP AND NOT PLATFORM_SWITCH)
    target_compile_definitions(PKHub PRIVATE PLATFORM_DESKTOP=1)
    message(STATUS "Desktop target enabled (UI when Borealis present, else headless)")
endif()

# -----------------------------------------------------------------------------
# Switch packaging
# -----------------------------------------------------------------------------
if(PLATFORM_SWITCH)
    target_compile_definitions(PKHub PRIVATE __SWITCH__=1 PLATFORM_SWITCH=1)

    if(CMAKE_SYSTEM_NAME STREQUAL "Generic" OR DEFINED ENV{DEVKITPRO})
        find_library(NX_LIB nx)
        if(NX_LIB)
            set(SWITCH_LIBS nx m)
            if(HAVE_BOREALIS)
                list(APPEND SWITCH_LIBS glfw3 EGL glapi drm_nouveau)
            endif()
            target_link_libraries(PKHub PRIVATE ${SWITCH_LIBS})
        else()
            message(WARNING "libnx not found — configure with Switch toolchain to produce .nro")
        endif()

        set(PROJECT_RESOURCES "${CMAKE_SOURCE_DIR}/resources")
        set(PROJECT_ICON "${CMAKE_SOURCE_DIR}/resources/icon.jpg")
        if(NOT EXISTS "${PROJECT_ICON}")
            set(PROJECT_ICON "")
        endif()

        find_program(NX_ELF2NRO elf2nro HINTS "$ENV{DEVKITPRO}/tools/bin" /opt/devkitpro/tools/bin)
        find_program(NX_NACPTOOL nacptool HINTS "$ENV{DEVKITPRO}/tools/bin" /opt/devkitpro/tools/bin)

        if(NX_ELF2NRO AND NX_NACPTOOL)
            add_custom_command(TARGET PKHub POST_BUILD
                COMMAND ${NX_NACPTOOL} --create "PKHub" "${PKHUB_AUTHOR}"
                        "${PROJECT_VERSION}" "$<TARGET_FILE_DIR:PKHub>/PKHub.nacp"
                        --titleid=${PKHUB_TITLEID}
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:PKHub>/romfs"
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${PROJECT_RESOURCES}" "$<TARGET_FILE_DIR:PKHub>/romfs"
                COMMAND ${NX_ELF2NRO} "$<TARGET_FILE:PKHub>"
                        "$<TARGET_FILE_DIR:PKHub>/PKHub.nro"
                        --nacp="$<TARGET_FILE_DIR:PKHub>/PKHub.nacp"
                        --romfsdir="$<TARGET_FILE_DIR:PKHub>/romfs"
                        $<$<BOOL:${PROJECT_ICON}>:--icon=${PROJECT_ICON}>
                COMMENT "Packaging PKHub.nro"
                VERBATIM
            )
        else()
            message(STATUS "elf2nro/nacptool not found — ELF-only build; .nro packaging skipped")
        endif()
    else()
        message(STATUS "Host machine without Switch toolchain — .nro packaging skipped")
    endif()
endif()

# -----------------------------------------------------------------------------
# Host unit tests
# -----------------------------------------------------------------------------
if(PKHUB_BUILD_TESTS)
    enable_testing()
    add_executable(pkhub_smoke tests/smoke_test.cpp
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
    target_include_directories(pkhub_smoke PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/libs/json
    )
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
    target_include_directories(pkhub_hub_persist PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/libs/json
    )
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
endif()

message(STATUS "PKHub ${PROJECT_VERSION} | Borealis: ${HAVE_BOREALIS} | SWITCH: ${PLATFORM_SWITCH} | DESKTOP: ${PLATFORM_DESKTOP}")
