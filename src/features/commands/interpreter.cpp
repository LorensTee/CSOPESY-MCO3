// src/features/commands/interpreter.cpp — command recognition and the response table (§3.7, §T2.4).
// The keystroke/editing path lives in line_editor.cpp; this unit owns only line execution and the accessors.
// TODO(T2.4): trim, split on the first whitespace run, then match the six commands case-sensitively;
// set_speed's argument must match [-+]?[0-9]+ in full (never a silent partial std::stoi); set_text renders
// all three Parameters::setText outcomes (Ok / Empty / NonAscii) instead of mangling the argument.
#include "csopesy/interpreter.hpp"

namespace csopesy {

Interpreter::Interpreter(Parameters& params, MarqueeProcess& proc) : params_(params), proc_(proc) {}

std::string Interpreter::executeLine(const std::string&) {
  // TODO(T2.4): the response table of §3.7; unit tests call this directly (lock-free, single-threaded).
  return {};
}

bool Interpreter::quitRequested() const { return quit_; }

std::string Interpreter::prompt() const { return "Command>"; }   // frozen literal (§3.9/§3.11)

std::string Interpreter::buffer() const { return line_; }

std::string Interpreter::lastMessage() const { return message_; }

}  // namespace csopesy
