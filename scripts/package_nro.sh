#!/usr/bin/env bash
set -euo pipefail
: "${NX_NACPTOOL:?}" "${NX_ELF2NRO:?}" "${OUT_DIR:?}" "${ELF_PATH:?}"
: "${PKHUB_AUTHOR:?}" "${PKHUB_VERSION:?}" "${PKHUB_TITLEID:?}"
: "${PKHUB_RESOURCES:?}" "${BOREALIS_RESOURCES:?}"

mkdir -p "${OUT_DIR}/romfs"
"${NX_NACPTOOL}" --create "PKHub" "${PKHUB_AUTHOR}" "${PKHUB_VERSION}" \
  "${OUT_DIR}/PKHub.nacp" --titleid="${PKHUB_TITLEID}"
test -s "${OUT_DIR}/PKHub.nacp"

# Merge app + Borealis romfs assets (sprites, XML, fonts).
cp -a "${PKHUB_RESOURCES}/." "${OUT_DIR}/romfs/"
cp -a "${BOREALIS_RESOURCES}/." "${OUT_DIR}/romfs/"

"${NX_ELF2NRO}" "${ELF_PATH}" "${OUT_DIR}/PKHub.nro" \
  --nacp="${OUT_DIR}/PKHub.nacp" \
  --romfsdir="${OUT_DIR}/romfs"

ls -lh "${OUT_DIR}/PKHub.nro"
