// src/entities/config_io.cpp — the no-recompile levers: ini parsing + CLI flags (§3.6, §T2.2).
// TODO(T2.2): '#' comments only at line start; unknown key -> warn to stderr and keep going; malformed
// number -> warn + keep default; missing file -> run on defaults (never fatal); developers split on ';'.
#include "csopesy/config_io.hpp"

namespace csopesy {

ConfigResult loadConfig(const std::string&, Parameters&) {
  // TODO(T2.2): read the file, then delegate to loadConfigFromText. Set ok=false when it cannot be read.
  return {};
}

ConfigResult loadConfigFromText(std::string_view, Parameters&) {
  // TODO(T2.2): the parser itself; `--config=` path handling stays in main (§3.6).
  return {};
}

ConfigResult applyCliArgs(const std::vector<std::string>&, Parameters&) {
  // TODO(T2.2): exactly the flag set of §3.6, minus --config/--diag which main consumes.
  return {};
}

}  // namespace csopesy
