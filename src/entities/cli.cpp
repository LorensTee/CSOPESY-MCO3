// src/entities/cli.cpp — argv parsing, the only parameter layer besides the commands (v3.0 §3.6, §T2.2).
//
// TODO(T2.2): exactly the three flags of §3.6 — --no-tty, --refresh-ms=N, --poll-ms=N, in `--flag=value`
// form only. An unknown flag warns and continues; a malformed or absent value warns and keeps the default; a
// well-formed OUT-OF-RANGE value is CLAMPED and reported (never a usage error); a repeated flag's last value
// wins. No file is ever read: there is no config layer any more (D1).
#include "csopesy/cli.hpp"

namespace csopesy {

CliResult parseCli(const std::vector<std::string>&) {
  // TODO(T2.2): the warning lines are the whole point — a typo must never cost the quiz (§3.6).
  return {};
}

}  // namespace csopesy
