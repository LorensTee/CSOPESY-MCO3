// include/csopesy/parameters.hpp — runtime program parameters.
//
// Parameters are applied in this order:
// runtime command, then command-line option, then config.txt, then built-in default.
// Text accepts printable ASCII only because the renderer treats one byte as one terminal column.
#pragma once
#include <string>
#include <vector>
namespace csopesy {

struct ClampReport {
  bool clamped = false;
  int requested = 0;
  int applied = 0;
  const char* field = "";
};

enum class TextResult {
  Ok,
  Empty,
  NonAscii
};

struct Parameters {
  // Marquee settings.
  std::string text = "CSOPESY";  // Set through setText(); every byte must be printable ASCII.
  int refreshMs = 100;           // Marquee frame interval in milliseconds. Range: [1, 10000].
  int pollingMs = 10;            // Maximum input wait in milliseconds. Range: [1, 1000].

  // Text shown below the marquee.
  std::vector<std::string> developers = {"Ang, Byron Scott", "Laborada, Nathan",
                                         "Sotingco, Kimbery Wynelle", "Tee, John Lorens"};
  std::string versionDate = "2026-09-27";  // Shown as "Version date: <date>". Empty is also valid.

  // Plain line mode. No raw mode, ANSI output, animation, or worker thread is used.
  // This option is for development and CI.
  bool noTty = false;

  static constexpr int kRefreshMin = 1;
  static constexpr int kRefreshMax = 10000;
  static constexpr int kPollMin = 1;
  static constexpr int kPollMax = 1000;

  // Set and clamp the marquee refresh interval.
  ClampReport setRefresh(int ms);

  // Set and clamp the input polling interval.
  ClampReport setPolling(int ms);

  // Trim outer spaces and keep internal spaces.
  // Return Empty or NonAscii without changing text when the input is invalid.
  TextResult setText(const std::string& t);

  // Return the built-in parameter values.
  static Parameters defaults();
};
}
