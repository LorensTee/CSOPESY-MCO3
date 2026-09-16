// src/platform/terminal_posix.cpp — Linux + macOS backend (§3.4, §T1.1).
// Defines Terminal::create(), installShutdownHandlers() and shutdownRequested() EXACTLY ONCE, in the one
// platform file CMake selects; there is no runtime #ifdef anywhere else (§3.4).
#include <csignal>
#include <memory>

#include "csopesy/shutdown.hpp"
#include "csopesy/terminal.hpp"

namespace csopesy {
namespace {

// The only thing the signal handler is allowed to touch: an async-signal-safe flag (§3.10).
// No allocation, no I/O, no terminal restore and no condition-variable notify inside the handler.
volatile std::sig_atomic_t g_shutdownRequested = 0;

}  // namespace

std::unique_ptr<Terminal> Terminal::create() {
  // TODO(T1.1): return std::make_unique<PosixTerminal>() once raw mode (termios), live sizing (TIOCGWINSZ),
  // poll()-based readEvent, the monotonic nowMs() and the idempotent restore() exist.
  return nullptr;
}

void installShutdownHandlers() {
  // TODO(T1.1): sigaction(SIGINT) + sigaction(SIGTERM) -> g_shutdownRequested = 1, and nothing else.
}

bool shutdownRequested() {
  const bool requested = g_shutdownRequested != 0;
  g_shutdownRequested = 0;   // read-and-clear; the INPUT thread is the only reader (§3.8, §3.10)
  return requested;
}

}  // namespace csopesy
