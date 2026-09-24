// src/features/marquee/renderer.cpp — the plain-text marquee: scroll math + frame assembly (§3.9).
#include "csopesy/renderer.hpp"

#include <string>

namespace csopesy {

int scrollOffset(long long cycles, int textWidth, int bandWidth) {
  const long long period = static_cast<long long>(textWidth) + bandWidth;
  if (period <= 0) return 0;   // empty text in an empty band: there is nothing to move
  const long long wrapped = cycles % period;
  return static_cast<int>(wrapped < 0 ? wrapped + period : wrapped);
}

std::string sliceRow(std::string_view text, int bandWidth, int offset) {
  if (bandWidth <= 0) return {};
  // offset is the band's left edge in text coordinates shifted by one band width, so offset == 0 is one whole
  // band past the text's right edge (a blank band) and offset == bandWidth lands text[0] in column 0.
  const int windowLeft = offset - bandWidth;
  const int textWidth = static_cast<int>(text.size());
  std::string row;
  row.reserve(static_cast<std::size_t>(bandWidth));
  for (int col = 0; col < bandWidth; ++col) {
    const int index = windowLeft + col;
    row.push_back((index >= 0 && index < textWidth) ? text[static_cast<std::size_t>(index)] : ' ');
  }
  return row;
}

std::string Renderer::buildFrame(const Parameters&, const MarqueeProcess&, const std::string&,
                                 const std::string&, const std::string&, int, int) const {
  // TODO(T4.2): band row, chrome rows, prompt row last; every line clipped to cols so nothing ever wraps.
  return {};
}

int Renderer::bandWidthFor(int cols) {
  return cols > 2 ? cols - 2 : 1;   // max(1, cols - 2): one space of margin on each side
}

}  // namespace csopesy
