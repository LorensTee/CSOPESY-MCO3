// src/features/commands/line_editor.cpp — the §3.11 line-editor rules (T2.3).
//
// Raw mode disables echo, so the app echoes keystrokes itself. The editing state (line_ / message_ / quit_)
// is frozen on Interpreter, so the keystroke path and its visibleSlice helper are defined here as members
// of that type rather than on a second object. line_editor.hpp declares nothing of its own by design.
#include "csopesy/interpreter.hpp"

#include <cstddef>

namespace csopesy {

std::string visibleSlice(std::string_view buffer, int availWidth) {
  if (availWidth <= 0) return {};
  const std::size_t width = static_cast<std::size_t>(availWidth);
  if (buffer.size() <= width) return std::string(buffer);
  // Show the tail: the prompt row is one fixed row, so the cursor stays visible instead of wrapping.
  return std::string(buffer.substr(buffer.size() - width));
}

bool Interpreter::feed(const KeyEvent& ev) {
  switch (ev.type) {
    case KeyType::Char: {
      // Printable ASCII only: control bytes and non-ASCII keys are ignored on entry (§3.7 rule 7).
      const unsigned char c = static_cast<unsigned char>(ev.ch);
      if (c >= 0x20 && c <= 0x7E) line_.push_back(ev.ch);
      return false;
    }
    case KeyType::Backspace:
      if (!line_.empty()) line_.pop_back();        // the cursor is always at the end, so this is the last char
      return false;
    case KeyType::Enter: {
      const std::string submitted = line_;
      line_.clear();
      message_ = executeLine(submitted);
      return true;
    }
    case KeyType::Eof: {
      const std::string submitted = line_;
      line_.clear();
      message_ = executeLine(submitted);
      // Raw mode has ISIG off, so Ctrl+C, Ctrl+D and a closed stdin all arrive as this one key. Requesting
      // quit here is what makes §3.10's non-signal exit paths work: ConsoleApp observes quit_ right after
      // postEvent and runs requestStop() -> join() before the terminal is restored.
      quit_ = true;
      return true;
    }
    default:
      return false;                                // arrows, Tab and Escape are ignored, with no state change
  }
}

}  // namespace csopesy
