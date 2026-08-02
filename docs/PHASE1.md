# Phase 1 progress

## Done

- XITRIX Borealis submodule at `libs/borealis`
- Hub persistence: `hub.json` + `.pkbox` (codec round-trip tested)
- Real backups for raw saves / hub directories; **Switch `main` byte backup** + marker JSON
- Clean-room **GBA Gen 3** decode + write-back
- **RetroArch / emulator save scan** + **file browser** (`.sav` / `.srm` / `.dsv`)
- Desktop path remap (`sdmc:/…` and `/…` → `./pkhub_data/…` or `$PKHUB_DATA_ROOT`)
- UI shell: home tabs, box grid, **party**, richer editor (shiny / level / nature / ability / IVs / EVs / moves / tera), Save via `SafetyPolicy`
- **Switch save mount** — title override first, then `fsOpen_SaveData` + **user picker**
- **SwishCrypto + PK8/PK9 parse/serialize** for Sword/Shield/Scarlet/Violet (empty-box scaffold for BDSP/LA until their formats land)
- **National Dex sprites** (normal + shiny `#1`–`#1025`) + SV internal→national remap (`SpeciesIds` / `SpriteService`)
- **English species + nature name tables** (PokeAPI / official nature order) in box, party, editor
- Polished Borealis UI (teal night theme, DetailCell lists, box/party/editor sprite slots)
- Switch `.nro` packaging via `scripts/build_switch_nro.sh` (Docker DevKitPro)
- **0.1.0-beta** homebrew package (`dist/PKHub.nro`) with app icon + branded home tabs
- Z-A remains stubbed

## Build

```bash
cmake -B build/desktop -DPLATFORM_DESKTOP=ON -DPKHUB_ENABLE_UI=OFF
cmake --build build/desktop -j && ctest --test-dir build/desktop --output-on-failure
```

Switch `.nro` (Docker / DevKitPro):

```bash
./scripts/build_switch_nro.sh
# → build/switch/PKHub.nro
```

Desktop Switch fixture testing:

```bash
# Place a dumped `main` save at:
#   ./pkhub_data/switch_saves/<16-hex-title-id>/main
# Optional: PKHUB_TITLE_OVERRIDE=1 to exercise override path
```

## Still next

- BDSP / Legends Arceus entity parsers (see [FORMATS_BDSP_LA.md](FORMATS_BDSP_LA.md))
- Move / ability English name tables
- NDS / 3DS raw parsers
- Wire UI against real graphics stack on a desktop with X11/Wayland dev packages
