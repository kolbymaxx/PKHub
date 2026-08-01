# BDSP / Legends: Arceus save notes (clean-room)

PKHub mounts BDSP and LA saves today, but **entity parse/serialize is not implemented yet**.

## Constraints

- Clean-room only — do **not** copy PKHeX / PKSM source or proprietary dumps of their layouts.
- Prefer publicly documented layouts, community write-ups that state offsets explicitly, or original reverse engineering from dumps you own.

## Known product facts (safe)

| Title | Boxes | Slot size (party) | Notes |
|-------|-------|-------------------|--------|
| BDSP | 40 × 30 | PB8-class (~0x158) | Flat / Unity-era save; not SwishCrypto block store |
| LA | 32 × 30 | PA8 | Different entity layout from PK8/PK9 |
| SwSh / SV | 32 × 30 | PK8 / PK9 | **Implemented** via SwishCrypto |

## Implementation plan

1. Document verified box/party base offsets from owned dumps + public sources.
2. Reuse `PokeCrypto8` where BDSP encryption matches Gen 8 shuffle/XOR (verify before assuming).
3. Add `Pa8Codec` for LA once PA8 field map is clean-room documented.
4. Wire `parseSwitchSave` / `serializeSwitchSave` like SwSh/SV paths.

Until then, mount succeeds and the UI shows empty boxes with an informative status message.
