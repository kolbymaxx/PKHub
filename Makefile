.PHONY: desktop switch host test clean

# Desktop / host (preferred for UI iteration once Borealis is present)
desktop:
	cmake -B build/desktop -DPLATFORM_DESKTOP=ON -DPLATFORM_SWITCH=OFF
	cmake --build build/desktop -j
	ctest --test-dir build/desktop --output-on-failure

host: desktop

switch:
	cmake -B build/switch -DPLATFORM_SWITCH=ON -DPLATFORM_DESKTOP=OFF \
		-DCMAKE_TOOLCHAIN_FILE="$(DEVKITPRO)/cmake/Switch.cmake"
	cmake --build build/switch -j

test: desktop

clean:
	rm -rf build
