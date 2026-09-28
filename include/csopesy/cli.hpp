// Handles the command-line parser and config.txt settings.
// Settings are applied in this order: runtime command, CLI, config file, then defaults.
// Parser accepts these options:
//   --no-tty
//   --refresh-ms=N
//   --poll-ms=N
#pragma once
#include <string>
#include <vector>
#include "csopesy/parameters.hpp"
namespace csopesy {
struct CliResult {
  Parameters params;                  // The config.txt values or the built-in defaults, plus the option values.
  std::vector<std::string> warnings;  // One line for each invalid key or option, and for each clamped value.
};

// Loads settings from a config file.
CliResult loadConfigFile(const std::string& path);

// Parses command-line options before the terminal is set up.
CliResult parseCli(const std::vector<std::string>& args, const std::string& configPath = "config.txt");
}
