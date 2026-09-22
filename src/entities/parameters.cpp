// src/entities/parameters.cpp — clamping that never throws, plus the ASCII text contract (§3.5, §T2.1).
// TODO(T2.1): setRefresh/setPolling clamp into [min, max] and fill in the ClampReport; setText trims, keeps
// internal space runs, and returns Empty or NonAscii with the state unchanged — it never normalizes, because
// mangling the operator's text would be worse than refusing it. defaults() is the built-in precedence layer.
#include "csopesy/parameters.hpp"

namespace csopesy {

ClampReport Parameters::setRefresh(int) {
  // TODO(T2.1): clamp to [kRefreshMin, kRefreshMax] and fill in the ClampReport.
  return {};
}

ClampReport Parameters::setPolling(int) {
  // TODO(T2.1): clamp to [kPollMin, kPollMax] and fill in the ClampReport.
  return {};
}

TextResult Parameters::setText(const std::string&) {
  // TODO(T2.1): trim; empty -> Empty; any byte outside [0x20, 0x7E] -> NonAscii; else assign and return Ok.
  // ASCII-only is what makes "one byte == one column" true for the renderer.
  return TextResult::Empty;
}

Parameters Parameters::defaults() {
  // The default member initializers are the built-in layer, so an empty body is correct.
  return {};
}

}  // namespace csopesy
