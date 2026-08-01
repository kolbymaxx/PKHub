# PKHub

Next-generation multi-generation Pokémon save editor and Hub manager for **Atmosphere** on Nintendo Switch.

> Surpass classic 3DS PKSM in polish, speed, safety, and breadth — modern Borealis UI, official Switch titles + emulator saves, and first-class Hub Storage.

**License:** GPLv3 — see [LICENSE](LICENSE).

## Status

**Phase 1 in progress** — see [docs/PHASE1.md](docs/PHASE1.md) and [docs/DECISIONS.md](docs/DECISIONS.md).

## Docs

| Doc | Contents |
|-----|----------|
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Layers, folders, data flow |
| [docs/DECISIONS.md](docs/DECISIONS.md) | Locked Phase 1+ decisions |
| [docs/PHASE1.md](docs/PHASE1.md) | Phase 1 progress / next steps |
| [docs/NAVIGATION.md](docs/NAVIGATION.md) | UI navigation |
| [docs/BOREALIS_SETUP.md](docs/BOREALIS_SETUP.md) | XITRIX Borealis, desktop + Switch |
| [docs/HUB_FORMAT.md](docs/HUB_FORMAT.md) | Versioned binary + JSON Hub layout |
| [docs/QUESTIONS.md](docs/QUESTIONS.md) | Remaining soft questions |

## Quick start (desktop / host)

```bash
# Headless core + tests (default; no GUI deps)
cmake -B build/desktop -DPLATFORM_DESKTOP=ON -DPLATFORM_SWITCH=OFF -DPKHUB_ENABLE_UI=OFF
cmake --build build/desktop -j
ctest --test-dir build/desktop --output-on-failure
./build/desktop/PKHub
```

Borealis UI (needs X11/Wayland + GL — see `docs/BOREALIS_SETUP.md`):

```bash
cmake -B build/ui -DPLATFORM_DESKTOP=ON -DPKHUB_ENABLE_UI=ON
cmake --build build/ui -j
```

Borealis submodule (already wired on Phase 1 branch):

```bash
git submodule update --init --recursive
```

## Switch build

With DevKitPro installed locally:

```bash
cmake -B build/switch -DPLATFORM_SWITCH=ON -DPLATFORM_DESKTOP=OFF \
  -DPKHUB_ENABLE_UI=ON -DBRLS_UNITY_BUILD=OFF -DFMT_OS=OFF \
  -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/Switch.cmake"
cmake --build build/switch -j
```

Or via Docker (recommended in CI / cloud agents):

```bash
./scripts/build_switch_nro.sh   # → build/switch/PKHub.nro
```

RomFS includes National Dex sprites (`resources/img/pokemon/` + shiny) and Borealis assets.

Prefer **title override** when editing official saves; `fsOpen_SaveData` + user picker is supported for SwSh/SV.

## Highlights of locked decisions

- Legends Z-A: **stub** until format is known  
- Soft legality warnings; confirm only for brick / extreme risk  
- Placeholder sprites in Phase 1  
- Borealis: **XITRIX** fork  
- Hub: versioned `.pkbox` + `hub.json`  
- Clean-room format work only (no PKHeX/PKSM source copies)
