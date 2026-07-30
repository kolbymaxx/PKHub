# Third-party libraries

## Borealis (required for UI)

**Default (locked):** [XITRIX/borealis](https://github.com/XITRIX/borealis) branch `moonlight_wiliwili`

```bash
git submodule add -b moonlight_wiliwili https://github.com/XITRIX/borealis.git libs/borealis
git submodule update --init --recursive
```

Alternate reference (desktop/CMake examples): [xfangfang/borealis](https://github.com/xfangfang/borealis)

See `docs/BOREALIS_SETUP.md` and `docs/DECISIONS.md`.

## Planned header-only deps

- `nlohmann/json` — config + Hub `hub.json` metadata
