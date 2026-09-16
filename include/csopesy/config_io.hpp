// include/csopesy/config_io.hpp — §3.6. The no-recompile levers: ini file + CLI flags.
//
// A typo must never cost the quiz (§3.6): unknown key / malformed number / missing file are
// reported through ConfigResult and are never fatal — the caller keeps running on defaults.
//
// NOT handled here, because neither is a Parameters field:
//   --config=PATH  is consumed by main BEFORE loadConfig runs (it names the file to load);
//   --diag         is consumed by main as its short-circuit debug report (§3.6).
#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "csopesy/parameters.hpp"
namespace csopesy {
struct ConfigResult {
  bool ok = false;                      // false => the file could not be read (never fatal)
  std::vector<std::string> warnings;    // one human-readable line per unknown key / malformed value
};

ConfigResult loadConfig(const std::string& path, Parameters& params);
ConfigResult loadConfigFromText(std::string_view text, Parameters& params);
ConfigResult applyCliArgs(const std::vector<std::string>& args, Parameters& params);
}
