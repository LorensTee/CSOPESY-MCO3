// src/features/commands/line_editor.cpp — keyboard input and line editing.
//
// Raw mode disables terminal echo, so the application handles echo and editing.
// Interpreter stores the line, message, and quit state. The visibleSlice() helper keeps the cursor visible.
#include "csopesy/interpreter.hpp"

#include <cstddef>

namespace csopesy {

std::string visibleSlice(std::string_view buffer, int availWidth) {
  if (availWidth <= 0) return {};
  const std::size_t width = static_cast<std::size_t>(availWidth);
  if (buffer.size() <= width) return std::string(buffer);
  // Show the end of the buffer so the cursor stays visible on the fixed prompt row.
  return std::string(buffer.substr(buffer.size() - width));
}

bool Interpreter::feed(const KeyEvent& ev) {
  switch (ev.type) {
    case KeyType::Char: {
      // Accept printable ASCII characters only. Ignore other character values.
      const unsigned char c = static_cast<unsigned char>(ev.ch);
      if (c >= 0x20 && c <= 0x7E) line_.push_back(ev.ch);
      return false;
    }
    case KeyType::Backspace:
      if (!line_.empty()) line_.pop_back();
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
      // Treat EOF as a quit request after processing the line.
      quit_ = true;
      return true;
    }
    default:
      return false;
  }
}

}  // namespace csopesy
