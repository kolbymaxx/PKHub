# Borealis Setup Recommendation

## Which fork?

**Recommended:** [xfangfang/borealis](https://github.com/xfangfang/borealis) (`wiliwili` branch) — or the closely related [XITRIX/borealis](https://github.com/XITRIX/borealis) (`moonlight_wiliwili` branch).

| Fork | Why |
|------|-----|
| **xfangfang/borealis** | Most actively maintained multi-platform Borealis; CMake-first; used by wiliwili / switchfin |
| **XITRIX/borealis** | Excellent Switch track record via Moonlight-Switch; good if we mirror that project’s CMake patterns |
| natinusala/borealis | Upstream; slower for Switch-focused features — use only as historical reference |

**PKHub default:** vendor **xfangfang/borealis** as a git submodule at `libs/borealis`, with the option to pin XITRIX’s branch if we hit Switch-specific regressions.

Template reference: [xfangfang/borealis_template](https://github.com/xfangfang/borealis_template)

## Toolchain & packages (devkitPro)

```bash
# Core Switch toolchain (assumed installed)
sudo (dkp-)pacman -S switch-dev switch-cmake

# Borealis / graphics deps
sudo (dkp-)pacman -S switch-glfw switch-mesa switch-glm switch-libwebp
# Optional: switch-libjpeg-turbo, switch-zlib, switch-freetype (often pulled transitively)
```

C++ standard: **C++20** (or C++17 minimum). Enable RTTI + exceptions (Borealis requires them).

## CMake integration (sketch)

```cmake
set(PLATFORM_SWITCH ON)
set(BOREALIS_LIBRARY ${CMAKE_SOURCE_DIR}/libs/borealis/library)
add_subdirectory(${BOREALIS_LIBRARY})

add_executable(PKHub ${PKHUB_SOURCES}
    ${BOREALIS_LIBRARY}/lib/platforms/switch/switch_wrapper.c)

target_link_libraries(PKHub PRIVATE borealis glfw3 EGL glapi drm_nouveau nx m)
# RomFS: resources/ → bundled into PKHub.nro
```

Build:

```bash
cmake -B build/switch -DPLATFORM_SWITCH=ON \
  -DCMAKE_TOOLCHAIN_FILE=${DEVKITPRO}/cmake/Switch.cmake
cmake --build build/switch -j$(nproc)
# produces PKHub.nro
```

## Useful libraries

| Library | Use |
|---------|-----|
| **Borealis** | UI, navigation, i18n, themes |
| **libnx** | FS, save data, account, applet |
| **nlohmann/json** (header-only) | `config.json`, Hub metadata |
| **miniz / zlib** | Compressed Hub packs, backup zips (Phase 2+) |
| **stb_image** or Borealis image loaders | Sprites / icons |
| *(optional later)* **PKHeX Core** concepts as reference only | Do **not** link C#; reimplement needed logic in C++ |

Do **not** depend on network stacks for Phase 1.

## Submodule init

```bash
git submodule add https://github.com/xfangfang/borealis.git libs/borealis
git submodule update --init --recursive
```

Until the submodule is added in a follow-up, `libs/README.md` documents the expected layout so CMake can fail with a clear message.

## RomFS layout

Borealis expects resources under RomFS (typically `romfs:/`):

```
resources/
├── i18n/en-US/app.json
├── xml/views/...
├── img/...
└── (borealis fonts/shaders merged at build time per template)
```

## Title override

Recommend running under title override of a Pokémon title when accessing that title’s save, **or** use `fsOpen_SaveData` / appropriate save mount APIs with the correct title ID + user account. Document both modes in-app; default UX: pick game → pick user → mount save.
