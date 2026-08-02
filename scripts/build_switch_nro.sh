#!/usr/bin/env bash
# Build PKHub.nro inside the official DevKitPro Docker image.
# Always bumps the build number unless PKHUB_SKIP_BUMP=1.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
IMAGE="${PKHUB_DKP_IMAGE:-devkitpro/devkita64:latest}"

if [[ "${PKHUB_SKIP_BUMP:-0}" != "1" ]]; then
  echo "Incrementing build number…"
  "$ROOT/scripts/bump_build.sh"
fi

LABEL="$(sed -n 's/^constexpr const char\* kAppVersion = "\([^"]*\)";$/\1/p' "$ROOT/include/pkhub/app/Version.hpp")"
echo "Packaging PKHub ${LABEL}"

TARGET="$ROOT/libs/borealis/library/lib/platforms/switch/switch_input.cpp"
if [[ -f "$TARGET" ]] && ! grep -q 'screenshot_button_thread_fn(token)' "$TARGET"; then
  echo "Applying borealis GCC15 jthread patch…"
  python3 - "$TARGET" <<'PY'
import sys
from pathlib import Path
p = Path(sys.argv[1])
old = "    this->screenshot_button_thread = std::jthread(&SwitchInputManager::screenshot_button_thread_fn, this);"
new = """    // GCC 15 / libstdc++: prefer stop_token lambda over PMF+this jthread ctor.
    this->screenshot_button_thread = std::jthread([this](std::stop_token token) {
        this->screenshot_button_thread_fn(token);
    });"""
t = p.read_text()
if old in t:
    p.write_text(t.replace(old, new, 1))
    print("patched")
else:
    print("already patched or unexpected content")
PY
fi

docker run --rm \
  -e DEVKITPRO=/opt/devkitpro \
  -v "$ROOT:/workspace" \
  -w /workspace \
  "$IMAGE" \
  bash -lc '
    set -euo pipefail
    source /opt/devkitpro/switchvars.sh
    export PATH="/opt/devkitpro/tools/bin:/opt/devkitpro/devkitA64/bin:/opt/devkitpro/portlibs/switch/bin:$PATH"
    export PORTLIBS_PREFIX=/opt/devkitpro/portlibs/switch
    rm -rf build/switch
    cmake -B build/switch \
      -DPLATFORM_SWITCH=ON \
      -DPLATFORM_DESKTOP=OFF \
      -DPKHUB_ENABLE_UI=ON \
      -DPKHUB_BUNDLE_BOREALIS=ON \
      -DPKHUB_BUILD_TESTS=OFF \
      -DBRLS_UNITY_BUILD=OFF \
      -DFMT_OS=OFF \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/Switch.cmake
    cmake --build build/switch -j"$(nproc)"
    ls -lh build/switch/PKHub.nro build/switch/PKHub.elf
  '

mkdir -p "$ROOT/dist" /opt/cursor/artifacts
cp -f "$ROOT/build/switch/PKHub.nro" "$ROOT/dist/PKHub.nro"
cp -f "$ROOT/build/switch/PKHub.nro" /opt/cursor/artifacts/PKHub.nro
ls -lh "$ROOT/dist/PKHub.nro" /opt/cursor/artifacts/PKHub.nro
echo "NRO ready: $ROOT/dist/PKHub.nro (${LABEL})"
