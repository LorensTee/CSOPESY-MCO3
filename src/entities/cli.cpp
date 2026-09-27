// src/entities/cli.cpp — command-line option parsing.
//
// Supported options are --no-tty, --refresh-ms=N, and --poll-ms=N.
// Invalid input does not stop startup. Unknown options and invalid values produce warnings.
// Valid values outside the allowed range are clamped. When an option is repeated, the last value wins.
#include "csopesy/cli.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace csopesy {
namespace {

// Return true only when s contains a complete signed integer.
// Store the magnitude with saturation so a very large value is still treated as out of range.
bool wellFormedInteger(const std::string& s, bool& negative, unsigned long long& mag) {
  if (s.empty()) return false;
  std::size_t i = 0;
  negative = false;
  if (s[0] == '+' || s[0] == '-') {
    negative = s[0] == '-';
    i = 1;
  }
  if (i >= s.size()) return false;               // A sign without digits is not a number.
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

// Use the same parsing path for both numeric options.
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
  CliResult result;                              // Start with the built-in defaults.
  for (const std::string& token : args) {
    if (token == "--no-tty") {
      result.params.noTty = true;                // This option takes no value.
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
