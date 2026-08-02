# Single source of truth for app / NACP versioning.
# scripts/bump_build.sh increments PKHUB_BUILD_NUMBER before each packaged NRO.

set(PKHUB_VERSION_MAJOR 0)
set(PKHUB_VERSION_MINOR 1)
set(PKHUB_VERSION_PATCH 0)
set(PKHUB_VERSION_CHANNEL "beta")
# Last packaged NRO build. scripts/bump_build.sh increments this before each NRO.
set(PKHUB_BUILD_NUMBER 3)

# Display / NACP string, e.g. 0.1.0-beta.2
set(PKHUB_VERSION_LABEL
    "${PKHUB_VERSION_MAJOR}.${PKHUB_VERSION_MINOR}.${PKHUB_VERSION_PATCH}-${PKHUB_VERSION_CHANNEL}.${PKHUB_BUILD_NUMBER}")
