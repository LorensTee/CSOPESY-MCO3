// Run the application after startup is complete.
// ConsoleApp starts the marquee worker and handles input on the main thread.
// The worker writes frames while it is running.
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

// Run without raw mode, ANSI output, or a worker thread.
// This mode runs the program as a normal line-based program.
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

  // Keep these objects local so they are destroyed in the correct order.
  // Scheduler joins the worker before these objects are destroyed.
  Renderer renderer;
  MarqueeProcess proc;
  Interpreter interp(params_, proc);
  Scheduler scheduler(term_, params_, renderer, interp, proc);

  // Draw the first frame before the worker starts.
  // The main thread can write it because no worker exists yet.
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

  // Read one event and send it to the scheduler.
  // Check for shutdown after every readEvent() call.
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
  scheduler.join();   // Wait for the worker to stop before shutdown.

  // The worker has stopped, so the main thread can write the exit message.
  // The renderer leaves the cursor at the bottom-right cell. Move to the start
  // of a fresh line before writing so the message is not split across lines.
  term_.write("\r\nExiting CSOPESY. Goodbye!\r\n");
  term_.flush();
  return 0;
}

}  // namespace csopesy
