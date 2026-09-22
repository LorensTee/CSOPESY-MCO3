// src/entities/process.cpp — the PCB (§T1.2).
// TODO(T1.2): start() returns false when already Running and clears hasRendered; stop() returns false when
// already Stopped. hasRendered == false makes the next tick draw immediately.
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
