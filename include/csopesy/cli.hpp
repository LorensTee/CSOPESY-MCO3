// include/csopesy/cli.hpp — command-line options and the optional config.txt defaults.
//
// Parameters are applied in this order:
// runtime command, then command-line option, then config.txt, then built-in default.
//
// The parser accepts three options:
//   --no-tty
//   --refresh-ms=N
//   --poll-ms=N
//
// Invalid input produces a warning and does not stop startup.
// A valid value outside its range is clamped.
// When an option appears more than once, the parser applies the last value.
#pragma once
#include <string>
#include <vector>
#include "csopesy/parameters.hpp"
namespace csopesy {
struct CliResult {
  Parameters params;                  // The config.txt values or the built-in defaults, plus the option values.
  std::vector<std::string> warnings;  // One line for each invalid key or option, and for each clamped value.
};

// Read a key=value file and return its parameter values.
// Ignore blank lines and lines whose first character is #.
// Remove spaces and tabs around keys and values.
// The keys refresh_ms and polling_ms use the ranges of their matching options.
// Unknown keys and malformed lines or values produce a warning and are ignored.
// Out-of-range values are clamped and reported.
// A missing or unreadable file returns the built-in defaults with no warning.
// parseCli calls this function for config.txt. Callers may specify another file path.
CliResult loadConfigFile(const std::string& path);

// args does not include argv[0].
// Accept only the --flag=value form for options that take a value.
// --no-tty is for plain line mode and must be known before the terminal enters raw mode.
// configPath defaults to config.txt in the working directory.
// Parsing happens before Terminal::create() and before any thread starts, because noTty must be known
// before raw mode is attempted.
CliResult parseCli(const std::vector<std::string>& args, const std::string& configPath = "config.txt");
}
