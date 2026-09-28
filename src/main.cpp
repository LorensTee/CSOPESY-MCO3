// src/main.cpp — program entry point.
//
// Parse command-line options, create the terminal, install shutdown handling,
// enter raw mode when needed, and run the application.
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "csopesy/cli.hpp"
#include "csopesy/console_app.hpp"
#include "csopesy/shutdown.hpp"
#include "csopesy/terminal.hpp"

namespace {

// Raw mode is a resource. Enter it in the constructor and restore it in the destructor.
// The platform layer also has an exit handler as a backup.
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

  // Write warnings to stderr so they do not change the frame on stdout.
  for (const std::string& warning : cli.warnings) {
    std::fprintf(stderr, "%s\n", warning.c_str());
  }

  const std::unique_ptr<csopesy::Terminal> term = csopesy::Terminal::create();
  // Select plain line mode before any worker thread starts.
  const bool plainLineMode = cli.params.noTty || !term->isTty();

  csopesy::installShutdownHandlers();
  TerminalGuard guard(*term, plainLineMode);

  csopesy::ConsoleApp app(*term, cli.params);
  return app.run();
}
