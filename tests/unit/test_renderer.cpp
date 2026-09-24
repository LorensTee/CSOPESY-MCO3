// tests/unit/test_renderer.cpp — T4.2/T4.3: the plain-text frame layout (chrome rows, the band at kBandRow,
// the prompt row last), the tight-terminal priority rule (band and prompt are never dropped), and the
// one-complete-string-per-frame contract.
//
// The frame is observed as a whole string and then split back into visible rows, so these tests assert what
// Terminal::write() actually receives rather than any internal helper.
#include "check.hpp"

#include <cstddef>
#include <string>
#include <vector>

#include "csopesy/parameters.hpp"
#include "csopesy/process.hpp"
#include "csopesy/renderer.hpp"

using csopesy::MarqueeProcess;
using csopesy::Parameters;
using csopesy::Renderer;

namespace {

const char* const kCursorHome = "\x1b[H";

// One frame == cursor home, then `rows` visible rows separated by CRLF (raw mode disables ONLCR, so the
// renderer owns the carriage return). Split it back apart for assertions.
// A malformed frame (a stub, or a frame that does not start at home) must fail the test without tripping an
// exception, so the rest of the suite still runs: report the failure, then hand back blank rows to index into.
std::vector<std::string> frameLines(const std::string& frame, int expectedRows, int* rowCount) {
  if (frame.size() < 3 || frame.compare(0, 3, kCursorHome) != 0) {
    CHECK(frame.size() >= 3);
    CHECK(frame.compare(0, 3, kCursorHome) == 0);
    *rowCount = -1;
    return std::vector<std::string>(static_cast<std::size_t>(expectedRows > 0 ? expectedRows : 0), std::string());
  }
  std::string body = frame.substr(3);
  std::vector<std::string> lines;
  std::size_t start = 0;
  for (;;) {
    const std::size_t nl = body.find("\r\n", start);
    if (nl == std::string::npos) {
      lines.push_back(body.substr(start));
      break;
    }
    lines.push_back(body.substr(start, nl - start));
    start = nl + 2;
  }
  *rowCount = static_cast<int>(lines.size());
  return lines;
}

std::string trimRight(const std::string& line) {
  std::size_t end = line.size();
  while (end > 0 && line[end - 1] == ' ') --end;
  return line.substr(0, end);
}

// substr without the out_of_range throw, so a malformed frame fails an assertion instead of aborting the run.
std::string window(const std::string& line, std::size_t pos, std::size_t len) {
  if (pos >= line.size()) return {};
  return line.substr(pos, len);
}

bool allLinesAreExactly(const std::vector<std::string>& lines, int cols) {
  for (const std::string& line : lines) {
    if (static_cast<int>(line.size()) != cols) return false;
  }
  return true;
}

// cycles == bandWidth puts offset at bandWidth, i.e. text[0] in the band's first column, so the band is
// visibly non-blank without depending on the wrap boundary.
MarqueeProcess processShowingTextAtColumnZero(int cols) {
  MarqueeProcess proc;
  proc.cycles = Renderer::bandWidthFor(cols);
  return proc;
}

}  // namespace

// --- bandWidthFor: one space of margin per side, floored at one column -----------------------------------

TEST(bandWidthFor_reserves_one_column_of_margin_on_each_side) {
  CHECK_EQ(Renderer::bandWidthFor(80), 78);
  CHECK_EQ(Renderer::bandWidthFor(40), 38);
  CHECK_EQ(Renderer::bandWidthFor(7), 5);
  CHECK_EQ(Renderer::bandWidthFor(4), 2);
  CHECK_EQ(Renderer::bandWidthFor(3), 1);
  CHECK_EQ(Renderer::bandWidthFor(2), 1);
  CHECK_EQ(Renderer::bandWidthFor(1), 1);
}

TEST(kBandRow_is_fixed_at_three) {
  CHECK_EQ(Renderer::kBandRow, 3);
}

// --- T4.2: the normal frame layout -----------------------------------------------------------------------

TEST(buildFrame_normal_size_carries_the_mock_chrome_verbatim) {
  const int rows = 11, cols = 40;
  Parameters params = Parameters::defaults();
  MarqueeProcess proc = processShowingTextAtColumnZero(cols);
  const Renderer renderer;

  const std::string frame = renderer.buildFrame(params, proc, "Command>", "", "", rows, cols);
  int lineCount = 0;
  const std::vector<std::string> lines = frameLines(frame, rows, &lineCount);

  CHECK_EQ(lineCount, rows);
  CHECK(allLinesAreExactly(lines, cols));
  CHECK_STR(trimRight(lines[0]), "Welcome to CSOPESY!");   // capital W (M2)
  CHECK_STR(trimRight(lines[1]), "");
  CHECK_STR(trimRight(lines[2]), " CSOPESY");             // band: one space of margin, then text[0] in col 0
  CHECK_STR(trimRight(lines[3]), "");
  CHECK_STR(trimRight(lines[4]), "Group developer:");
  CHECK_STR(trimRight(lines[5]), "De La Cruz, Juan");
  CHECK_STR(trimRight(lines[6]), "Santos, Alex");
  CHECK_STR(trimRight(lines[7]), "");
  CHECK_STR(trimRight(lines[8]), "Version date:");         // blank version date => no trailing space
  CHECK_STR(trimRight(lines[9]), "");
  CHECK(lines[10].compare(0, 8, "Command>") == 0);         // prompt row last
}

TEST(buildFrame_band_is_on_kBandRow_and_nowhere_else) {
  const int rows = 24, cols = 30;
  Parameters params = Parameters::defaults();
  params.setText("ZQX");   // a marker the welcome line does not contain, so the search is unambiguous
  MarqueeProcess proc = processShowingTextAtColumnZero(cols);
  const Renderer renderer;

  int lineCount = 0;
  const std::vector<std::string> lines =
      frameLines(renderer.buildFrame(params, proc, "Command>", "", "", rows, cols), rows, &lineCount);

  CHECK_EQ(lineCount, rows);
  CHECK_STR(trimRight(lines[Renderer::kBandRow - 1]), " ZQX");
  for (int row = 1; row <= rows; ++row) {
    if (row == Renderer::kBandRow) continue;
    CHECK(lines[static_cast<std::size_t>(row - 1)].find("ZQX") == std::string::npos);
  }
}

TEST(buildFrame_welcome_line_keeps_the_capital_w) {
  Parameters params = Parameters::defaults();
  MarqueeProcess proc;
  const Renderer renderer;
  int lineCount = 0;
  const std::vector<std::string> lines =
      frameLines(renderer.buildFrame(params, proc, "Command>", "", "", 11, 40), 11, &lineCount);
  CHECK_STR(trimRight(lines[0]), "Welcome to CSOPESY!");
}

TEST(buildFrame_band_moves_one_column_per_cycle) {
  const int rows = 11, cols = 20;
  const int bandWidth = Renderer::bandWidthFor(cols);
  Parameters params = Parameters::defaults();   // text == "CSOPESY"
  MarqueeProcess proc;
  const Renderer renderer;

  proc.cycles = bandWidth;   // text[0] in band column 0
  int lineCount = 0;
  const std::vector<std::string> atStart =
      frameLines(renderer.buildFrame(params, proc, "Command>", "", "", rows, cols), rows, &lineCount);
  CHECK_STR(window(atStart[Renderer::kBandRow - 1], 1, 7), "CSOPESY");

  proc.cycles = bandWidth + 1;   // one column further left, so text[0] is clipped off
  const std::vector<std::string> advanced =
      frameLines(renderer.buildFrame(params, proc, "Command>", "", "", rows, cols), rows, &lineCount);
  CHECK_STR(window(advanced[Renderer::kBandRow - 1], 1, 6), "SOPESY");
}

TEST(buildFrame_clips_every_row_to_cols_so_nothing_wraps) {
  const int rows = 24, cols = 12;
  Parameters params = Parameters::defaults();
  params.developers = {"A developer name far wider than the console", "Second also long"};
  params.versionDate = "2026-09-28-with-a-long-suffix";
  MarqueeProcess proc = processShowingTextAtColumnZero(cols);
  const Renderer renderer;

  int lineCount = 0;
  const std::vector<std::string> lines = frameLines(
      renderer.buildFrame(params, proc, "Command>", "a typed line that is far too long", "a message that is too long too",
                          rows, cols),
      rows, &lineCount);
  CHECK_EQ(lineCount, rows);
  CHECK(allLinesAreExactly(lines, cols));
}

TEST(buildFrame_prompt_row_is_last_and_shows_the_buffer_tail) {
  const int rows = 11, cols = 20;
  Parameters params = Parameters::defaults();
  MarqueeProcess proc;
  const Renderer renderer;
  const std::string buffer = "0123456789ABCDEFGHIJ";   // wider than the prompt row

  int lineCount = 0;
  const std::vector<std::string> lines =
      frameLines(renderer.buildFrame(params, proc, "Command>", buffer, "", rows, cols), rows, &lineCount);

  const std::string& promptRow = lines[static_cast<std::size_t>(rows - 1)];
  CHECK(promptRow.compare(0, 8, "Command>") == 0);
  CHECK(promptRow.find('J') != std::string::npos);   // newest char visible (the cursor stays in view)
  CHECK(promptRow.find('0') == std::string::npos);   // oldest char scrolled off
}

// --- T4.3: message rows, tight terminals, full rebuild ---------------------------------------------------

TEST(buildFrame_renders_message_lines_above_the_prompt) {
  const int rows = 16, cols = 40;
  Parameters params = Parameters::defaults();
  MarqueeProcess proc;
  const Renderer renderer;

  int lineCount = 0;
  const std::vector<std::string> lines =
      frameLines(renderer.buildFrame(params, proc, "Command>", "", "Marquee started.\nsecond line", rows, cols), rows,
                 &lineCount);

  CHECK_EQ(lineCount, rows);
  CHECK_STR(trimRight(lines[static_cast<std::size_t>(rows - 3)]), "Marquee started.");
  CHECK_STR(trimRight(lines[static_cast<std::size_t>(rows - 2)]), "second line");
  CHECK(lines[static_cast<std::size_t>(rows - 1)].compare(0, 8, "Command>") == 0);
}

TEST(buildFrame_keeps_the_band_and_the_prompt_on_tight_terminals) {
  const int cols = 30;
  Parameters params = Parameters::defaults();
  const Renderer renderer;

  for (int rows = 2; rows <= 6; ++rows) {
    MarqueeProcess proc = processShowingTextAtColumnZero(cols);
    int lineCount = 0;
    const std::vector<std::string> lines =
        frameLines(renderer.buildFrame(params, proc, "Command>", "", "", rows, cols), rows, &lineCount);

    CHECK_EQ(lineCount, rows);
    CHECK(allLinesAreExactly(lines, cols));

    // The band stays at kBandRow while there is room for it, otherwise it sits directly above the block.
    const int bandRow = rows >= Renderer::kBandRow + 1 ? Renderer::kBandRow : rows - 1;
    CHECK(lines[static_cast<std::size_t>(bandRow - 1)].find("CSOPESY") != std::string::npos);
    // The prompt is never dropped and is always the last row.
    CHECK(lines[static_cast<std::size_t>(rows - 1)].compare(0, 8, "Command>") == 0);
  }
}

TEST(buildFrame_keeps_the_band_and_the_prompt_when_both_rows_and_cols_are_tiny) {
  const Renderer renderer;
  Parameters params = Parameters::defaults();
  MarqueeProcess proc;

  for (int cols = 1; cols <= 4; ++cols) {
    int lineCount = 0;
    const std::vector<std::string> lines =
        frameLines(renderer.buildFrame(params, proc, "Command>", "", "", 4, cols), 4, &lineCount);
    CHECK_EQ(lineCount, 4);
    CHECK(allLinesAreExactly(lines, cols));
    CHECK(lines[Renderer::kBandRow - 1].size() == static_cast<std::size_t>(cols));   // band row present
    CHECK(lines[3].compare(0, 1, "C") == 0);                                        // prompt row present
  }
}

TEST(buildFrame_rejects_a_degenerate_terminal_instead_of_emitting_a_broken_frame) {
  const Renderer renderer;
  Parameters params = Parameters::defaults();
  MarqueeProcess proc;
  CHECK_STR(renderer.buildFrame(params, proc, "Command>", "", "", 24, 0), "");
  CHECK_STR(renderer.buildFrame(params, proc, "Command>", "", "", 0, 80), "");
  CHECK_STR(renderer.buildFrame(params, proc, "Command>", "", "", -1, 80), "");
}

TEST(buildFrame_rebuilds_for_a_new_size_with_no_invalidation_flag) {
  const int rows = 12;
  const Renderer renderer;
  Parameters params = Parameters::defaults();
  MarqueeProcess proc;

  const std::string wide = renderer.buildFrame(params, proc, "Command>", "", "", rows, 40);
  const std::string narrow = renderer.buildFrame(params, proc, "Command>", "", "", rows, 20);

  int wideRows = 0, narrowRows = 0;
  const std::vector<std::string> wideLines = frameLines(wide, rows, &wideRows);
  const std::vector<std::string> narrowLines = frameLines(narrow, rows, &narrowRows);

  CHECK_EQ(wideRows, rows);
  CHECK_EQ(narrowRows, rows);
  CHECK(allLinesAreExactly(wideLines, 40));
  CHECK(allLinesAreExactly(narrowLines, 20));
  CHECK(wide != narrow);   // the rebuild follows the new cols immediately; no stale width survives
}

TEST(buildFrame_is_pure_so_identical_inputs_rebuild_identical_frames) {
  const Renderer renderer;
  Parameters params = Parameters::defaults();
  MarqueeProcess proc;
  proc.cycles = 5;

  const std::string first = renderer.buildFrame(params, proc, "Command>", "abc", "a message", 14, 40);
  const std::string second = renderer.buildFrame(params, proc, "Command>", "abc", "a message", 14, 40);
  CHECK_STR(first, second);
}
