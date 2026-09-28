// Parse command-line options and optional config.txt defaults.
// Apply values in this order: runtime command, command-line option, config.txt, built-in default.
// Invalid input does not stop startup.
// Unknown names and invalid values produce a warning and keep the current value.
// Values outside their allowed range are clamped and reported.
// When an option is repeated, the parser applies the last value.
#include "csopesy/cli.hpp"

#include <cstddef>
#include <fstream>
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
  if (i >= s.size()) return false;
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
    if (mag >= 2147483648ULL) return -2147483647 - 1;
    return -static_cast<int>(mag);
  }
  if (mag > 2147483647ULL) return 2147483647;
  return static_cast<int>(mag);
}

// Use the same parsing path for config values and command-line values.
// The prefix identifies the source in config-file warnings.
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

// Remove spaces, tabs, and a final carriage return.
// A file with CRLF line endings then behaves like one with LF line endings.
std::string trim(const std::string& s) {
  std::size_t begin = 0, end = s.size();
  while (begin < end && (s[begin] == ' ' || s[begin] == '\t')) ++begin;
  while (end > begin && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r')) --end;
  return s.substr(begin, end - begin);
}

}  // namespace

CliResult loadConfigFile(const std::string& path) {
  CliResult result;                       // Start with the built-in defaults.
  std::ifstream in(path, std::ios::binary);
  if (!in) return result;                 // The config file is optional.

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
  CliResult result = loadConfigFile(configPath);   // Load config defaults before CLI options.
  for (const std::string& token : args) {
    if (token == "--no-tty") {
      result.params.noTty = true;
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
