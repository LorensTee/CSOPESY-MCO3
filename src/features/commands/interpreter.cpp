// src/features/commands/interpreter.cpp — command recognition and the response table (§3.7, §T2.4).
// The keystroke/editing path lives in line_editor.cpp; this unit owns line execution and the accessors.
#include "csopesy/interpreter.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>

namespace csopesy {
namespace {

bool isSpace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

std::string trim(const std::string& s) {
  std::size_t a = 0, b = s.size();
  while (a < b && isSpace(s[a])) ++a;
  while (b > a && isSpace(s[b - 1])) --b;
  return s.substr(a, b - a);
}

enum class Cmd { Help, Start, Stop, SetText, SetSpeed, Exit, Unknown };

Cmd lookup(const std::string& name) {
  // Exact, case-sensitive names only; "HELP" is deliberately not a command (§3.7 rule 3).
  static const std::unordered_map<std::string, Cmd> table = {
      {"help", Cmd::Help},         {"start_marquee", Cmd::Start}, {"stop_marquee", Cmd::Stop},
      {"set_text", Cmd::SetText},  {"set_speed", Cmd::SetSpeed},  {"exit", Cmd::Exit}};
  const auto it = table.find(name);
  return it == table.end() ? Cmd::Unknown : it->second;
}

const char* const kHelp =
    "Available commands:\n"
    "  help           - displays the commands and its description\n"
    "  start_marquee  - starts the marquee animation\n"
    "  stop_marquee   - stops the marquee animation\n"
    "  set_text       - accepts a text input and displays it as a marquee\n"
    "  set_speed      - sets the marquee animation refresh in milliseconds\n"
    "  exit           - terminates the console";

// Matches [-+]?[0-9]+ in full. The magnitude saturates far above the clamp bound so a huge literal is
// clamped and reported rather than misread as a usage error (std::stoi would silently accept "5abc").
bool parseFullInteger(const std::string& s, int& out) {
  if (s.empty()) return false;
  std::size_t i = 0;
  bool negative = false;
  if (s[0] == '+' || s[0] == '-') {
    negative = s[0] == '-';
    i = 1;
  }
  if (i >= s.size()) return false;
  unsigned long long mag = 0;
  for (; i < s.size(); ++i) {
    if (s[i] < '0' || s[i] > '9') return false;
    const unsigned int digit = static_cast<unsigned int>(s[i] - '0');
    if (mag > (1000000000000000000ULL - digit) / 10ULL) mag = 1000000000000000000ULL;
    else mag = mag * 10ULL + digit;
  }
  if (negative) out = (mag >= 2147483648ULL) ? -2147483647 - 1 : -static_cast<int>(mag);
  else out = (mag > 2147483647ULL) ? 2147483647 : static_cast<int>(mag);
  return true;
}

}  // namespace

Interpreter::Interpreter(Parameters& params, MarqueeProcess& proc) : params_(params), proc_(proc) {}

std::string Interpreter::executeLine(const std::string& raw) {
  const std::string line = trim(raw);
  if (line.empty()) {
    message_.clear();                              // empty line: no message, prompt redrawn (§3.7)
    return {};
  }

  const std::size_t sep = line.find_first_of(" \t\n\r\f\v");
  const std::string cmd = sep == std::string::npos ? line : line.substr(0, sep);
  const std::string arg = sep == std::string::npos ? std::string() : trim(line.substr(sep));

  switch (lookup(cmd)) {
    case Cmd::Help:
      message_ = kHelp;
      break;

    case Cmd::Start:
      message_ = proc_.start() ? "Marquee started." : "Marquee is already running.";
      break;

    case Cmd::Stop:
      message_ = proc_.stop() ? "Marquee stopped." : "Marquee is not running.";
      break;

    case Cmd::SetText:
      if (arg.empty()) {
        message_ = "Usage: set_text <text>";
        break;
      }
      switch (params_.setText(arg)) {
        case TextResult::Ok:
          message_ = "Marquee text set to \"" + params_.text + "\".";
          break;
        case TextResult::NonAscii:
          message_ = "set_text: only printable ASCII characters are supported.";
          break;
        case TextResult::Empty:
        default:
          message_ = "Usage: set_text <text>";
          break;
      }
      break;

    case Cmd::SetSpeed: {
      int requested = 0;
      if (arg.empty() || !parseFullInteger(arg, requested)) {
        message_ = "Usage: set_speed <milliseconds>";
        break;
      }
      const ClampReport report = params_.setRefresh(requested);
      if (report.clamped) {
        message_ = "Speed " + std::to_string(report.requested) + " ms is out of range [1, 10000]; clamped to " +
                   std::to_string(report.applied) + " ms.";
      } else {
        message_ = "Marquee speed set to " + std::to_string(report.applied) + " ms.";
      }
      break;
    }

    case Cmd::Exit:
      quit_ = true;                                // read only by the input thread (§3.11)
      message_ = "Exiting CSOPESY. Goodbye!";
      break;

    case Cmd::Unknown:
    default:
      message_ = "Command not recognized: \"" + cmd + "\". Type \"help\" for the list of commands.";
      break;
  }
  return message_;
}

bool Interpreter::quitRequested() const { return quit_; }

std::string Interpreter::prompt() const { return "Command>"; }   // frozen literal (§3.9/§3.11)

std::string Interpreter::buffer() const { return line_; }

std::string Interpreter::lastMessage() const { return message_; }

}  // namespace csopesy
