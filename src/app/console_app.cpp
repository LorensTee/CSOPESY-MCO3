// src/app/console_app.cpp — the composition root (§3.1, §T2.5).
// TODO(T2.5): draw the header, Scheduler::start(), become the input/command thread, then run the §3.10
// shutdown sequence (requestStop -> join -> restore) and print the goodbye line last.
#include "csopesy/console_app.hpp"
#include "csopesy/terminal.hpp"

namespace csopesy {

ConsoleApp::ConsoleApp(Terminal& term, Parameters& params) : term_(term), params_(params) {}

int ConsoleApp::run() {
  // TODO(T2.5): owns Renderer, Interpreter, MarqueeProcess and Scheduler and wires them together.
  return 0;
}

}  // namespace csopesy
