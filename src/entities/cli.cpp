// src/entities/cli.cpp — argv parsing plus the optional config.txt default layer.
// Precedence is built-in defaults < config.txt < CLI flags (runtime commands are applied later by the
// interpreter). Bad input never aborts startup: an unknown key/flag or a malformed/absent value warns and
// keeps the current value, a well-formed out-of-range value is clamped and reported, and a repeated flag's
// last value wins.
#include "csopesy/cli.hpp"

#include <cstddef>
#include <fstream>
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

// Both numeric inputs differ only in their bound, field name and spelling, so they share this path. `prefix`
// lets the config-file caller mark the source of the warning without changing the flag wording.
void applyValueFlag(CliResult& result, const std::string& name, const std::string& raw,
                    ClampReport (Parameters::*set)(int), const std::string& prefix = "") {
  bool negative = false;
  unsigned long long mag = 0;
  if (!wellFormedInteger(raw, negative, mag)) {
    result.warnings.push_back(prefix + name + "=" + raw + ": not a number; keeping the current value.");
    return;
  }
  const ClampReport report = (result.params.*set)(saturateToInt(mag, negative));
  if (report.clamped) {
    result.warnings.push_back(prefix + name + "=" + raw + " clamped to " + std::to_string(report.applied) +
                              " ms.");
  }
}

// Strips surrounding spaces/tabs and a trailing CR so CRLF files behave like LF files.
std::string trim(const std::string& s) {
  std::size_t begin = 0, end = s.size();
  while (begin < end && (s[begin] == ' ' || s[begin] == '\t')) ++begin;
  while (end > begin && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r')) --end;
  return s.substr(begin, end - begin);
}

}  // namespace

CliResult loadConfigFile(const std::string& path) {
  CliResult result;                       // the built-in defaults are the base layer
  std::ifstream in(path, std::ios::binary);
  if (!in) return result;                 // absent or unreadable: optional, so silent

  const std::string prefix = path + ": ";
  std::string line;
  while (std::getline(in, line)) {
    line = trim(line);
    if (line.empty() || line[0] == '#') continue;

    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) {
      result.warnings.push_back(prefix + "ignoring malformed line: \"" + line + "\".");
      continue;
    }
    const std::string key = trim(line.substr(0, eq));
    const std::string value = trim(line.substr(eq + 1));
    if (key.empty()) {
      result.warnings.push_back(prefix + "ignoring malformed line: \"" + line + "\".");
      continue;
    }

    if (key == "refresh_ms") {
      applyValueFlag(result, key, value, &Parameters::setRefresh, prefix);
    } else if (key == "polling_ms") {
      applyValueFlag(result, key, value, &Parameters::setPolling, prefix);
    } else {
      result.warnings.push_back(prefix + "unknown key \"" + key + "\" ignored.");
    }
  }
  return result;
}

CliResult parseCli(const std::vector<std::string>& args, const std::string& configPath) {
  CliResult result = loadConfigFile(configPath);   // config.txt sits below the flags
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
