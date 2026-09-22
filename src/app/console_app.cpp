// src/app/console_app.cpp — the composition root (§T2.5).
// TODO(T2.5): draw the header, Scheduler::start(), become the input/command thread, then run the shutdown
// sequence (requestStop -> join -> restore) and print the goodbye line last.
#include "csopesy/console_app.hpp"
#include "csopesy/terminal.hpp"

namespace csopesy {

ConsoleApp::ConsoleApp(Terminal& term, Parameters& params) : term_(term), params_(params) {}

int ConsoleApp::run() {
  // TODO(T2.5): owns Renderer, Interpreter, MarqueeProcess and Scheduler and wires them together.
  return 0;
}

}  // namespace csopesy
