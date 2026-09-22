// src/platform/terminal_posix.cpp — the Linux + macOS backend (§3.4, §T1.1). Terminal::create(),
// installShutdownHandlers() and shutdownRequested() are defined exactly once, in the one platform file CMake
// selects; there is no runtime #ifdef anywhere else.
#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <string_view>

#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "csopesy/shutdown.hpp"
#include "csopesy/terminal.hpp"

namespace csopesy {
namespace {

// A signal handler may only touch an async-signal-safe flag: no allocation, no I/O, no terminal restore and
// no condition-variable notify.
volatile std::sig_atomic_t g_shutdownRequested = 0;

void onShutdownSignal(int) { g_shutdownRequested = 1; }

// A bare Esc is indistinguishable from the start of an arrow sequence until the next bytes either arrive or
// do not, so the rest of the sequence gets a short deadline instead of a blocking read.
constexpr int kEscapeFollowUpMs = 5;

enum class ReadResult { Byte, Timeout, Eof };

ReadResult readByte(int timeoutMs, unsigned char& out) {
  pollfd pfd{STDIN_FILENO, POLLIN, 0};
  const int ready = ::poll(&pfd, 1, timeoutMs);
  if (ready <= 0) { return ReadResult::Timeout; }   // EINTR included: the caller re-checks the shutdown flag
  const ssize_t n = ::read(STDIN_FILENO, &out, 1);
  if (n < 0) { return ReadResult::Timeout; }
  if (n == 0) { return ReadResult::Eof; }           // stdin closed
  return ReadResult::Byte;
}

class PosixTerminal final : public Terminal {
 public:
  ~PosixTerminal() override { restore(); }

  void enterRawMode() override {
    if (raw_) { return; }                          // idempotent: never re-save an already-raw terminal
    if (::tcgetattr(STDIN_FILENO, &saved_) != 0) { return; }   // not a tty: stay in plain line mode

    termios raw = saved_;
    // Input: Enter arrives as 0x0D, and stop Ctrl+S/Ctrl+Q from swallowing keys.
    raw.c_iflag &= static_cast<tcflag_t>(~(ICRNL | INLCR | IXON));
    // Output: no post-processing, so ONLCR is off and this layer must emit its own CRLF.
    raw.c_oflag &= static_cast<tcflag_t>(~OPOST);
    // Own the editing and the signals: Ctrl+C reaches readEvent() as 0x03 instead of becoming SIGINT.
    raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO | ISIG));
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (::tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0) { raw_ = true; }
  }

  void restore() override {
    if (!raw_) { return; }                         // idempotent: nothing was changed
    ::tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_);
    raw_ = false;
  }

  Size size() const override {
    winsize ws{};
    if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 && ::ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != 0) {
      return Size{};
    }
    if (ws.ws_row == 0 || ws.ws_col == 0) { return Size{}; }   // fall back to 24x80
    return Size{static_cast<int>(ws.ws_row), static_cast<int>(ws.ws_col)};
  }

  bool readEvent(KeyEvent& out, int timeoutMs) override {
    unsigned char b = 0;
    switch (readByte(timeoutMs, b)) {
      case ReadResult::Timeout: return false;
      case ReadResult::Eof: out = KeyEvent{KeyType::Eof, 0}; return true;
      case ReadResult::Byte: break;
    }
    switch (b) {
      case 0x0D:
      case 0x0A: out = KeyEvent{KeyType::Enter, 0}; return true;
      case 0x7F:
      case 0x08: out = KeyEvent{KeyType::Backspace, 0}; return true;
      case 0x09: out = KeyEvent{KeyType::Tab, 0}; return true;
      case 0x04:                                       // Ctrl+D
      case 0x03: out = KeyEvent{KeyType::Eof, 0}; return true;   // Ctrl+C: ISIG is off, so it arrives here
      case 0x1B: return decodeEscape(out);
      default: break;
    }
    if (b >= 0x20 && b <= 0x7E) { out = KeyEvent{KeyType::Char, static_cast<char>(b)}; return true; }
    return false;                                      // other control bytes carry no meaning in this contract
  }

  void write(std::string_view bytes) override {
    const char* p = bytes.data();
    std::size_t left = bytes.size();
    while (left > 0) {
      const ssize_t n = ::write(STDOUT_FILENO, p, left);
      if (n < 0) {
        if (errno == EINTR) { continue; }
        return;                                        // e.g. EAGAIN on a nonblocking stdout: drop the frame
      }
      p += n;
      left -= static_cast<std::size_t>(n);
    }
  }

  void flush() override { std::fflush(stdout); }

  bool isTty() const override { return ::isatty(STDIN_FILENO) != 0 && ::isatty(STDOUT_FILENO) != 0; }

  long long nowMs() const override {
    timespec ts{};
    if (::clock_gettime(CLOCK_MONOTONIC, &ts) != 0) { return 0; }
    return static_cast<long long>(ts.tv_sec) * 1000 + static_cast<long long>(ts.tv_nsec) / 1000000;
  }

 private:
  // CSI (ESC [ X) and SS3 (ESC O X) both end in the same letter for the four arrows.
  bool decodeEscape(KeyEvent& out) {
    unsigned char seq = 0;
    if (readByte(kEscapeFollowUpMs, seq) != ReadResult::Byte) {
      out = KeyEvent{KeyType::Escape, 0};
      return true;
    }
    if (seq == '[' || seq == 'O') {
      unsigned char final = 0;
      if (readByte(kEscapeFollowUpMs, final) == ReadResult::Byte) {
        switch (final) {
          case 'A': out = KeyEvent{KeyType::ArrowUp, 0}; return true;
          case 'B': out = KeyEvent{KeyType::ArrowDown, 0}; return true;
          case 'C': out = KeyEvent{KeyType::ArrowRight, 0}; return true;
          case 'D': out = KeyEvent{KeyType::ArrowLeft, 0}; return true;
          default: break;
        }
      }
    }
    out = KeyEvent{KeyType::Escape, 0};
    return true;
  }

  termios saved_{};
  bool raw_ = false;
};

}  // namespace

std::unique_ptr<Terminal> Terminal::create() { return std::make_unique<PosixTerminal>(); }

void installShutdownHandlers() {
  struct sigaction action {};
  action.sa_handler = &onShutdownSignal;
  ::sigemptyset(&action.sa_mask);
  action.sa_flags = 0;   // no SA_RESTART: poll() must return EINTR so the flag is seen without waiting a tick
  ::sigaction(SIGINT, &action, nullptr);
  ::sigaction(SIGTERM, &action, nullptr);
}

bool shutdownRequested() {
  const bool requested = g_shutdownRequested != 0;
  g_shutdownRequested = 0;   // read-and-clear; the input thread is the only reader
  return requested;
}

}  // namespace csopesy
