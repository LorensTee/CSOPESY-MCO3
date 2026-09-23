// src/platform/terminal_win32.cpp — the Windows backend (§3.4, §T3.1). Terminal::create(),
// installShutdownHandlers() and shutdownRequested() are defined exactly once, in the one platform file CMake
// selects; there is no runtime #ifdef anywhere else.
//
// Two console modes are saved and restored together: the input mode is put into `_getch` form (no echo, no
// line buffering, no Ctrl+C translation, QuickEdit cleared) and the output mode gains VT processing, so the
// frame's ANSI sequences are interpreted rather than printed. VT *input* stays off on purpose — arrows arrive
// as the 0/224-prefixed scan codes `_getch` returns, never as escape sequences.
#define WIN32_LEAN_AND_MEAN   // trim windows.h to what the console APIs need
#define NOMINMAX
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600   // GetTickCount64
#endif
#include <windows.h>

#include <conio.h>

#include <cstddef>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string_view>

#include "csopesy/shutdown.hpp"
#include "csopesy/terminal.hpp"

namespace csopesy {
namespace {

// SetConsoleCtrlHandler runs on its own thread, so it cannot interrupt the input thread's blocked
// WaitForSingleObject; pollingMs bounds how long the flag can sit unread. Read-and-clear stays on the input
// thread only. The handler is flag-only: no allocation, no I/O, no restore, no condition-variable notify.
volatile std::sig_atomic_t g_shutdownRequested = 0;

BOOL WINAPI onConsoleControl(DWORD /*type*/) {
  g_shutdownRequested = 1;
  return TRUE;   // handled: the default handler must not kill the process mid-frame
}

class Win32Terminal;
Win32Terminal* g_active = nullptr;   // the atexit backstop's handle on the live terminal

// Raw mode is only meaningful on a console pair, and a caller that reaches enterRawMode() has already decided
// it is not in plain line mode (`isTty()` / `--no-tty` are the selectors). There is no ANSI-free animated
// fallback, so a failure here stops the program instead of degrading to escape codes printed as text.
[[noreturn]] void fatalStartup(const char* what) {
  std::fprintf(stderr, "csopesy: fatal: %s (GetLastError=%lu)\n", what,
               static_cast<unsigned long>(::GetLastError()));
  std::fflush(stderr);
  std::exit(EXIT_FAILURE);
}

class Win32Terminal final : public Terminal {
 public:
  Win32Terminal() : in_(::GetStdHandle(STD_INPUT_HANDLE)), out_(::GetStdHandle(STD_OUTPUT_HANDLE)) {}

  ~Win32Terminal() override {
    if (g_active == this) { g_active = nullptr; }   // the backstop must not touch a half-destroyed object
    restore();
  }

  void enterRawMode() override {
    if (raw_) { return; }   // idempotent: never re-save an already-raw console
    if (in_ == INVALID_HANDLE_VALUE || out_ == INVALID_HANDLE_VALUE ||
        !::GetConsoleMode(in_, &savedIn_) || !::GetConsoleMode(out_, &savedOut_)) {
      fatalStartup("no console on standard input/output; run with --no-tty for plain line mode");
    }

    // Output first: VT processing is what makes the frame's ANSI sequences layout instead of text.
    // DISABLE_NEWLINE_AUTO_RETURN matches POSIX raw mode's cleared OPOST — a lone \n moves down without also
    // returning the carriage, so the renderer emits its own CRLF on both platforms.
    DWORD outMode = savedOut_;
    outMode |= static_cast<DWORD>(ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN);
    if (!::SetConsoleMode(out_, outMode)) {
      fatalStartup("could not enable virtual terminal (ANSI) output; use Windows Terminal or another VT-capable console");
    }
    if (!::SetConsoleMode(in_, rawInputMode(savedIn_))) {
      ::SetConsoleMode(out_, savedOut_);   // leave the console as found before giving up
      fatalStartup("could not enter raw input mode on this console");
    }
    raw_ = true;
  }

  void restore() override {
    if (!raw_) { return; }   // idempotent, and safe when raw mode was never entered
    ::SetConsoleMode(in_, savedIn_);
    ::SetConsoleMode(out_, savedOut_);
    raw_ = false;
  }

  Size size() const override {
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (out_ == INVALID_HANDLE_VALUE || !::GetConsoleScreenBufferInfo(out_, &info)) { return Size{}; }
    // The visible window, not the scroll-back buffer: a resize changes srWindow without changing dwSize.
    const int cols = info.srWindow.Right - info.srWindow.Left + 1;
    const int rows = info.srWindow.Bottom - info.srWindow.Top + 1;
    if (cols <= 0 || rows <= 0) { return Size{}; }   // fall back to 24x80
    return Size{rows, cols};
  }

  bool readEvent(KeyEvent& out, int timeoutMs) override {
    if (in_ == INVALID_HANDLE_VALUE) { return false; }
    const DWORD waitMs = timeoutMs < 0 ? INFINITE : static_cast<DWORD>(timeoutMs);
    // _kbhit() cannot wait and polling it would burn a core and corrupt every timing measurement, so the
    // timeout is a real wait on the console input handle.
    if (::WaitForSingleObject(in_, waitMs) != WAIT_OBJECT_0) { return false; }
    if (_kbhit() == 0) {
      // A record the CRT path filters out (typically FOCUS_EVENT) keeps the console handle signaled, so
      // returning with it still buffered would make every later wait return immediately and spin this thread.
      drainFilteredRecords();
      return false;
    }
    return decode(_getch(), out);
  }

  void write(std::string_view bytes) override {
    if (out_ == INVALID_HANDLE_VALUE) { return; }
    const char* p = bytes.data();
    std::size_t left = bytes.size();
    while (left > 0) {
      const std::size_t kMaxChunk = 1u << 30;   // WriteFile takes a DWORD; a frame is far smaller
      const DWORD chunk = static_cast<DWORD>(left < kMaxChunk ? left : kMaxChunk);
      DWORD written = 0;
      if (!::WriteFile(out_, p, chunk, &written, nullptr) || written == 0) { return; }
      p += written;
      left -= written;
    }
  }

  void flush() override {}   // WriteFile is unbuffered: the frame is already on the console

  bool isTty() const override {
    DWORD mode = 0;
    return in_ != INVALID_HANDLE_VALUE && out_ != INVALID_HANDLE_VALUE &&
           ::GetConsoleMode(in_, &mode) != 0 && ::GetConsoleMode(out_, &mode) != 0;
  }

  long long nowMs() const override { return static_cast<long long>(::GetTickCount64()); }

 private:
  // The `_getch` form: ENABLE_EXTENDED_FLAGS is required for the QuickEdit bit to apply, and clearing
  // ENABLE_QUICK_EDIT_MODE stops a stray click from suspending the animation. Processed input is off so
  // Ctrl+C arrives as 0x03 like it does on POSIX; mouse/window records are unwanted and would only wake the
  // wait spuriously.
  static DWORD rawInputMode(DWORD saved) {
    const DWORD kUnwanted = ENABLE_QUICK_EDIT_MODE | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT;
    const DWORD kCooked = ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT;
    DWORD mode = saved | ENABLE_EXTENDED_FLAGS;
    mode &= ~kUnwanted;
    mode &= ~kCooked;
    mode &= ~static_cast<DWORD>(ENABLE_VIRTUAL_TERMINAL_INPUT);
    return mode;
  }

  // Consumes the records `_getch` would never deliver — FOCUS_EVENT, key-up, window/mouse records — one at a
  // time, and stops at the first key-down record. Stopping there is what makes this safe: `_kbhit() == 0`
  // proved there is no usable key right now, and a keystroke racing in is left for the next readEvent call.
  void drainFilteredRecords() {
    for (;;) {
      INPUT_RECORD rec{};
      DWORD n = 0;
      if (!::PeekConsoleInputA(in_, &rec, 1, &n) || n == 0) { return; }
      if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown != FALSE) { return; }
      if (!::ReadConsoleInputA(in_, &rec, 1, &n) || n == 0) { return; }
    }
  }

  bool decode(int c, KeyEvent& out) {
    if (c == EOF) { out = KeyEvent{KeyType::Eof, 0}; return true; }
    if (c == 0 || c == 224) { return decodeExtended(out); }   // Windows arrow-key lead bytes
    switch (c) {
      case 0x0D:
      case 0x0A: out = KeyEvent{KeyType::Enter, 0}; return true;
      case 0x08:
      case 0x7F: out = KeyEvent{KeyType::Backspace, 0}; return true;
      case 0x09: out = KeyEvent{KeyType::Tab, 0}; return true;
      case 0x1B: out = KeyEvent{KeyType::Escape, 0}; return true;   // VT input is off, so this is a bare Esc
      case 0x03:   // Ctrl+C
      case 0x04:   // Ctrl+D
      case 0x1A:   // Ctrl+Z, the Windows end-of-input character
        out = KeyEvent{KeyType::Eof, 0};
        return true;
      default: break;
    }
    if (c >= 0x20 && c <= 0x7E) { out = KeyEvent{KeyType::Char, static_cast<char>(c)}; return true; }
    return false;   // other control bytes carry no meaning in this contract
  }

  // Extended keys are a 0 or 224 lead byte followed by a scan code, and both bytes are already buffered.
  bool decodeExtended(KeyEvent& out) {
    if (_kbhit() == 0) { return false; }
    switch (_getch()) {
      case 72: out = KeyEvent{KeyType::ArrowUp, 0}; return true;
      case 80: out = KeyEvent{KeyType::ArrowDown, 0}; return true;
      case 75: out = KeyEvent{KeyType::ArrowLeft, 0}; return true;
      case 77: out = KeyEvent{KeyType::ArrowRight, 0}; return true;
      default: break;   // Insert/Delete/Home/End/function keys carry no meaning in this contract
    }
    return false;
  }

  HANDLE in_;
  HANDLE out_;
  DWORD savedIn_ = 0;
  DWORD savedOut_ = 0;
  bool raw_ = false;   // the saved modes are currently replaced by the raw/VT ones
};

void restoreActiveTerminal() {
  if (g_active != nullptr) { g_active->restore(); }
}

}  // namespace

std::unique_ptr<Terminal> Terminal::create() {
  // One atexit backstop per process: the RAII guard in main is the primary restore path, and this covers an
  // exit() that skipped it. It only ever calls the idempotent restore().
  static const bool registered = std::atexit(&restoreActiveTerminal) == 0;
  (void)registered;
  auto terminal = std::make_unique<Win32Terminal>();
  g_active = terminal.get();
  return terminal;
}

void installShutdownHandlers() {
  ::SetConsoleCtrlHandler(&onConsoleControl, TRUE);
}

bool shutdownRequested() {
  const bool requested = g_shutdownRequested != 0;
  g_shutdownRequested = 0;   // read-and-clear; the INPUT thread is the only reader (§3.8, §3.10)
  return requested;
}

}  // namespace csopesy
