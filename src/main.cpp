// src/main.cpp — the ENTRY FILE (§3.1 app/ layer; handout deliverable).
//
// TODO(T2.5): parseCli(argv) -> Terminal::create() -> installShutdownHandlers() -> RAII TerminalGuard
// (enterRawMode/restore) -> ConsoleApp{term, params}.run(). There is no --config, no --diag and no config
// file any more (D1, D5): the only flags are --no-tty / --refresh-ms / --poll-ms, and params.noTty must be
// known BEFORE raw mode is attempted (§3.6).
#include "csopesy/cli.hpp"
#include "csopesy/console_app.hpp"

int main() {
  // TODO(T2.5): the composition root. Phase 0 ships declarations and stubs only.
  return 0;
}
