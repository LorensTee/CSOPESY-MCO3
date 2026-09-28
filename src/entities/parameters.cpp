// src/entities/parameters.cpp — parameter validation and text handling.
//
// setRefresh() and setPolling() clamp values to their allowed ranges.
// setText() trims outer spaces, keeps internal spaces, and accepts printable ASCII only.
// Invalid text leaves the current value unchanged.
#include "csopesy/parameters.hpp"
#include <utility>

namespace csopesy {
namespace {

// The renderer treats one byte as one column, so text must use printable ASCII.
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
  // Remove leading and trailing spaces. Keep internal spaces unchanged.
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
  // Default member values already set every parameter.
  return {};
}

}  // namespace csopesy
