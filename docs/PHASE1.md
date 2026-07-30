# Phase 1 progress

## Done

- XITRIX Borealis submodule at `libs/borealis`
- Hub persistence: `hub.json` + `.pkbox` (codec round-trip tested)
- Real backups for raw saves / hub directories; Switch marker JSON stub
- Clean-room **GBA Gen 3** decode (`parseSave` / `decodeBoxMon` / `encodeBoxMon`)
- **GBA Gen 3 write-back** (`writeSave` / `encodePartyMon` / `RawSaveBackend::commit`)
- Desktop path remap (`sdmc:/…` → `./pkhub_data/…` or `$PKHUB_DATA_ROOT`)
- UI shell (compiled when `-DPKHUB_ENABLE_UI=ON`): home tabs, box grid placeholders, core editor (shiny / level), save button via `SafetyPolicy`
- Switch title list + Z-A stub; Switch backend desktop placeholders (device mount next)

## Build

```bash
# Logic + tests (default in CI / minimal images)
cmake -B build/desktop -DPLATFORM_DESKTOP=ON -DPKHUB_ENABLE_UI=OFF
cmake --build build/desktop -j && ctest --test-dir build/desktop --output-on-failure

# Desktop UI (install SDL3/X11 or Wayland deps first — see docs/BOREALIS_SETUP.md)
cmake -B build/ui -DPLATFORM_DESKTOP=ON -DPKHUB_ENABLE_UI=ON
cmake --build build/ui -j
```

## Still next

- RetroArch path scanner + file browser activity
- libnx title-override / `fsOpen_SaveData` parse for SV (and SwSh)
- Richer editor fields (moves, nature, ability, EVs/IVs editors)
- Wire UI against real graphics stack on a desktop with X11/Wayland dev packages
