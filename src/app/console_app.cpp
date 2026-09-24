// src/app/console_app.cpp — the composition root (§T2.5).
// ConsoleApp starts the marquee worker, then BECOMES the input/command thread. While the worker lives the
// marquee thread is the only terminal writer; the initial header frame and the final goodbye line are written
// before start() and after join(), i.e. at the two moments the program is single-threaded again.
#include <iostream>
#include <string>

#include "csopesy/console_app.hpp"
#include "csopesy/interpreter.hpp"
#include "csopesy/process.hpp"
#include "csopesy/renderer.hpp"
#include "csopesy/scheduler.hpp"
#include "csopesy/shutdown.hpp"
#include "csopesy/terminal.hpp"

namespace csopesy {
namespace {

// Plain line mode (--no-tty or a non-tty stdin): no raw mode, no ANSI, no frames, NO worker thread. Responses
// are ordinary lines, which is what lets the real binary be smoke-tested on all three CI images.
int runPlainLineMode(Parameters& params) {
  MarqueeProcess proc;
  Interpreter interp(params, proc);

  const std::string prompt = interp.prompt();
  std::string line;
  std::cout << prompt << ' ' << std::flush;
  while (std::getline(std::cin, line)) {
    const std::string response = interp.executeLine(line);
    if (!response.empty()) {
      std::cout << response << '\n';
    }
    if (interp.quitRequested()) {
      return 0;
    }
    std::cout << prompt << ' ' << std::flush;
  }
  return 0;
}

}  // namespace

ConsoleApp::ConsoleApp(Terminal& term, Parameters& params) : term_(term), params_(params) {}

int ConsoleApp::run() {
  if (params_.noTty || !term_.isTty()) {
    return runPlainLineMode(params_);
  }

  // Locals, not members: this keeps the frozen public header unchanged and makes §3.10's destruction order
  // explicit in one scope. ~Scheduler joins as a backstop on any early return.
  Renderer renderer;
  MarqueeProcess proc;
  Interpreter interp(params_, proc);
  Scheduler scheduler(term_, params_, renderer, interp, proc);

  // The header is drawn before the worker exists, so the worker remains the only writer while it lives.
  {
    const Size size = term_.size();
    const std::string frame = renderer.buildFrame(params_, proc, interp.prompt(), interp.buffer(),
                                                  interp.lastMessage(), size.rows, size.cols);
    if (!frame.empty()) {
      term_.write(frame);
      term_.flush();
    }
  }

  scheduler.start();

  // Input/command thread: read one event, hand it to the worker, then re-check both exit intents. The check
  // runs after EVERY readEvent (including the timeout/EINTR path), or `exit` would leave the worker animating.
  for (;;) {
    KeyEvent event;
    if (term_.readEvent(event, params_.pollingMs)) {
      scheduler.postEvent(event);
    }
    if (interp.quitRequested() || shutdownRequested()) {
      break;
    }
  }

  scheduler.requestStop();
  scheduler.join();   // before the terminal is restored: no worker may write into a restored terminal

  // The join returned the program to one thread, so the input thread may print the goodbye line.
  term_.write("Exiting CSOPESY. Goodbye!\r\n");
  term_.flush();
  return 0;
}

}  // namespace csopesy
