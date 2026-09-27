// include/csopesy/cli.hpp — frozen contract, v3.1 §3.6.
//
// v3.1 (D19) adds the optional `config.txt` default layer, so precedence is now FOUR layers:
//
//     runtime command  >  CLI flag  >  config.txt  >  built-in default
//
// `config.txt` supplies defaults only: `parseCli` loads it first and the flags are applied on top. A missing
// file is not an error and produces no warnings. `--config` itself still does not exist; only the file's
// conventional name is recognized.
//
// The v2.6 rule survives verbatim, because it is about the quiz rather than about a file format (§3.6):
// **a typo must never cost the quiz.** An unknown key or flag, a malformed value and an absent value each
// degrade to the current value plus one warning line, and none of them can abort startup.
#pragma once
#include <string>
#include <vector>
#include "csopesy/parameters.hpp"
namespace csopesy {
struct CliResult {
  Parameters params;                  // config.txt (or built-in defaults) + whatever the flags provided
  std::vector<std::string> warnings;  // one line per unknown key/flag / malformed value / clamp; NEVER fatal
};

// Reads `path` as a simple `key=value` file. The recognized keys are the same values the flags accept, with
// the same validators and ranges: `refresh_ms` -> setRefresh, `polling_ms` -> setPolling. Blank lines and
// lines whose first non-space character is `#` are ignored, and surrounding whitespace on the key and value
// is trimmed. Unknown keys, malformed lines and malformed values warn and are ignored; out-of-range values
// are clamped and reported. A missing or unreadable file returns the built-in defaults with no warnings, so
// the file is optional. `parseCli` calls this for `config.txt`; it is public so tests and future callers can
// point at an explicit path.
CliResult loadConfigFile(const std::string& path);

// `args` EXCLUDES argv[0]. The whole recognized set is three flags (§3.6):
//   --no-tty          -> params.noTty = true        DEV/CI ONLY (D13); never part of the graded run
//   --refresh-ms=N    -> params.setRefresh(N)       clamped to [1, 10000] and reported
//   --poll-ms=N       -> params.setPolling(N)       clamped to [1, 1000] and reported
// `--flag=value` is the only accepted form; a repeated flag's last value wins; an old `--config=PATH` is
// simply an unknown flag and warns. `configPath` defaults to `config.txt` in the process working directory
// and exists so tests can avoid reading the real file. Parsing happens in main BEFORE Terminal::create() and
// before any thread exists, because `noTty` must be known before raw mode is attempted.
CliResult parseCli(const std::vector<std::string>& args, const std::string& configPath = "config.txt");
}
