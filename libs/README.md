# Third-party libraries

## Borealis (required for UI)

```bash
git submodule add https://github.com/xfangfang/borealis.git libs/borealis
git submodule update --init --recursive
```

Alternate (XITRIX fork used by Moonlight-Switch):

```bash
git submodule add https://github.com/XITRIX/borealis.git libs/borealis
```

See `docs/BOREALIS_SETUP.md`.

## Planned header-only deps

- `nlohmann/json` — config + hub metadata (add under `libs/json` or FetchContent)
