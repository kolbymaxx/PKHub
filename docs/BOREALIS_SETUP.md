# Borealis Setup Recommendation

## Which fork? (locked)

**PKHub default:** [XITRIX/borealis](https://github.com/XITRIX/borealis) (`moonlight_wiliwili` branch), per project decision and common Switch homebrew guidance (e.g. Moonlight-Switch, GBAtemp recommendations).

| Fork | Role for PKHub |
|------|----------------|
| **XITRIX/borealis** | **Primary submodule** — Switch-proven, recommended by the community for native-feeling homebrew |
| **xfangfang/borealis** | Compatible sibling (shared ancestry). Useful reference for multi-platform / desktop CMake patterns (wiliwili, borealis_template). Prefer only if XITRIX lacks a needed desktop path |
| natinusala/borealis | Historical upstream only |

**Note:** XITRIX and xfangfang forks track closely. If desktop iteration is blocked on XITRIX’s branch, we may temporarily follow xfangfang’s desktop CMake while keeping Switch parity with XITRIX — document any pin in `libs/README.md`.

Template / desktop reference: [xfangfang/borealis_template](https://github.com/xfangfang/borealis_template) · Switch reference: [XITRIX/Moonlight-Switch](https://github.com/XITRIX/Moonlight-Switch)

## Toolchain & packages (devkitPro)

```bash
sudo (dkp-)pacman -S switch-dev switch-cmake
sudo (dkp-)pacman -S switch-glfw switch-mesa switch-glm switch-libwebp
```

Desktop (Ubuntu/Debian-ish): GLFW, GLM, and whatever Borealis’s CMake requests (`libglfw3-dev`, etc.).

C++ standard: **C++20**. Enable RTTI + exceptions.

## CMake — desktop (preferred for UI iteration)

```bash
git submodule add -b moonlight_wiliwili https://github.com/XITRIX/borealis.git libs/borealis
git submodule update --init --recursive

cmake -B build/desktop -DPLATFORM_DESKTOP=ON -DPLATFORM_SWITCH=OFF
cmake --build build/desktop -j
./build/desktop/PKHub
```

Without the submodule, the same flags build a **headless** smoke binary for core logic tests.

## CMake — Switch `.nro`

```bash
cmake -B build/switch -DPLATFORM_SWITCH=ON -DPLATFORM_DESKTOP=OFF \
  -DCMAKE_TOOLCHAIN_FILE=${DEVKITPRO}/cmake/Switch.cmake
cmake --build build/switch -j$(nproc)
# → PKHub.nro
```

## Useful libraries

| Library | Use |
|---------|-----|
| **Borealis (XITRIX)** | UI, navigation, i18n, themes |
| **libnx** | FS, save data, account, applet |
| **nlohmann/json** | `config.json`, Hub `hub.json` metadata |
| **miniz / zlib** | backup zips (Phase 2+) |
| Placeholders | Phase 1 sprites — no sprite atlas required |

## Submodule init

```bash
git submodule add -b moonlight_wiliwili https://github.com/XITRIX/borealis.git libs/borealis
git submodule update --init --recursive
```

## Save access (Switch)

1. **Title override (priority)** — hold R while launching the Pokémon title, then run PKHub for the most reliable save access.
2. **`fsOpen_SaveData` + user picker** — cleaner UX when the mount succeeds without override.

In-app: explain both; default `SaveAccessMode::Auto` tries override context first.

## RomFS layout

```
resources/
├── i18n/en-US/app.json
├── xml/views/...
└── img/...   # placeholders first; real sprites later
```
