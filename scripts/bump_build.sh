#!/usr/bin/env bash
# Increment PKHUB_BUILD_NUMBER and regenerate Version.hpp.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FILE="$ROOT/cmake/PkhubVersion.cmake"
HDR="$ROOT/include/pkhub/app/Version.hpp"

if [[ ! -f "$FILE" ]]; then
  echo "missing $FILE" >&2
  exit 1
fi

current="$(sed -n 's/^set(PKHUB_BUILD_NUMBER \([0-9]*\))$/\1/p' "$FILE" | head -1)"
if [[ -z "$current" ]]; then
  echo "could not parse PKHUB_BUILD_NUMBER" >&2
  exit 1
fi

next=$((current + 1))
tmp="$(mktemp)"
sed "s/^set(PKHUB_BUILD_NUMBER ${current})$/set(PKHUB_BUILD_NUMBER ${next})/" "$FILE" >"$tmp"
mv "$tmp" "$FILE"

major=$(sed -n 's/^set(PKHUB_VERSION_MAJOR \([0-9]*\))$/\1/p' "$FILE")
minor=$(sed -n 's/^set(PKHUB_VERSION_MINOR \([0-9]*\))$/\1/p' "$FILE")
patch=$(sed -n 's/^set(PKHUB_VERSION_PATCH \([0-9]*\))$/\1/p' "$FILE")
channel=$(sed -n 's/^set(PKHUB_VERSION_CHANNEL "\([^"]*\)")$/\1/p' "$FILE")
label="${major}.${minor}.${patch}-${channel}.${next}"

cat >"$HDR" <<EOF
#pragma once

namespace pkhub {

constexpr const char* kAppName = "PKHub";
/// Display / NACP version — generated from cmake/PkhubVersion.cmake.
constexpr const char* kAppVersion = "${label}";
constexpr int kAppBuildNumber = ${next};

}  // namespace pkhub
EOF

echo "Bumped build ${current} → ${next} (${label})"
echo "$label"
