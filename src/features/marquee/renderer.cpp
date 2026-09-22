// src/features/marquee/renderer.cpp — the plain-text marquee: scroll math + frame assembly (v3.0 §3.9).
//
// TODO(T4.1): scrollOffset wraps at textWidth + bandWidth (the text scrolls fully off before re-entering).
// TODO(T4.2): buildFrame assembles ONE string — cursor-home, the chrome rows, the band row at kBandRow, and
// the prompt row LAST. There is no glyph table and no FrameBuffer: the band is one row of scrolling text
// (D2, D6), so this file is scroll arithmetic plus string assembly and nothing else.
// TODO(T4.3): sliceRow is ALWAYS exactly bandWidth bytes; tight terminals drop chrome from the bottom up but
// never drop the band or the prompt row, and no line may ever exceed `cols` (§3.9).
#include "csopesy/renderer.hpp"

namespace csopesy {

int scrollOffset(long long, int, int) {
  // TODO(T4.1): one column per frame, in [0, textWidth + bandWidth).
  return 0;
}

std::string sliceRow(std::string_view, int bandWidth, int) {
  // TODO(T4.3): slice + pad. The width invariant (§3.9) already holds here: exactly bandWidth bytes.
  if (bandWidth > 0) {
    return std::string(static_cast<unsigned>(bandWidth), ' ');
  }
  return {};
}

std::string Renderer::buildFrame(const Parameters&, const MarqueeProcess&, const std::string&,
                                 const std::string&, const std::string&, int, int) const {
  // TODO(T4.2): band row, chrome rows, prompt row last; every line clipped to cols so nothing ever wraps.
  return {};
}

int Renderer::bandWidthFor(int cols) {
  // TODO(T4.2): max(1, cols - 2) — one space of margin on each side of the band.
  return cols > 2 ? cols - 2 : 1;
}

}  // namespace csopesy
