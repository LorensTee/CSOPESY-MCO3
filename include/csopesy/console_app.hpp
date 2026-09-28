// include/csopesy/console_app.hpp — main application control.
//
// ConsoleApp receives the terminal and program parameters.
// Its run() method creates the renderer, interpreter, process, and scheduler,
// draws the first frame, handles input, stops the worker, and prints the exit message.
#pragma once
namespace csopesy {
class Terminal;
struct Parameters;

class ConsoleApp {
 public:
  ConsoleApp(Terminal& term, Parameters& params);

  // Run the application and return its exit code.
  // The worker must stop before the terminal is restored.
  int run();

 private:
  Terminal& term_;
  Parameters& params_;
};
}
