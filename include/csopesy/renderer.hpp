// include/csopesy/renderer.hpp — marquee layout and frame construction.
//
// buildFrame() creates one complete frame for the current terminal size.
// The frame is written as one string. The renderer uses plain text and rebuilds the frame after each update,
// so a resize does not need separate frame state.
#pragma once
#include <string>
#include <string_view>
#include "csopesy/parameters.hpp"
#include "csopesy/process.hpp"
namespace csopesy {

// Calculate the marquee position.
// offset 0 shows a blank band. offset bandWidth shows the first text character.
// The position wraps after the text and the blank band have both moved across the screen.
int scrollOffset(long long cycles, int textWidth, int bandWidth);

// Return one band row with exactly bandWidth bytes.
// Pad the row with spaces when the text does not fill the band.
std::string sliceRow(std::string_view text, int bandWidth, int offset);

class Renderer {
 public:
  // Build one complete frame for rows x cols.
  // The frame contains the marquee band, fixed text, message rows, and the prompt row.
  // The band and prompt stay visible on short terminals. Lower-priority fixed text is dropped first.
  std::string buildFrame(const Parameters&, const MarqueeProcess&, const std::string& prompt,
                         const std::string& buffer, const std::string& message,
                         int rows, int cols) const;

  // Return the marquee width while keeping one column of margin on each side.
  static int bandWidthFor(int cols);

  // Row used for the marquee on a normal terminal.
  static constexpr int kBandRow = 3;
};

}
