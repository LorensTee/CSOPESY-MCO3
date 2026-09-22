// src/entities/process.cpp — the PCB (§T1.2).
// hasRendered == false makes the next tick draw immediately; clearing it here is what makes a restarted
// marquee repaint on its first tick instead of waiting out a stale deadline.
#include "csopesy/process.hpp"

namespace csopesy {

bool MarqueeProcess::start() {
  if (state == ProcessState::Running) {
    return false;
  }
  state = ProcessState::Running;
  hasRendered = false;
  return true;
}

bool MarqueeProcess::stop() {
  if (state == ProcessState::Stopped) {
    return false;
  }
  state = ProcessState::Stopped;
  return true;
}

}  // namespace csopesy
