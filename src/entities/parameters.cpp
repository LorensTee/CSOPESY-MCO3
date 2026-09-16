// src/entities/parameters.cpp — one struct, layered precedence, clamping that never throws (§3.5, §T2.1).
// TODO(T2.1): setRefresh/setPolling clamp into [min, max] and report; setText trims, and returns false
// (state unchanged) when the result is empty; defaults() is the §3.6 built-in layer.
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

bool Parameters::setText(const std::string&) {
  // TODO(T2.1): trim; internal whitespace runs are preserved verbatim (§3.7 rule 4).
  return false;
}

Parameters Parameters::defaults() {
  // Default member initializers already are the built-in layer (§3.5), so this is correct as written.
  // TODO(T2.1): keep this in sync if a default ever moves to the config layer.
  return {};
}

}  // namespace csopesy
