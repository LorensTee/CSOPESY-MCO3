// include/csopesy/parameters.hpp — frozen contract, v3.0 §3.5.
//
// v3.0 DELTA (T0.6, ratified 2026-09-22): `asciiArt` (D2), `marqueeRow` (D3) and `measurePath` (D4) are
// REMOVED — ten fields become seven. `setText` now reports WHY it refused, via TextResult, because the text
// contract is explicitly ASCII (D15) and the interpreter must be able to tell "empty" from "non-ASCII"
// apart to print the right message (§3.7).
#pragma once
#include <string>
#include <vector>
namespace csopesy {
struct ClampReport { bool clamped = false; int requested = 0; int applied = 0; const char* field = ""; };

// D15, the ASCII text contract. The renderer's width model is ONE BYTE == ONE COLUMN, so an argument that
// could make a slice split a multi-byte character is REJECTED, never normalized: the text the operator asked
// for is never silently altered (§3.5, §3.7 rule 7).
enum class TextResult { Ok, Empty, NonAscii };

struct Parameters {
  // marquee
  std::string text   = "CSOPESY";   // INVARIANT: every byte is in [0x20, 0x7E]. Mutated only via setText().
  int  refreshMs     = 100;         // set_speed target. range [1, 10000]
  int  pollingMs     = 10;          // MAX idle wait when nothing is ready. [1, 1000]; does NOT delay keys
  // chrome  (matches the handout's mock exactly, §1.4)
  std::vector<std::string> developers = {"De La Cruz, Juan", "Santos, Alex"};
  std::string versionDate = "";     // blank in the mock; blank is valid
  // infra
  bool noTty = false;               // plain line mode: no raw mode, no ANSI, no frames, no worker thread.
                                    // DEV/CI ONLY (D13) — never part of the graded run, and the one field
                                    // written before any thread exists (§3.5's shared-state rule).

  static constexpr int kRefreshMin = 1, kRefreshMax = 10000;
  static constexpr int kPollMin = 1,    kPollMax = 1000;

  ClampReport setRefresh(int ms);      // clamps + reports; never throws
  ClampReport setPolling(int ms);
  TextResult  setText(const std::string& t);  // trim, keep internal runs; Ok | Empty | NonAscii
  static Parameters defaults();
};
}
