// src/platform/terminal_win32.cpp — the Windows backend (§3.4, §T3.1).
// Defines Terminal::create(), installShutdownHandlers() and shutdownRequested() EXACTLY ONCE, in the one
// platform file CMake selects; there is no runtime #ifdef anywhere else (§3.4).
#include <csignal>
#include <memory>

#include "csopesy/shutdown.hpp"
#include "csopesy/terminal.hpp"

namespace csopesy {
namespace {

// SetConsoleCtrlHandler runs on a THREAD OF ITS OWN, so it cannot interrupt the input thread's blocked
// WaitForSingleObject; polling_ms bounds Ctrl+C-to-exit latency instead (§3.8). Read-and-clear stays on
// the input thread only, exactly as on POSIX.
volatile std::sig_atomic_t g_shutdownRequested = 0;

}  // namespace

std::unique_ptr<Terminal> Terminal::create() {
  // TODO(T3.1): return std::make_unique<Win32Terminal>() once VT output is enabled, the console mode is
  // saved/restored, querySize() reads live sizing and readEvent reads STD_INPUT_HANDLE with a timeout.
  return nullptr;
}

void installShutdownHandlers() {
  // TODO(T3.1): SetConsoleCtrlHandler(...) -> g_shutdownRequested = 1, and nothing else.
}

bool shutdownRequested() {
  const bool requested = g_shutdownRequested != 0;
  g_shutdownRequested = 0;   // read-and-clear; the INPUT thread is the only reader (§3.8, §3.10)
  return requested;
}

}  // namespace csopesy
