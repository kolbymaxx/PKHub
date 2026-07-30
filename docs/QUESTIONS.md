# Clarifying Questions (before large implementation)

Please answer these so Phase 1 code stays aligned with your intent.

## Product / scope

1. **Legends Z-A** — Is save-format research in-scope for Phase 1, or should we stub the game entry until offsets are known?
2. **HOME / Legality** — Prefer soft warnings only in Phase 1, or block “risky” writes behind an explicit confirm every time?
3. **Sprites** — Ship a compressed sprite pack in RomFS, download once to SD, or generate placeholders until an asset pipeline exists?
4. **Languages** — English-only for Phase 1 UI, or multi-lang from day one (Borealis i18n is easy to wire early)?

## Technical

5. **Borealis fork** — Confirm **xfangfang/borealis** as default, or do you already depend on **XITRIX/borealis** elsewhere?
6. **Title IDs / save mounting** — Prefer title-override workflow only, or full `fsOpen_SaveData` by title ID + user selector?
7. **Hub format** — Custom binary (`.pkhub`) vs JSON+binary blobs vs SQLite? (Recommendation: versioned custom binary + JSON sidecar metadata.)
8. **PC build** — Should CMake also support a desktop Borealis target for faster UI iteration?
9. **Licensing** — Intended license for PKHub (GPLv3 like many homebrew tools, MIT, etc.)? Affects submodule/code reuse.
10. **Reference code** — OK to study PKSM / PKHeX algorithms as reference (reimplemented clean-room in C++), or any code we must avoid?

## Emulator paths

11. Priority RetroArch roots on Switch SD — confirm these defaults:
    - `/retroarch/cores/savefiles/`
    - `/retroarch/saves/`
    - `/switch/Checkpoint/saves/` (3DS dumps sometimes mirrored)
    - Manual browser from `/` and `/emu/`

## Non-goals (please confirm)

12. No cloud sync, no online legality API, no ROM hacking / shiny hunting automation in PKHub itself — correct?
