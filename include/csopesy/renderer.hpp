// include/csopesy/renderer.hpp — frozen contract, v3.0 §3.9.
//
// v3.0 DELTA (T0.6, ratified 2026-09-22): the marquee is PLAIN TEXT (D2), so there is no glyph table, no
// `artWidthFor`, no `composeBand` and no FrameBuffer. `drawFrame(FrameBuffer&, ...)` becomes
// `buildFrame(...) -> std::string`: ONE complete frame, assembled by the marquee thread and handed to
// `Terminal::write()` as a single string (D6). A full rebuild every frame is also what removes the v2
// "invalidation is not submission" defect class: a resize needs no flag, only the next frame (§3.8).
#pragma once
#include <string>
#include <string_view>
#include "csopesy/parameters.hpp"
#include "csopesy/process.hpp"
namespace csopesy {
// Pure scroll math. `offset` is the band's left edge expressed in TEXT coordinates shifted by one band
// width: windowLeft = offset - bandWidth. So offset == 0 draws an all-blank band (the text has just left)
// and offset == bandWidth puts text[0] in the band's first column. The wrap period is textWidth + bandWidth
// — the text scrolls FULLY OFF before it re-enters, which is what makes the band read as a marquee rather
// than an endless smear.
int scrollOffset(long long cycles, int textWidth, int bandWidth);   // result is in [0, textWidth + bandWidth)

// The band's window into `text`. ALWAYS exactly `bandWidth` bytes, padded with spaces. Unit-tested directly:
// this width invariant is the one the whole layout depends on (§3.9).
std::string sliceRow(std::string_view text, int bandWidth, int offset);

class Renderer {
 public:
  // ONE complete frame, ready for ONE write(): cursor-home, every chrome row padded to `cols`, the band row,
  // then the prompt row LAST. `rows`/`cols` come from `Terminal::size()`, read by the caller — never under
  // `mu_` (§3.8 rule 3). Tight terminals drop chrome from the bottom up; the band and prompt rows are never
  // dropped (§3.9).
  std::string buildFrame(const Parameters&, const MarqueeProcess&, const std::string& prompt,
                         const std::string& buffer, const std::string& message,
                         int rows, int cols) const;

  static int bandWidthFor(int cols);            // max(1, cols - 2): one space of margin on each side
  static constexpr int kBandRow = 3;            // FIXED, near the top (professor answer #5 / D3)
};
}
