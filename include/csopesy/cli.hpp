// include/csopesy/cli.hpp — command-line options.
//
// The parser accepts three options:
//   --no-tty
//   --refresh-ms=N
//   --poll-ms=N
//
// Invalid input produces a warning and does not stop startup.
// A valid value outside its range is clamped.
// When an option appears more than once, the last value wins.
#pragma once
#include <string>
#include <vector>
#include "csopesy/parameters.hpp"
namespace csopesy {
struct CliResult {
  Parameters params;                  // Contains the defaults and all accepted option values.
  std::vector<std::string> warnings;  // One warning for each invalid option or clamped value.
};

// args does not include argv[0].
// Accept only the --flag=value form for options that take a value.
// --no-tty is for plain line mode and must be known before the terminal enters raw mode.
CliResult parseCli(const std::vector<std::string>& args);
}
