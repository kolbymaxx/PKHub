.PHONY: desktop switch host test clean ui

desktop:
	cmake -B build/desktop -DPLATFORM_DESKTOP=ON -DPLATFORM_SWITCH=OFF -DPKHUB_ENABLE_UI=OFF
	cmake --build build/desktop -j
	ctest --test-dir build/desktop --output-on-failure

ui:
	cmake -B build/ui -DPLATFORM_DESKTOP=ON -DPLATFORM_SWITCH=OFF -DPKHUB_ENABLE_UI=ON
	cmake --build build/ui -j

host: desktop

switch:
	cmake -B build/switch -DPLATFORM_SWITCH=ON -DPLATFORM_DESKTOP=OFF \
		-DCMAKE_TOOLCHAIN_FILE="$(DEVKITPRO)/cmake/Switch.cmake"
	cmake --build build/switch -j

test: desktop

clean:
	rm -rf build
