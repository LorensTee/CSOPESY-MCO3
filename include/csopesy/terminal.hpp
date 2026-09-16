// include/csopesy/terminal.hpp — the only platform-dependent contract, §3.3.
#pragma once
#include <memory>
#include <string_view>
#include "csopesy/keys.hpp"
namespace csopesy {
struct Size { int rows = 24; int cols = 80; };

class Terminal {
 public:
  virtual ~Terminal() = default;
  virtual void enterRawMode() = 0;   // no echo, no line buffering, VT output on
  virtual void restore() = 0;        // idempotent
  virtual Size size() const = 0;     // live size (resize aware)
  virtual bool readEvent(KeyEvent& out, int timeoutMs) = 0;  // BLOCKS up to timeoutMs; false on timeout
  virtual void write(std::string_view bytes) = 0;            // one call == one frame
  virtual void flush() = 0;
  virtual bool isTty() const = 0;    // false => plain line mode: no raw mode, no ANSI, no frames (§3.6)
  virtual long long nowMs() const = 0;  // monotonic; the ONLY clock source (injected into Scheduler)
  static std::unique_ptr<Terminal> create();   // defined once, in src/platform/*
};
}
