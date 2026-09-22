// src/entities/cli.cpp — argv parsing, the only parameter layer besides the commands (§T2.2).
// TODO(T2.2): three flags — --no-tty, --refresh-ms=N, --poll-ms=N — in `--flag=value` form only. Bad input
// never aborts startup: an unknown flag or a malformed/absent value warns and keeps the default, a
// well-formed out-of-range value is clamped and reported, and a repeated flag's last value wins.
#include "csopesy/cli.hpp"

namespace csopesy {

CliResult parseCli(const std::vector<std::string>&) {
  // TODO(T2.2): a typo must never cost the quiz — warn and continue.
  return {};
}

}  // namespace csopesy
