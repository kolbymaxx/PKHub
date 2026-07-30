# Convenience wrappers — prefer CMake directly.
# Host smoke:
#   make host
# Switch (requires DEVKITPRO + borealis):
#   make switch

.PHONY: host switch test clean

host:
	cmake -B build/host -DPLATFORM_SWITCH=OFF -DPKHUB_BUILD_TESTS=ON
	cmake --build build/host -j
	ctest --test-dir build/host --output-on-failure

switch:
	cmake -B build/switch -DPLATFORM_SWITCH=ON \
		-DCMAKE_TOOLCHAIN_FILE="$(DEVKITPRO)/cmake/Switch.cmake"
	cmake --build build/switch -j

test: host

clean:
	rm -rf build
