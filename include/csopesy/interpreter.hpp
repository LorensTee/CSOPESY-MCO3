// include/csopesy/interpreter.hpp — command input and editing state.
//
// Interpreter stores the current input line, the latest message, and the quit state.
// The application owns synchronization when the interpreter is shared between threads.
// The line editor and command executor use this same object.

#pragma once
#include <string>
#include <string_view>
#include "csopesy/keys.hpp"
#include "csopesy/parameters.hpp"
#include "csopesy/process.hpp"
namespace csopesy {

// Return the part of the input buffer that fits in the prompt row.
// When the buffer is too long, return its tail so the cursor remains visible.
std::string visibleSlice(std::string_view buffer, int availWidth);

class Interpreter {
 public:
  Interpreter(Parameters&, MarqueeProcess&);

  // Process one key event. Return true when the event submits a complete line.
  bool feed(const KeyEvent&);

  // Execute a complete input line.
  std::string executeLine(const std::string&);

  bool quitRequested() const;

  // Return the prompt shown before the input buffer.
  std::string prompt() const;

  // Return the current input buffer.
  std::string buffer() const;

  // Return the latest command response.
  std::string lastMessage() const;

 private:
  Parameters& params_;
  MarqueeProcess& proc_;
  std::string line_;
  std::string message_;
  bool quit_ = false;
};
}
