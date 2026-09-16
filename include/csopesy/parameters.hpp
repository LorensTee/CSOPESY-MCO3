// include/csopesy/parameters.hpp — one struct, layered precedence, §3.5.
#pragma once
#include <string>
#include <vector>
namespace csopesy {
struct ClampReport { bool clamped = false; int requested = 0; int applied = 0; const char* field = ""; };

struct Parameters {
  // marquee
  std::string text   = "CSOPESY";
  int  refreshMs     = 100;    // set_speed target. range [1, 10000]
  int  pollingMs     = 10;     // MAX idle wait when nothing is ready. [1, 1000]; does NOT delay keys (§3.8)
  bool asciiArt      = true;   // false => single-row plain scroll (CLI: --plain)
  int  marqueeRow    = 3;      // 1-based first band row. config/CLI only (§1.4)
  // chrome  (matches the handout's mock exactly)
  std::vector<std::string> developers = {"De La Cruz, Juan", "Santos, Alex"};
  std::string versionDate = "";          // blank in the mock; blank is valid
  // infra
  bool noTty = false;                    // force plain line mode (no raw mode, no ANSI, no frames)
  std::string measurePath;               // --measure=FILE (§3.6); empty => off. CLI-only, set before threads start

  static constexpr int kRefreshMin = 1, kRefreshMax = 10000;
  static constexpr int kPollMin = 1,    kPollMax = 1000;

  ClampReport setRefresh(int ms);      // clamps + reports; never throws
  ClampReport setPolling(int ms);
  bool setText(const std::string& t);  // false if empty after trim (state unchanged)
  static Parameters defaults();
};
}
