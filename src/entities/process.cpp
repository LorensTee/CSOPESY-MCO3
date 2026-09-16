// src/entities/process.cpp — the PCB (§3.8, §T1.2).
// TODO(T1.2): start() is false when already Running and clears hasRendered; stop() is false when already
// Stopped. hasRendered == false makes the next tick draw immediately (fresh OR restarted).
#include "csopesy/process.hpp"

namespace csopesy {

bool MarqueeProcess::start() {
  // TODO(T1.2)
  return false;
}

bool MarqueeProcess::stop() {
  // TODO(T1.2)
  return false;
}

}  // namespace csopesy
