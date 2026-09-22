// include/csopesy/cli.hpp — frozen contract, v3.0 §3.6.
//
// v3.0 REPLACES config_io.hpp with this header (D1). There is no config file, no `--config`, no
// `quiz_case_<n>.ini` and no parser for any of them; `Parameters`' precedence is now THREE layers, not four:
//
//     runtime command  >  CLI flag  >  built-in default
//
// The v2.6 rule survives verbatim, because it is about the quiz rather than about a file format (§3.6):
// **a typo must never cost the quiz.** An unknown flag, a malformed number and an absent value each degrade
// to the default plus one warning line, and none of them can abort startup.
#pragma once
#include <string>
#include <vector>
#include "csopesy/parameters.hpp"
namespace csopesy {
struct CliResult {
  Parameters params;                  // defaults + whatever the flags explicitly provided
  std::vector<std::string> warnings;  // one line per unknown flag / malformed value / clamp; NEVER fatal
};

// `args` EXCLUDES argv[0]. The whole recognized set is three flags (§3.6):
//   --no-tty          -> params.noTty = true        DEV/CI ONLY (D13); never part of the graded run
//   --refresh-ms=N    -> params.setRefresh(N)       clamped to [1, 10000] and reported
//   --poll-ms=N       -> params.setPolling(N)       clamped to [1, 1000] and reported
// `--flag=value` is the only accepted form; a repeated flag's last value wins; an old `--config=PATH` is
// simply an unknown flag and warns. Parsing happens in main BEFORE Terminal::create() and before any thread
// exists, because `noTty` must be known before raw mode is attempted.
CliResult parseCli(const std::vector<std::string>& args);
}
