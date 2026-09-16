// include/csopesy/frame_buffer.hpp — §3.9. Private members are added by T4.3 (implementation),
// not frozen here; the public API below is the contract.
#pragma once
#include <string>
#include <string_view>
#include <vector>
namespace csopesy {
class FrameBuffer {
 public:
  void resize(int rows, int cols);
  void put(int row, int col, char c);            // 1-based
  void putRow(int row, int col, std::string_view s);
  std::string renderDiff();                      // ONE string; the caller issues ONE write()
  void invalidate();                             // force full repaint (after resize/clear)
};
}
