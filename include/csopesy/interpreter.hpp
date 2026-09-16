// include/csopesy/interpreter.hpp — §3.11.
// NOTE (T0.1): the plan's snippet lists only <string> + parameters.hpp + process.hpp, but the contract
// below uses std::string_view and KeyEvent, so <string_view> and csopesy/keys.hpp are required for the
// header to compile standalone. Two added includes; no signature changed.
#pragma once
#include <string>
#include <string_view>
#include "csopesy/keys.hpp"
#include "csopesy/parameters.hpp"
#include "csopesy/process.hpp"
namespace csopesy {
// The horizontal-scroll window for the prompt row (§3.11). Unit-tested directly: result.size() <= availWidth,
// and when the buffer is longer than the window the TAIL is shown so the cursor stays visible.
std::string visibleSlice(std::string_view buffer, int availWidth);

class Interpreter {
 public:
  Interpreter(Parameters&, MarqueeProcess&);   // start/stop_marquee mutate the PCB
  bool feed(const KeyEvent&);                  // true when a full line was executed
  std::string executeLine(const std::string&); // the entry point used by unit tests
  bool quitRequested() const;
  std::string prompt()      const;             // "Command>"
  std::string buffer()      const;             // echoed typed text
  std::string lastMessage() const;             // the response line(s) currently displayed
 private:
  Parameters& params_; MarqueeProcess& proc_;
  std::string line_, message_; bool quit_ = false;
};
}
