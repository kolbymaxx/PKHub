cmake_minimum_required(VERSION 3.16)
project(PKHub VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PLATFORM_SWITCH "Build Nintendo Switch .nro" ON)
option(PKHUB_BUNDLE_BOREALIS "Build against libs/borealis when present" ON)

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
    source/core/backup/BackupService.cpp
    source/core/safety/LegalityService.cpp
    source/core/fs/Paths.cpp
    source/services/TransferService.cpp
    source/backends/switch/SwitchSaveBackend.cpp
    source/backends/raw/RawSaveBackend.cpp
    source/backends/discovery/SaveDiscovery.cpp
    source/ui/UiBootstrap.cpp
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

# -----------------------------------------------------------------------------
# Target
# -----------------------------------------------------------------------------
add_executable(PKHub ${PKHUB_SOURCES})

target_include_directories(PKHub PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_options(PKHub PRIVATE
    -Wall -Wextra -Wno-unused-parameter
    -fexceptions -frtti
)

if(HAVE_BOREALIS)
    message(STATUS "Borealis found — enabling UI build")
    target_compile_definitions(PKHub PRIVATE PKHUB_HAS_BOREALIS=1)

    # Prefer xfangfang-style CMake if present.
    if(EXISTS "${BOREALIS_LIBRARY}/CMakeLists.txt")
        add_subdirectory(${BOREALIS_LIBRARY} EXCLUDE_FROM_ALL)
        target_link_libraries(PKHub PRIVATE borealis)
    else()
        # Fallback include path; consumer must link borealis objects via Makefile glue.
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
# Switch packaging
# -----------------------------------------------------------------------------
if(PLATFORM_SWITCH)
    target_compile_definitions(PKHub PRIVATE __SWITCH__=1 PLATFORM_SWITCH=1)

    if(CMAKE_SYSTEM_NAME STREQUAL "Generic" OR DEFINED ENV{DEVKITPRO})
        # When using official Switch toolchain file, link libnx + EGL stack.
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

        if(COMMAND nx_generate_nacp OR DEFINED NX_ELF2NRO_EXE OR DEFINED CMAKE_NX_ELF2NRO)
            # toolchain-provided helpers vary; provide a portable custom command when tools exist
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
        message(STATUS "Host build: Switch .nro packaging skipped (set DEVKITPRO / Switch toolchain)")
    endif()
endif()

# -----------------------------------------------------------------------------
# Host unit tests (parsers) — optional
# -----------------------------------------------------------------------------
option(PKHUB_BUILD_TESTS "Build host unit tests" OFF)
if(PKHUB_BUILD_TESTS)
    enable_testing()
    add_executable(pkhub_smoke tests/smoke_test.cpp
        source/core/pokemon/GameId.cpp
        source/core/pokemon/Pokemon.cpp
        source/core/save/Box.cpp
        source/core/hub/HubStorage.cpp
        source/services/TransferService.cpp
    )
    target_include_directories(pkhub_smoke PRIVATE ${CMAKE_SOURCE_DIR}/include)
    add_test(NAME smoke COMMAND pkhub_smoke)
endif()

message(STATUS "PKHub ${PROJECT_VERSION} | Borealis: ${HAVE_BOREALIS} | PLATFORM_SWITCH: ${PLATFORM_SWITCH}")
