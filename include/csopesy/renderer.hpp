// include/csopesy/renderer.hpp — §3.9.
#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "csopesy/parameters.hpp"
#include "csopesy/process.hpp"
#include "csopesy/frame_buffer.hpp"
namespace csopesy {
// Pure scroll math: frame `cycles` puts column scrollOffset(...) at band column 0.
int scrollOffset(long long cycles, int artWidth, int bandWidth);
std::string sliceRow(std::string_view artRow, int bandWidth, int offset);  // pads with spaces

class Renderer {
 public:
  void drawFrame(FrameBuffer&, const Parameters&, const MarqueeProcess&,
                 const std::string& prompt, const std::string& buffer, const std::string& message);
  static int artWidthFor(std::string_view text);         // == kCellCols*len + (len-1)
  std::vector<std::string> composeBand(const Parameters&, const MarqueeProcess&) const;
};
}
