// src/main.cpp — the graded entry file (app/ layer).
// TODO(T2.5): parseCli(argv) -> Terminal::create() -> installShutdownHandlers() -> RAII terminal guard
// -> ConsoleApp{term, params}.run(). params.noTty must be known before raw mode is attempted.
#include "csopesy/cli.hpp"
#include "csopesy/console_app.hpp"

int main() {
  // TODO(T2.5): composition root.
  return 0;
}
