// src/main.cpp — the ENTRY FILE (§3.1 app/ layer; handout deliverable).
// TODO(T2.5): parse argv -> --config=PATH -> loadConfig -> applyCliArgs -> --diag short-circuit ->
// Terminal::create() -> installShutdownHandlers() -> RAII TerminalGuard (enterRawMode/restore) ->
// ConsoleApp{term, params}.run().
#include "csopesy/console_app.hpp"
#include "csopesy/parameters.hpp"

int main() {
  // TODO(T2.5): the composition root. Phase 0 ships declarations and stubs only.
  return 0;
}
