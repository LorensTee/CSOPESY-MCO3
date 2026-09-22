// src/features/commands/interpreter.cpp — command recognition and the response table (v3.0 §3.7, §T2.4).
// TODO(T2.4): trim -> split on the first whitespace run -> case-sensitive match against the SIX commands;
// set_speed's argument must match [-+]?[0-9]+ in full (never a silent std::stoi partial parse); and set_text
// has THREE outcomes to render from Parameters::setText — Ok / Empty / NonAscii — so a non-ASCII argument gets
// its own message instead of being mangled (D15; the §3.7 row added by v3).
#include "csopesy/interpreter.hpp"

namespace csopesy {

std::string visibleSlice(std::string_view, int) {
  // TODO(T2.3): at most availWidth characters, showing the TAIL so the cursor stays visible (§3.11).
  return {};
}

Interpreter::Interpreter(Parameters& params, MarqueeProcess& proc) : params_(params), proc_(proc) {}

bool Interpreter::feed(const KeyEvent&) {
  // TODO(T2.4): one keystroke in; true when a full line was executed.
  return false;
}

std::string Interpreter::executeLine(const std::string&) {
  // TODO(T2.4): the response table of §3.7; unit tests call this directly (lock-free, single-threaded).
  return {};
}

bool Interpreter::quitRequested() const { return quit_; }

std::string Interpreter::prompt() const { return "Command>"; }   // frozen literal (§3.9/§3.11)

std::string Interpreter::buffer() const { return line_; }

std::string Interpreter::lastMessage() const { return message_; }

}  // namespace csopesy
