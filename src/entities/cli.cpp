// src/entities/cli.cpp — argv parsing, the only parameter layer besides the commands (§T2.2).
// Three flags — --no-tty, --refresh-ms=N, --poll-ms=N — in `--flag=value` form only. Bad input never aborts
// startup: an unknown flag or a malformed/absent value warns and keeps the current value, a well-formed
// out-of-range value is clamped and reported, and a repeated flag's last value wins.
#include "csopesy/cli.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace csopesy {
namespace {

// True when `s` matches [-+]?[0-9]+ in full. On success `mag` holds the magnitude saturated far above any
// clamp bound and `negative` reports the sign, so a huge literal is treated as out-of-range (clamped) rather
// than misclassified as malformed.
bool wellFormedInteger(const std::string& s, bool& negative, unsigned long long& mag) {
  if (s.empty()) return false;
  std::size_t i = 0;
  negative = false;
  if (s[0] == '+' || s[0] == '-') {
    negative = s[0] == '-';
    i = 1;
  }
  if (i >= s.size()) return false;               // "+" or "-" alone is not a number
  mag = 0;
  for (; i < s.size(); ++i) {
    if (s[i] < '0' || s[i] > '9') return false;
    const unsigned int digit = static_cast<unsigned int>(s[i] - '0');
    if (mag > (1000000000000000000ULL - digit) / 10ULL) mag = 1000000000000000000ULL;
    else mag = mag * 10ULL + digit;
  }
  return true;
}

int saturateToInt(unsigned long long mag, bool negative) {
  if (negative) {
    if (mag >= 2147483648ULL) return -2147483647 - 1;   // INT_MIN
    return -static_cast<int>(mag);
  }
  if (mag > 2147483647ULL) return 2147483647;           // INT_MAX
  return static_cast<int>(mag);
}

// Both numeric flags differ only in their bound, field name and spelling, so they share this path.
void applyValueFlag(CliResult& result, const std::string& flag, const std::string& raw,
                    ClampReport (Parameters::*set)(int)) {
  bool negative = false;
  unsigned long long mag = 0;
  if (!wellFormedInteger(raw, negative, mag)) {
    result.warnings.push_back(flag + "=" + raw + ": not a number; keeping the default.");
    return;
  }
  const ClampReport report = (result.params.*set)(saturateToInt(mag, negative));
  if (report.clamped) {
    result.warnings.push_back(flag + "=" + raw + " clamped to " + std::to_string(report.applied) + " ms.");
  }
}

}  // namespace

CliResult parseCli(const std::vector<std::string>& args) {
  CliResult result;                              // params start from the built-in defaults
  for (const std::string& token : args) {
    if (token == "--no-tty") {
      result.params.noTty = true;                // takes no value
    } else if (token.rfind("--refresh-ms=", 0) == 0) {
      applyValueFlag(result, "--refresh-ms", token.substr(13), &Parameters::setRefresh);
    } else if (token.rfind("--poll-ms=", 0) == 0) {
      applyValueFlag(result, "--poll-ms", token.substr(10), &Parameters::setPolling);
    } else if (token == "--refresh-ms" || token == "--poll-ms") {
      result.warnings.push_back(token + ": missing value; keeping the default.");
    } else {
      result.warnings.push_back("Unknown option ignored: \"" + token + "\".");
    }
  }
  return result;
}

}  // namespace csopesy
