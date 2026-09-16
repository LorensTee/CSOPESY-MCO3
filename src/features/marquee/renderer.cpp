// src/features/marquee/renderer.cpp — glyph layout, scroll math, band composition, header region.
// TODO(T4.2/T4.4/T4.5): scrollOffset wraps at artWidth + bandWidth; sliceRow output is always exactly
// bandWidth columns; the header rows are repainted only when their text changes (§3.9).
#include "csopesy/renderer.hpp"

namespace csopesy {

int scrollOffset(long long, int, int) {
  // TODO(T4.2): one column per frame, wrapping at artWidth + bandWidth.
  return 0;
}

std::string sliceRow(std::string_view, int bandWidth, int) {
  // TODO(T4.2): slice + pad. The width invariant (§3.9) already holds here: exactly bandWidth columns.
  if (bandWidth > 0) {
    return std::string(static_cast<unsigned>(bandWidth), ' ');
  }
  return {};
}

void Renderer::drawFrame(FrameBuffer&, const Parameters&, const MarqueeProcess&, const std::string&,
                         const std::string&, const std::string&) {
  // TODO(T4.4/T4.5): band rows, then the header region, then the one-row prompt (§3.9).
}

int Renderer::artWidthFor(std::string_view) {
  // TODO(T4.1): kCellCols * len + (len - 1).
  return 0;
}

std::vector<std::string> Renderer::composeBand(const Parameters&, const MarqueeProcess&) const {
  // TODO(T4.4): kArtRows art rows, or 1 plain row when !asciiArt.
  return {};
}

}  // namespace csopesy
