# Phase 1 progress

## Done

- XITRIX Borealis submodule at `libs/borealis`
- Hub persistence: `hub.json` + `.pkbox` (codec round-trip tested)
- Real backups for raw saves / hub directories; Switch marker JSON stub
- Clean-room **GBA Gen 3** decode + **write-back**
- Desktop path remap (`sdmc:/…` → `./pkhub_data/…` or `$PKHUB_DATA_ROOT`)
- UI shell (compiled when `-DPKHUB_ENABLE_UI=ON`)
- **Switch save mount** — title override first, then `fsOpen_SaveData` / user; desktop fixtures under `switch_saves/<titleId>/main`
- SV/SwSh/etc. **mount + empty box scaffold** (Pokémon decrypt/parse still TODO)

## Build

```bash
cmake -B build/desktop -DPLATFORM_DESKTOP=ON -DPKHUB_ENABLE_UI=OFF
cmake --build build/desktop -j && ctest --test-dir build/desktop --output-on-failure
```

Desktop Switch fixture testing:

```bash
# Place a dumped `main` save at:
#   ./pkhub_data/switch_saves/<16-hex-title-id>/main
# Optional: PKHUB_TITLE_OVERRIDE=1 to exercise override path
```

## Still next

- RetroArch path scanner + file browser (see open PR if not merged)
- Full SV / SwSh Pokémon decrypt & serialize (SwishCrypto / gen8 crypto)
- Richer editor fields (moves, nature, ability, EVs/IVs editors)
- NDS / 3DS raw parsers
