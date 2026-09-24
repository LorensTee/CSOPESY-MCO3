// src/entities/parameters.cpp — clamping that never throws, plus the ASCII text contract (§3.5, §T2.1).
// setRefresh/setPolling clamp into [min, max] and fill in the ClampReport; setText trims, keeps internal
// space runs, and returns Empty or NonAscii with the state unchanged — it never normalizes, because mangling
// the operator's text would be worse than refusing it. defaults() is the built-in precedence layer.
//
// The ASCII-only rule is load-bearing, not cosmetic: the renderer's width model is one byte == one column.
#include "csopesy/parameters.hpp"
#include <utility>

namespace csopesy {
namespace {

// One byte must equal one column for the renderer's width math, so only the printable ASCII range is text.
bool isPrintableAscii(unsigned char c) { return c >= 0x20 && c <= 0x7E; }

}  // namespace

ClampReport Parameters::setRefresh(int ms) {
  ClampReport r;
  r.field = "refreshMs";
  r.requested = ms;
  int applied = ms;
  if (applied < kRefreshMin) applied = kRefreshMin;
  else if (applied > kRefreshMax) applied = kRefreshMax;
  r.applied = applied;
  r.clamped = applied != ms;
  refreshMs = applied;
  return r;
}

ClampReport Parameters::setPolling(int ms) {
  ClampReport r;
  r.field = "pollingMs";
  r.requested = ms;
  int applied = ms;
  if (applied < kPollMin) applied = kPollMin;
  else if (applied > kPollMax) applied = kPollMax;
  r.applied = applied;
  r.clamped = applied != ms;
  pollingMs = applied;
  return r;
}

TextResult Parameters::setText(const std::string& t) {
  // Trim the surrounding spaces only; internal runs survive verbatim (§3.5 step 1, §3.7 rule 4).
  std::size_t begin = 0, end = t.size();
  while (begin < end && t[begin] == ' ') ++begin;
  while (end > begin && t[end - 1] == ' ') --end;
  std::string trimmed = t.substr(begin, end - begin);

  if (trimmed.empty()) return TextResult::Empty;
  for (unsigned char c : trimmed) {
    if (!isPrintableAscii(c)) return TextResult::NonAscii;
  }

  text = std::move(trimmed);
  return TextResult::Ok;
}

Parameters Parameters::defaults() {
  // The default member initializers are the built-in layer, so an empty body is correct.
  return {};
}

}  // namespace csopesy
