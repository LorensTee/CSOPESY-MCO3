// src/shared/terminal/frame_buffer.cpp — one assembled frame per write() (§3.9, §T4.3).
// TODO(T4.3): renderDiff() returns ONE string; invalidate() forces a full repaint after resize/clear.
// Private members arrive with T4.3 — the public API in frame_buffer.hpp is the frozen part.
#include "csopesy/frame_buffer.hpp"

namespace csopesy {

void FrameBuffer::resize(int, int) { /* TODO(T4.3) */ }

void FrameBuffer::put(int, int, char) { /* TODO(T4.3) */ }

void FrameBuffer::putRow(int, int, std::string_view) { /* TODO(T4.3) */ }

std::string FrameBuffer::renderDiff() {
  // TODO(T4.3)
  return {};
}

void FrameBuffer::invalidate() { /* TODO(T4.3) */ }

}  // namespace csopesy
