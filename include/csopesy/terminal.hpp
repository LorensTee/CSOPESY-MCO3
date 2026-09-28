// include/csopesy/terminal.hpp — platform terminal interface.
//
// Terminal provides the common interface used by the application on Windows, Linux, and macOS.
// The platform implementation controls raw input, terminal size, console output, and the clock.
#pragma once
#include <memory>
#include <string_view>
#include "csopesy/keys.hpp"
namespace csopesy {

struct Size {
  int rows = 24;
  int cols = 80;
};

class Terminal {
 public:
  virtual ~Terminal() = default;

  // Disable normal terminal echo and line buffering, and enable VT output when supported.
  virtual void enterRawMode() = 0;

  // Restore the terminal state saved before raw mode.
  virtual void restore() = 0;

  // Return the current visible terminal size.
  virtual Size size() const = 0;

  // Wait up to timeoutMs for an input event.
  // Return false when no event is ready before the timeout.
  virtual bool readEvent(KeyEvent& out, int timeoutMs) = 0;

  // Write one complete frame or output message.
  virtual void write(std::string_view bytes) = 0;

  // Flush pending output.
  virtual void flush() = 0;

  // Return true when the input and output streams are terminals.
  // Return false to select plain line mode.
  virtual bool isTty() const = 0;

  // Return monotonic time in milliseconds.
  // Scheduler uses this as its only clock source.
  virtual long long nowMs() const = 0;

  // Create the platform-specific terminal implementation.
  static std::unique_ptr<Terminal> create();
};

}
