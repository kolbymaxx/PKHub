# PKHub

Next-generation multi-generation Pokémon save editor and Hub manager for **Atmosphere** on Nintendo Switch.

> Surpass classic 3DS PKSM in polish, speed, safety, and breadth — modern Borealis UI, official Switch titles + emulator saves, and first-class Hub Storage.

**License:** GPLv3 — see [LICENSE](LICENSE).

## Status

**Phase 0 — Architecture & skeleton**

Locked product decisions are in [docs/DECISIONS.md](docs/DECISIONS.md).

## Docs

| Doc | Contents |
|-----|----------|
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Layers, folders, data flow |
| [docs/DECISIONS.md](docs/DECISIONS.md) | Locked Phase 1+ decisions |
| [docs/NAVIGATION.md](docs/NAVIGATION.md) | UI navigation |
| [docs/BOREALIS_SETUP.md](docs/BOREALIS_SETUP.md) | XITRIX Borealis, desktop + Switch |
| [docs/HUB_FORMAT.md](docs/HUB_FORMAT.md) | Versioned binary + JSON Hub layout |
| [docs/QUESTIONS.md](docs/QUESTIONS.md) | Remaining soft questions |

## Quick start (desktop / host)

```bash
# Headless core smoke (no Borealis submodule yet)
cmake -B build/desktop -DPLATFORM_DESKTOP=ON -DPLATFORM_SWITCH=OFF
cmake --build build/desktop -j
ctest --test-dir build/desktop --output-on-failure
./build/desktop/PKHub
```

With Borealis (UI iteration):

```bash
git submodule add -b moonlight_wiliwili https://github.com/XITRIX/borealis.git libs/borealis
git submodule update --init --recursive
# then same cmake -DPLATFORM_DESKTOP=ON …
```

## Switch build

```bash
cmake -B build/switch -DPLATFORM_SWITCH=ON -DPLATFORM_DESKTOP=OFF \
  -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/Switch.cmake"
cmake --build build/switch -j
```

Prefer **title override** when editing official saves; `fsOpen_SaveData` + user picker is also planned.

## Highlights of locked decisions

- Legends Z-A: **stub** until format is known  
- Soft legality warnings; confirm only for brick / extreme risk  
- Placeholder sprites in Phase 1  
- Borealis: **XITRIX** fork  
- Hub: versioned `.pkbox` + `hub.json`  
- Clean-room format work only (no PKHeX/PKSM source copies)
