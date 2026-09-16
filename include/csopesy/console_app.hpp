// include/csopesy/console_app.hpp — the graded entry path's composition root (§3.1, §T2.5).
//
// The plan names the type and its `run()` but never prints the header, so the shape below is derived
// from §T2.5 Step 1-2: main owns Terminal + Parameters and hands them in; ConsoleApp owns the
// Scheduler, Renderer, Interpreter and PCB and wires them together.
//
// T2.5 adds the private composition-root members (Renderer / Interpreter / MarqueeProcess / Scheduler)
// as part of implementing run(). That is a private-member addition, not a public-contract change, so it
// does not reopen the T0.4 freeze.
#pragma once
namespace csopesy {
class Terminal;
struct Parameters;

class ConsoleApp {
 public:
  ConsoleApp(Terminal& term, Parameters& params);
  // Draws the header, starts the marquee thread (Scheduler::start()), then BECOMES the input/command
  // thread; on exit runs the §3.10 shutdown sequence (requestStop -> join -> restore) and is the only
  // place the goodbye line is printed. Returns the process exit code.
  int run();

 private:
  Terminal& term_;
  Parameters& params_;
};
}
