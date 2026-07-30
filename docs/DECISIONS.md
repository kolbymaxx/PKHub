# Project Decisions (locked)

Answers from product owner — Phase 1+ must follow these.

| # | Topic | Decision |
|---|--------|----------|
| 1 | Legends: Z-A | **Stub** until format is documented. Placeholder backend returns “format not yet documented”. Do not block Phase 1. |
| 2 | Legality / risk | **Soft warnings** always. **Confirm dialog only** for extremely high-risk / save-brick actions. Power users may still create anything. |
| 3 | Sprites | **Placeholders first** (colored boxes / generic icons). Real sprites later (extracted or downloadable pack). |
| 4 | Borealis | Prefer **XITRIX/borealis** (community Switch recommendation). Note: xfangfang remains a compatible, highly maintained sibling fork if we need desktop/CMake examples. |
| 5 | Save access | Support **both**; prioritize **title override** for reliability; also implement `fsOpen_SaveData` + user picker. |
| 6 | Hub format | **Versioned binary** Pokémon data + **JSON** metadata (box names, notes, tags). |
| 7 | Desktop UI | **Yes** — desktop Borealis target is strongly preferred for iteration. |
| 8 | License | **GPLv3**. |
| 9 | Reference code | Clean-room reimplementation of public formats / ideas OK. **Do not copy** PKHeX or PKSM source. |

## Still open (non-blocking)

- Phase 1 UI language: default English; keep Borealis i18n wired for later.
- RetroArch scan roots (proposed defaults in QUESTIONS.md — treat as accepted unless overridden).
- Non-goals accepted: no cloud sync, no online legality API, no ROM-hacking automation in-app.
