# Main Navigation Flow

## Top-level: Home Activity

Three primary destinations (tab bar or large home tiles — Borealis `TabFrame` / custom home):

```
┌──────────────────────────────────────────────────────────┐
│  PKHub                                                    │
│                                                          │
│   [ Games ]   [ Files ]   [ Hub ]                          │
│                                                          │
│   Recent sessions · Safety tip strip (non-hero)           │
└──────────────────────────────────────────────────────────┘
```

| Entry | Purpose |
|-------|---------|
| **Switch Games** | Detect installed / known official titles; open via libnx save APIs |
| **Files** | Auto-scan RetroArch (+ common paths) and manual file browser |
| **Hub** | Persistent bank; always available offline |

Secondary (overflow / settings gear): Backups, Settings, About, Legality info.

---

## Title override / save mount

1. **Title override (preferred)** — most reliable full save access.
2. **`fsOpen_SaveData` + user picker** — cleaner when it works without override.

`SaveAccessMode::Auto` tries override context first, then FsSaveData.

## Flow A — Switch Games

```
Home → Games
  → Game list (SV, SwSh, LA, BDSP, …; Z-A shown as stub)
  → Access mode hint (title override recommended)
  → User/account picker (when using FsSaveData)
  → Mount save (SwitchSaveBackend)  OR  show “format not yet documented” for Z-A
  → Save Workspace (boxes / party / editor)
```

## Flow B — Emulator Saves

```
Home → Files
  → Tabs: Detected | Browse
  → Detected: RetroArch cores paths (GBA → NDS → 3DS priority)
  → Browse: FileBrowserActivity (filter .sav .srm .dsv …)
  → Format detect → RawSaveBackend
  → Save Workspace
```

## Flow C — Hub Storage

```
Home → Hub Storage
  → Hub boxes grid (create / rename / reorder in Phase 2)
  → Same box UI as game saves
  → Transfer mode: pick destination save session or stay in-hub
```

---

## Save Workspace (shared)

Once a backend (or Hub) is open:

```
┌──────────── TabFrame ────────────────────────────────────┐
│  Boxes  │  Party  │  Editor*  │  Bag†  │  Transfer  │ … │
└──────────────────────────────────────────────────────────┘
  Boxes:    box selector + 30-slot grid (sprites)
  Party:    up to 6 slots
  Editor:   opened from a selected Pokémon (species, shiny, IVs, …)
  Transfer: source/dest picker (current save ↔ Hub ↔ other open save)
  Bag:      Phase 2 where applicable

* Editor may be a pushed Activity rather than a tab.
† Phase 2+
```

Controls (target):

- **A** select / edit  
- **B** back  
- **X** move / grab for transfer  
- **Y** quick shiny toggle or details (TBD)  
- **L/R** prev/next box  
- **ZL/ZR** jump party / Hub  
- Touch: tap slots, swipe boxes  

---

## Session switching

Only one *writable* game save session is active by default (simplifies backup/dirty state). Hub can stay open alongside. Opening another game:

1. If dirty → save / discard / cancel  
2. Close previous backend  
3. Open new backend  

Phase 2 may allow dual-pane compare (optional).

---

## Safety UX placement

- Soft banners for legality / HOME risk (never block edits by default).
- Confirm modals only for brick / extreme risk (`SafetyPolicy`).
- Backup success snackbar after save.
- Placeholder colored slots until real sprites ship.
