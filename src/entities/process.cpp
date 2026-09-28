// src/entities/process.cpp — process state changes.
//
// Starting a process resets hasRendered so the next tick draws a frame immediately.
// Stopping a process only changes its state.
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
