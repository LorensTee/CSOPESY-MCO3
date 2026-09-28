// Provide the terminal backend for Linux and macOS.
// The rest of the application uses the Terminal interface.
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

// A signal handler may change only this flag.
// Do not allocate memory, perform I/O, restore the terminal, or notify a condition variable here.
volatile std::sig_atomic_t g_shutdownRequested = 0;

void onShutdownSignal(int) { g_shutdownRequested = 1; }

// A standalone escape character may be the start of an arrow-key sequence.
// Wait briefly for the next byte before treating it as a standalone Esc key.
constexpr int kEscapeFollowUpMs = 5;

enum class ReadResult { Byte, Timeout, Eof };

ReadResult readByte(int timeoutMs, unsigned char& out) {
  pollfd pfd{STDIN_FILENO, POLLIN, 0};
  const int ready = ::poll(&pfd, 1, timeoutMs);
  if (ready <= 0) { return ReadResult::Timeout; }   // Includes EINTR. The caller checks the shutdown flag.
  const ssize_t n = ::read(STDIN_FILENO, &out, 1);
  if (n < 0) { return ReadResult::Timeout; }
  if (n == 0) { return ReadResult::Eof; }           // Standard input is closed.
  return ReadResult::Byte;
}

class PosixTerminal final : public Terminal {
 public:
  ~PosixTerminal() override { restore(); }

  void enterRawMode() override {
    if (raw_) { return; }                          // Do not save the terminal twice.
    if (::tcgetattr(STDIN_FILENO, &saved_) != 0) { return; }   // Stay in line mode when stdin is not a tty.

    termios raw = saved_;
    // Keep Enter as 0x0D and disable Ctrl+S/Ctrl+Q flow control.
    raw.c_iflag &= static_cast<tcflag_t>(~(ICRNL | INLCR | IXON));
    // Disable output processing. The renderer writes its own CRLF separators.
    raw.c_oflag &= static_cast<tcflag_t>(~OPOST);
    // Disable line buffering, echo, and signal generation.
    // The input layer handles these keys.
    raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO | ISIG));
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (::tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0) { raw_ = true; }
  }

  void restore() override {
    if (!raw_) { return; }                         // The terminal is already in its original mode.
    ::tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_);
    raw_ = false;
  }

  Size size() const override {
    winsize ws{};
    if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 && ::ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != 0) {
      return Size{};
    }
    if (ws.ws_row == 0 || ws.ws_col == 0) { return Size{}; }   // Report an invalid size.
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
      case 0x04:                                       // Ctrl+D.
      case 0x03: out = KeyEvent{KeyType::Eof, 0}; return true;   // Ctrl+C reaches this code because ISIG is off.
      case 0x1B: return decodeEscape(out);
      default: break;
    }
    if (b >= 0x20 && b <= 0x7E) { out = KeyEvent{KeyType::Char, static_cast<char>(b)}; return true; }
    return false;
  }

  void write(std::string_view bytes) override {
    const char* p = bytes.data();
    std::size_t left = bytes.size();
    while (left > 0) {
      const ssize_t n = ::write(STDOUT_FILENO, p, left);
      if (n < 0) {
        if (errno == EINTR) { continue; }
        return;                                        // Drop the frame when the write cannot continue.
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
  // CSI and SS3 arrow sequences end with the arrow letter.
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
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;   // Let poll() return EINTR so the input thread can check the flag.
  ::sigaction(SIGINT, &action, nullptr);
  ::sigaction(SIGTERM, &action, nullptr);
}

bool shutdownRequested() {
  const bool requested = g_shutdownRequested != 0;
  g_shutdownRequested = 0;   // Read and clear the flag. The input thread is the only reader.
  return requested;
}

}  // namespace csopesy
