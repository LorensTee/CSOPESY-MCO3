// src/features/marquee/glyphs.cpp — the block-text glyph table and rasterizer.
// TODO(T4.1): the full 5x5 table (~45 more cells), the fallback rule, and the 1-column inter-glyph gap.
#include "csopesy/glyphs.hpp"

namespace csopesy {

const Glyph* glyphFor(char) {
  // TODO(T4.1): the full glyph table. Until then every character resolves to the box fallback, so the
  // frozen invariant "never nullptr (unknown -> box glyph)" (§3.9) already holds for any caller.
  static const Glyph kFallback{{"#####", "#   #", "#   #", "#   #", "#####"}};
  return &kFallback;
}

std::vector<std::string> renderBlockText(std::string_view) {
  // TODO(T4.1): one string per cell row, glyphs joined with a single-column gap.
  return std::vector<std::string>(kCellRows);
}

}  // namespace csopesy
