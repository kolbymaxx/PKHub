#pragma once

#if defined(PKHUB_HAS_BOREALIS)

#include <borealis.hpp>

namespace pkhub::ui::theme {

// HOME / PC companion palette — forest mint, not purple or cream-serif.
inline NVGcolor bg() { return nvgRGB(8, 14, 18); }
inline NVGcolor bgPanel() { return nvgRGBA(14, 24, 30, 235); }
inline NVGcolor bgWallpaper() { return nvgRGB(16, 38, 42); }
inline NVGcolor bgWallpaperHi() { return nvgRGB(22, 52, 56); }
inline NVGcolor accent() { return nvgRGB(72, 186, 154); }
inline NVGcolor accentDim() { return nvgRGBA(72, 186, 154, 100); }
inline NVGcolor text() { return nvgRGB(236, 246, 240); }
inline NVGcolor textMuted() { return nvgRGBA(140, 190, 170, 230); }
inline NVGcolor textFaint() { return nvgRGBA(100, 140, 125, 200); }
inline NVGcolor slotEmpty() { return nvgRGBA(12, 22, 26, 180); }
inline NVGcolor slotFill() { return nvgRGBA(18, 32, 38, 240); }
inline NVGcolor slotBorder() { return nvgRGBA(70, 150, 125, 110); }
inline NVGcolor shiny() { return nvgRGB(245, 208, 78); }
inline NVGcolor partyStrip() { return nvgRGBA(10, 20, 24, 245); }
inline NVGcolor rule() { return nvgRGBA(72, 186, 154, 90); }

}  // namespace pkhub::ui::theme

#endif
