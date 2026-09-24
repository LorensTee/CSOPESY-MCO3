// src/main.cpp — the graded entry file (app/ layer, §T2.5 Step 1).
// Composition root: parse flags -> build the platform terminal -> install signal handlers -> enter raw mode
// through an RAII guard -> hand everything to ConsoleApp. No globals beyond the guard's lifetime.
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "csopesy/cli.hpp"
#include "csopesy/console_app.hpp"
#include "csopesy/shutdown.hpp"
#include "csopesy/terminal.hpp"

namespace {

// Raw mode is a resource: entered on construction, restored on every normal-scope exit. The platform layer
// installs the atexit backstop, so this guard is the primary path rather than the only one.
class TerminalGuard {
 public:
  TerminalGuard(csopesy::Terminal& term, bool plainLineMode) : term_(term), active_(!plainLineMode) {
    if (active_) term_.enterRawMode();
  }
  ~TerminalGuard() {
    if (active_) term_.restore();
  }
  TerminalGuard(const TerminalGuard&) = delete;
  TerminalGuard& operator=(const TerminalGuard&) = delete;

 private:
  csopesy::Terminal& term_;
  bool active_;
};

}  // namespace

int main(int argc, char** argv) {
  const std::vector<std::string> args(argv + 1, argv + argc);
  csopesy::CliResult cli = csopesy::parseCli(args);

  // Warnings go to stderr: a bad flag must not corrupt the frame the worker will draw on stdout.
  for (const std::string& warning : cli.warnings) {
    std::fprintf(stderr, "%s\n", warning.c_str());
  }

  const std::unique_ptr<csopesy::Terminal> term = csopesy::Terminal::create();
  // Plain line mode is chosen once, before any thread exists, and covers both --no-tty and a non-tty stdin.
  const bool plainLineMode = cli.params.noTty || !term->isTty();

  csopesy::installShutdownHandlers();
  TerminalGuard guard(*term, plainLineMode);

  csopesy::ConsoleApp app(*term, cli.params);
  return app.run();
}
