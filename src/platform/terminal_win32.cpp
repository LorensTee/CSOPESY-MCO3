// src/platform/terminal_win32.cpp — the Windows terminal backend.
//
// Terminal::create(), installShutdownHandlers(), and shutdownRequested() live in the selected platform file.
// The rest of the application uses the Terminal interface and does not need platform checks.
//
// Save the console input and output modes together.
// Input uses _getch() style behavior with no echo, no line buffering, no Ctrl+C translation, and no QuickEdit.
// Output enables VT processing so the renderer's ANSI sequences work.
#define WIN32_LEAN_AND_MEAN   // Limit windows.h to the console APIs used here.
#define NOMINMAX
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600   // GetTickCount64.
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

// Windows calls the control handler on another thread.
// Set only this flag here. The input thread reads and clears it.
volatile std::sig_atomic_t g_shutdownRequested = 0;

BOOL WINAPI onConsoleControl(DWORD /*type*/) {
  g_shutdownRequested = 1;
  return TRUE;   // Prevent the default handler from stopping the process during a frame write.
}

class Win32Terminal;
Win32Terminal* g_active = nullptr;   // Used by the atexit restore path.

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
    if (g_active == this) { g_active = nullptr; }   // The restore path must not use a destroyed object.
    restore();
  }

  void enterRawMode() override {
    if (raw_) { return; }   // Do not save an already-raw console again.
    if (in_ == INVALID_HANDLE_VALUE || out_ == INVALID_HANDLE_VALUE ||
        !::GetConsoleMode(in_, &savedIn_) || !::GetConsoleMode(out_, &savedOut_)) {
      fatalStartup("no console on standard input/output; run with --no-tty for plain line mode");
    }

    // Enable VT output before changing input.
    // DISABLE_NEWLINE_AUTO_RETURN lets the renderer control CRLF output on both platforms.
    DWORD outMode = savedOut_;
    outMode |= static_cast<DWORD>(ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN);
    if (!::SetConsoleMode(out_, outMode)) {
      fatalStartup("could not enable virtual terminal (ANSI) output; use Windows Terminal or another VT-capable console");
    }
    if (!::SetConsoleMode(in_, rawInputMode(savedIn_))) {
      ::SetConsoleMode(out_, savedOut_);   // Restore the original output mode before stopping startup.
      fatalStartup("could not enter raw input mode on this console");
    }
    raw_ = true;
  }

  void restore() override {
    if (!raw_) { return; }   // Nothing to restore.
    ::SetConsoleMode(in_, savedIn_);
    ::SetConsoleMode(out_, savedOut_);
    raw_ = false;
  }

  Size size() const override {
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (out_ == INVALID_HANDLE_VALUE || !::GetConsoleScreenBufferInfo(out_, &info)) { return Size{}; }
    // Use the visible window size, not the scroll-back buffer size.
    const int cols = info.srWindow.Right - info.srWindow.Left + 1;
    const int rows = info.srWindow.Bottom - info.srWindow.Top + 1;
    if (cols <= 0 || rows <= 0) { return Size{}; }
    return Size{rows, cols};
  }

  bool readEvent(KeyEvent& out, int timeoutMs) override {
    if (in_ == INVALID_HANDLE_VALUE) { return false; }
    const DWORD waitMs = timeoutMs < 0 ? INFINITE : static_cast<DWORD>(timeoutMs);

    // Wait on the console handle instead of polling _kbhit().
    // This avoids a busy loop and keeps input timing stable.
    if (::WaitForSingleObject(in_, waitMs) != WAIT_OBJECT_0) { return false; }
    if (_kbhit() == 0) {
      // Remove input records that _getch() does not return.
      // Leave the first key-down record in the queue for the next call.
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
      const std::size_t kMaxChunk = 1u << 30;   // WriteFile accepts a DWORD; frames are much smaller.
      const DWORD chunk = static_cast<DWORD>(left < kMaxChunk ? left : kMaxChunk);
      DWORD written = 0;
      if (!::WriteFile(out_, p, chunk, &written, nullptr) || written == 0) { return; }
      p += written;
      left -= written;
    }
  }

  void flush() override {}   // WriteFile writes directly to the console.

  bool isTty() const override {
    DWORD mode = 0;
    return in_ != INVALID_HANDLE_VALUE && out_ != INVALID_HANDLE_VALUE &&
           ::GetConsoleMode(in_, &mode) != 0 && ::GetConsoleMode(out_, &mode) != 0;
  }

  long long nowMs() const override { return static_cast<long long>(::GetTickCount64()); }

 private:
  // _getch() uses two bytes for extended keys.
  // Disable QuickEdit, mouse input, window input, line input, echo, and processed input.
  static DWORD rawInputMode(DWORD saved) {
    const DWORD kUnwanted = ENABLE_QUICK_EDIT_MODE | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT;
    const DWORD kCooked = ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT;
    DWORD mode = saved | ENABLE_EXTENDED_FLAGS;
    mode &= ~kUnwanted;
    mode &= ~kCooked;
    mode &= ~static_cast<DWORD>(ENABLE_VIRTUAL_TERMINAL_INPUT);
    return mode;
  }

  // Consume console records that _getch() does not return.
  // Stop at the first key-down record so a new key remains for the next readEvent() call.
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
    if (c == 0 || c == 224) { return decodeExtended(out); }   // Windows extended-key lead bytes.
    switch (c) {
      case 0x0D:
      case 0x0A: out = KeyEvent{KeyType::Enter, 0}; return true;
      case 0x08:
      case 0x7F: out = KeyEvent{KeyType::Backspace, 0}; return true;
      case 0x09: out = KeyEvent{KeyType::Tab, 0}; return true;
      case 0x1B: out = KeyEvent{KeyType::Escape, 0}; return true;   // VT input is disabled.
      case 0x03:   // Ctrl+C.
      case 0x04:   // Ctrl+D.
      case 0x1A:   // Ctrl+Z.
        out = KeyEvent{KeyType::Eof, 0};
        return true;
      default: break;
    }
    if (c >= 0x20 && c <= 0x7E) { out = KeyEvent{KeyType::Char, static_cast<char>(c)}; return true; }
    return false;   // Ignore other control values.
  }

  // Extended keys use a 0 or 224 lead byte followed by a scan code.
  bool decodeExtended(KeyEvent& out) {
    if (_kbhit() == 0) { return false; }
    switch (_getch()) {
      case 72: out = KeyEvent{KeyType::ArrowUp, 0}; return true;
      case 80: out = KeyEvent{KeyType::ArrowDown, 0}; return true;
      case 75: out = KeyEvent{KeyType::ArrowLeft, 0}; return true;
      case 77: out = KeyEvent{KeyType::ArrowRight, 0}; return true;
      default: break;   // Ignore Insert, Delete, Home, End, and function keys.
    }
    return false;
  }

  HANDLE in_;
  HANDLE out_;
  DWORD savedIn_ = 0;
  DWORD savedOut_ = 0;
  bool raw_ = false;   // True after raw input and VT output modes replace the saved modes.
};

void restoreActiveTerminal() {
  if (g_active != nullptr) { g_active->restore(); }
}

}  // namespace

std::unique_ptr<Terminal> Terminal::create() {
  // Register one exit handler as a backup.
  // The RAII guard in main is the normal restore path.
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
  g_shutdownRequested = 0;   // Read and clear the flag. The input thread is the only reader.
  return requested;
}

}  // namespace csopesy
