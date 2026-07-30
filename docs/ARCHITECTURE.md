# PKHub Architecture

Next-generation multi-generation Pokémon save editor and manager for Atmosphere CFW.

## Design Principles

1. **Backend-agnostic UI** — Screens talk only to `ISaveBackend` / `HubStorage`, never to game-specific formats.
2. **Unified Pokémon model** — One editable `Pokemon` object across gens; backends serialize/deserialize.
3. **Safety first** — Every write is preceded by an automatic backup; legality/online risk uses **soft warnings**, with **confirm dialogs only** for save-brick / extreme-risk actions (see `SafetyPolicy`).
4. **Offline-first** — No network required for core features; Phase 1 uses sprite placeholders.
5. **Hub as a first-class save** — Hub Storage implements the same box/party abstractions as game saves (versioned binary + JSON metadata).
6. **Clean-room formats** — Public save structures reimplemented in C++; do not copy PKHeX/PKSM source.
7. **Dual platforms** — Desktop Borealis for UI iteration; Switch `.nro` for device.

---

## High-Level Layers

```
┌─────────────────────────────────────────────────────────────┐
│  UI (Borealis)                                              │
│  Activities · Tabs · Views · Dialogs                        │
└───────────────────────────┬─────────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────┐
│  Application Services                                       │
│  SessionManager · TransferService · SearchService            │
│  BackupService · LegalityService · SpriteCache              │
└───────────────────────────┬─────────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────┐
│  Domain Core                                                │
│  Pokemon · Box · Party · Inventory · Generation · GameId    │
│  HubStorage · ISaveBackend · SaveSession                    │
└───────────┬─────────────────────────────┬───────────────────┘
            │                             │
┌───────────▼───────────┐     ┌───────────▼───────────────────┐
│  SwitchSaveBackend    │     │  RawSaveBackend               │
│  (libnx fsSave*)      │     │  (.sav / .dsv / .srm / 3DS)   │
└───────────────────────┘     └───────────────┬───────────────┘
                                              │
                                  ┌───────────▼───────────────┐
                                  │  Format parsers (gen 3–9) │
                                  │  GBA · NDS · 3DS · SWSH…  │
                                  └───────────────────────────┘
```

---

## Folder Structure

```
PKHub/
├── CMakeLists.txt
├── README.md
├── docs/
│   ├── ARCHITECTURE.md          ← this file
│   ├── DECISIONS.md
│   ├── NAVIGATION.md
│   ├── BOREALIS_SETUP.md
│   ├── HUB_FORMAT.md
│   └── QUESTIONS.md
├── include/pkhub/
│   ├── core/
│   │   ├── pokemon/             # Pokemon, Species, Moves, Types…
│   │   ├── save/                # ISaveBackend, Box, Party, Session
│   │   ├── hub/                 # HubStorage
│   │   ├── safety/              # Legality, RiskLevel, warnings
│   │   ├── backup/              # BackupService
│   │   └── fs/                  # Path helpers, SD layout
│   ├── backends/                # Backend factories / discovery
│   ├── services/                # Transfer, Search, Session
│   ├── ui/                      # Activity / Tab declarations
│   └── platform/                # Switch-specific wrappers
├── source/
│   ├── main.cpp
│   ├── app/                     # App bootstrap, config
│   ├── core/                    # Domain implementations
│   ├── backends/
│   │   ├── switch/              # Official title save I/O
│   │   ├── raw/                 # Emulator file parsers
│   │   │   ├── gba/
│   │   │   ├── nds/
│   │   │   ├── ctr/             # 3DS
│   │   │   └── switch_emu/      # future / shared helpers
│   │   └── discovery/           # RetroArch path scanners
│   ├── services/
│   ├── ui/
│   │   ├── activities/
│   │   ├── tabs/
│   │   ├── views/
│   │   └── dialogs/
│   └── platform/
├── resources/                   # Borealis RomFS
│   ├── i18n/
│   ├── xml/
│   └── img/
├── libs/                        # git submodules (borealis, …)
├── tools/                       # host-side asset/codegen helpers
└── tests/                       # host-unit tests for parsers
```

SD card runtime layout (created on first launch):

```
sdmc:/switch/PKHub/
├── config.json
├── hub/
│   ├── hub.pkhub                # Hub Storage database
│   └── boxes/
├── backups/
│   └── <gameId>/<timestamp>/
├── cache/
│   └── sprites/
└── logs/
```

---

## Core Interfaces (Summary)

| Interface / Type | Role |
|------------------|------|
| `ISaveBackend` | Open/close/load/save; expose boxes, party, inventory; report `GameId` / generation |
| `SwitchSaveBackend` | Official titles via libnx save data APIs + title ID map |
| `RawSaveBackend` | File-based emulator saves; format auto-detect |
| `Pokemon` | Unified editable Pokémon (generation-aware optional fields) |
| `Box` / `Party` | Slot containers of `Pokemon` (nullable empty slots) |
| `HubStorage` | Persistent multi-box bank on SD; same edit/move semantics as a save |
| `SaveSession` | Open backend + dirty tracking + backup-before-write |
| `TransferService` | Move/copy between any `IBoxProvider` (save ↔ hub ↔ save) |
| `BackupService` | Timestamped copies under `sdmc:/switch/PKHub/backups` |
| `LegalityService` | Soft checks + risk labels (never blocks edit; always warns) |

Detailed declarations live under `include/pkhub/`.

---

## Data Flow: Edit & Save

```
User edits Pokemon in UI
        │
        ▼
Pokemon (in-memory) mutated
        │
SaveSession marks dirty
        │
User taps Save
        │
BackupService::backup(session)     ← always first
        │
ISaveBackend::commit()
        │
Success / failure dialog + legality reminder if online-risk
```

## Data Flow: Hub Transfer

```
Source IBoxProvider ──copy/move──► TransferService ──► Dest IBoxProvider
        │                                                      │
   SaveBackend or Hub                                    SaveBackend or Hub
```

Both sides expose `IBoxProvider` so Hub is not a special-case path in the UI.

---

## Generation Strategy

Parsers live behind `ISaveBackend` and write into the unified `Pokemon` model:

| Gen | Titles | Backend | Notes |
|-----|--------|---------|-------|
| 3 | RSE / FRLG | Raw (GBA) | `.sav` / `.srm`, 128K + RTC variants |
| 4 | DPPt / HGSS | Raw (NDS) | `.sav` / `.dsv` |
| 5 | BW / B2W2 | Raw (NDS) | |
| 6–7 | XY / ORAS / SM / USUM | Raw (3DS) | common Citra / checkpoint dumps |
| 8 | SwSh / BDSP / LA | Switch | title IDs + known offsets |
| 9 | SV (+ DLC) | Switch | |
| 9 | Legends Z-A | **Stub** | `UnsupportedSaveBackend` until format documented |

Phase 1 prioritizes: **SV → SwSh → GBA → Hub**, then expand. Z-A does not block Phase 1.

---

## Safety Model

- **Hard rule:** never write without a successful backup attempt (override requires **confirm**).
- **Online / HOME:** soft banners and legality indicators; do not block edits.
- **Confirm-gated:** disable backups, write without backup, delete non-empty boxes, raw/hex edits (`SafetyPolicy`).
- **Dirty session** prompts on exit / switching games.
- See `docs/DECISIONS.md`.

---

## Threading / Performance

- File I/O and parse on a worker; UI stays on Borealis main thread.
- Sprite atlas / lazy load per box page.
- Large Hub searches use deferred/filter indexes (Phase 2).

---

## Testing Strategy

- Host-compilable parser unit tests under `tests/` (no libnx required).
- Fixture saves (synthetic / redacted) for gen 3–9 round-trips.
- Switch integration smoke: open → view box → edit nick → save → reload.
