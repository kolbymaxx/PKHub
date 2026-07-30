# PKHub

Next-generation multi-generation Pokémon save editor and Hub manager for **Atmosphere** on Nintendo Switch.

> Surpass classic 3DS PKSM in polish, speed, safety, and breadth — modern Borealis UI, official Switch titles + emulator saves, and first-class Hub Storage.

## Status

**Phase 0 — Architecture & skeleton** (this branch)

- Architecture, navigation, and Borealis setup docs
- Core interfaces: `ISaveBackend`, `Pokemon`, `HubStorage`, `TransferService`, …
- CMake project that builds a headless smoke binary on host, and `.nro` when the Switch toolchain + Borealis are present

## Docs

| Doc | Contents |
|-----|----------|
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Layers, folders, data flow |
| [docs/NAVIGATION.md](docs/NAVIGATION.md) | UI navigation (Switch / Emulator / Hub) |
| [docs/BOREALIS_SETUP.md](docs/BOREALIS_SETUP.md) | Borealis fork, packages, CMake |
| [docs/QUESTIONS.md](docs/QUESTIONS.md) | Clarifying questions before Phase 1 |

## Quick start (host smoke)

```bash
cmake -B build/host -DPLATFORM_SWITCH=OFF -DPKHUB_BUILD_TESTS=ON
cmake --build build/host -j
ctest --test-dir build/host --output-on-failure
./build/host/PKHub
```

## Switch build (devkitPro)

```bash
git submodule update --init --recursive   # after adding libs/borealis
cmake -B build/switch -DPLATFORM_SWITCH=ON \
  -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/Switch.cmake"
cmake --build build/switch -j
# → build/switch/PKHub.nro
```

Install to `sdmc:/switch/PKHub/PKHub.nro`. Prefer title override when editing official saves.

## Supported games (target)

**Switch:** Scarlet/Violet (+DLC), Legends Z-A, Sword/Shield (+DLC), Legends Arceus, BDSP  

**Emulated:** GBA (Gen 3) → DS (Gen 4–5) → 3DS (Gen 6–7)

## Safety

Automatic backup before any save write. Legality / online-risk warnings are advisory but visible.

## License

TBD — see `docs/QUESTIONS.md`.
