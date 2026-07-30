# Main Navigation Flow

## Top-level: Home Activity

Three primary destinations (tab bar or large home tiles — Borealis `TabFrame` / custom home):

```
┌──────────────────────────────────────────────────────────┐
│  PKHub                                                    │
│                                                          │
│   [ Switch Games ]   [ Emulator Saves ]   [ Hub Storage ] │
│                                                          │
│   Recent sessions · Safety tip strip (non-hero)           │
└──────────────────────────────────────────────────────────┘
```

| Entry | Purpose |
|-------|---------|
| **Switch Games** | Detect installed / known official titles; open via libnx save APIs |
| **Emulator Saves** | Auto-scan RetroArch (+ common paths) and manual file browser |
| **Hub Storage** | Persistent bank; always available offline |

Secondary (overflow / settings gear): Backups, Settings, About, Legality info.

---

## Flow A — Switch Games

```
Home → Switch Games
  → Game list (SV, ZA, SwSh, LA, BDSP, …)  [installed badge if detectable]
  → User/account picker (if needed)
  → Mount save (SwitchSaveBackend)
  → Auto-backup prompt policy
  → Save Workspace (boxes / party / editor)
```

## Flow B — Emulator Saves

```
Home → Emulator Saves
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

- Banner on Switch game workspace: “Editing this save can affect online / HOME use.”
- Modal before first write per session.
- Backup success snackbar after save.
