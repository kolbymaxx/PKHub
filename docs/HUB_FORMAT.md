# Hub Storage Format

**Decision:** versioned binary for Pokémon payloads + JSON for metadata.

## On-disk layout

```
sdmc:/switch/PKHub/hub/
├── hub.json                 # metadata (version, boxes, notes, tags)
├── data/
│   └── boxes/
│       ├── 0000.pkbox       # binary slot payloads for box 0
│       ├── 0001.pkbox
│       └── …
└── holding.pkbox            # optional transfer holding / “party” area
```

## `hub.json` (metadata)

```json
{
  "formatVersion": 1,
  "appMinVersion": "0.1.0",
  "boxes": [
    {
      "id": 0,
      "name": "Box 1",
      "notes": "",
      "tags": [],
      "slotCount": 30
    }
  ],
  "settings": {
    "defaultView": "grid"
  }
}
```

## `.pkbox` binary (versioned)

Little-endian:

| Offset | Type | Field |
|--------|------|-------|
| 0 | u32 | magic `PKBX` (`0x58424B50`) |
| 4 | u16 | formatVersion (1) |
| 6 | u16 | slotCount |
| 8 | u32 | flags |
| 12 | … | slot records |

Each slot:

| Type | Field |
|------|-------|
| u16 | payloadLength (0 = empty) |
| u8 | generation (3–9) |
| u8 | reserved |
| u16 | species (hint for UI without full parse) |
| u8[payloadLength] | generation-native or unified blob |

Lossless round-trip prefers storing the **native encrypted/decrypted party/box structure** for the origin generation when known; otherwise the unified serialized form.

## Compatibility

- Bump `formatVersion` on breaking changes.
- Readers must reject unknown major versions with a clear error (never silently corrupt).
- JSON may gain fields without bumping binary version.
