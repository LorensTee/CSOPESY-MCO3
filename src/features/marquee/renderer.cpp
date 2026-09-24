// src/features/marquee/renderer.cpp — the plain-text marquee: scroll math + frame assembly (§3.9).
//
// One frame is ONE string, built fresh every time and handed to Terminal::write(). There is no FrameBuffer and
// no diffing: a resize needs no invalidation flag, only the next frame, because the caller's rows/cols are the
// frame's rows/cols.
#include "csopesy/renderer.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace csopesy {
namespace {

// The frame opens at the home cell. Raw mode clears ONLCR on POSIX and Windows sets
// DISABLE_NEWLINE_AUTO_RETURN, so the renderer owns its carriage returns: rows are separated by CRLF.
constexpr char kCursorHome[] = "\x1b[H";
constexpr char kRowSeparator[] = "\r\n";

std::string padOrClip(const std::string& line, int cols) {
  if (cols <= 0) return {};
  if (static_cast<int>(line.size()) >= cols) return line.substr(0, static_cast<std::size_t>(cols));
  return line + std::string(static_cast<std::size_t>(cols) - line.size(), ' ');
}

std::vector<std::string> splitLines(const std::string& text) {
  std::vector<std::string> lines;
  if (text.empty()) return lines;
  std::size_t start = 0;
  for (;;) {
    const std::size_t newline = text.find('\n', start);
    if (newline == std::string::npos) {
      lines.push_back(text.substr(start));
      break;
    }
    lines.push_back(text.substr(start, newline - start));
    start = newline + 1;
  }
  return lines;
}

// The prompt row shows the TAIL of the buffer so the cursor stays visible on one fixed row (§3.11). Copying
// that rule here is deliberate: visibleSlice lives in the commands slice, and features/ may not cross-import,
// so the marquee owns the window it draws.
std::string buildPromptRow(const std::string& prompt, const std::string& buffer, int cols) {
  const int available = cols - static_cast<int>(prompt.size()) - 1;
  std::string window;
  if (available > 0 && !buffer.empty()) {
    const std::size_t width = static_cast<std::size_t>(available);
    window = buffer.size() <= width ? buffer : buffer.substr(buffer.size() - width);
  }
  return padOrClip(prompt + " " + window, cols);
}

}  // namespace

int scrollOffset(long long cycles, int textWidth, int bandWidth) {
  const long long period = static_cast<long long>(textWidth) + bandWidth;
  if (period <= 0) return 0;   // empty text in an empty band: there is nothing to move
  const long long wrapped = cycles % period;
  return static_cast<int>(wrapped < 0 ? wrapped + period : wrapped);
}

std::string sliceRow(std::string_view text, int bandWidth, int offset) {
  if (bandWidth <= 0) return {};
  // offset is the band's left edge in text coordinates shifted by one band width, so offset == 0 is one whole
  // band past the text's right edge (a blank band) and offset == bandWidth lands text[0] in column 0.
  const int windowLeft = offset - bandWidth;
  const int textWidth = static_cast<int>(text.size());
  std::string row;
  row.reserve(static_cast<std::size_t>(bandWidth));
  for (int col = 0; col < bandWidth; ++col) {
    const int index = windowLeft + col;
    row.push_back((index >= 0 && index < textWidth) ? text[static_cast<std::size_t>(index)] : ' ');
  }
  return row;
}

std::string Renderer::buildFrame(const Parameters& params, const MarqueeProcess& proc,
                                 const std::string& prompt, const std::string& buffer,
                                 const std::string& message, int rows, int cols) const {
  if (rows <= 0 || cols <= 0) return {};

  const int bandWidth = bandWidthFor(cols);
  const int offset = scrollOffset(proc.cycles, static_cast<int>(params.text.size()), bandWidth);
  const std::string band = sliceRow(params.text, bandWidth, offset);

  // The band is pinned at kBandRow while the terminal is tall enough; a shorter terminal keeps it directly
  // above the message/prompt block instead of dropping it, because a missing band fails the assignment's only
  // visual requirement (§3.9).
  const int bandRow = rows >= kBandRow + 1 ? kBandRow : (rows >= 2 ? rows - 1 : 0);

  // The message and the prompt are bottom-anchored, so the newest response lines survive a tight `rows`.
  std::vector<std::string> messageLines = splitLines(message);
  int messageCount = static_cast<int>(messageLines.size());
  const int messageBudget = bandRow > 0 ? rows - bandRow - 1 : 0;
  if (messageCount > messageBudget) {
    messageLines.erase(messageLines.begin(), messageLines.begin() + (messageCount - messageBudget));
    messageCount = messageBudget;
  }

  const int blockTop = rows - messageCount;       // 1-based row of the first message line
  const int separatorRow = blockTop - 1;          // the blank line above the block
  const bool hasSeparator = bandRow > 0 && separatorRow > bandRow;

  std::vector<std::string> frame(static_cast<std::size_t>(rows));

  if (bandRow > 0) {
    frame[static_cast<std::size_t>(bandRow - 1)] = " " + band + " ";            // one space of margin per side
    if (bandRow >= 2) frame[0] = "Welcome to CSOPESY!";                          // capital W (§1.4 / M2)
  }

  // Chrome below the band, dropped from the bottom up in the plan's priority order: `Version date:`, then the
  // developer names, then `Group developer:` (§3.9).
  std::vector<std::string> chrome;
  chrome.push_back("");
  chrome.push_back("Group developer:");
  for (const std::string& developer : params.developers) chrome.push_back(developer);
  chrome.push_back("");
  chrome.push_back(params.versionDate.empty() ? std::string("Version date:")
                                              : "Version date: " + params.versionDate);

  const int afterFirst = bandRow + 1;                                     // 1-based
  const int afterLast = (hasSeparator ? separatorRow : blockTop) - 1;     // 1-based
  int capacity = afterLast - afterFirst + 1;
  if (capacity < 0) capacity = 0;
  while (static_cast<int>(chrome.size()) > capacity) chrome.pop_back();

  for (int i = 0; i < static_cast<int>(chrome.size()); ++i) {
    frame[static_cast<std::size_t>(afterFirst - 1 + i)] = chrome[static_cast<std::size_t>(i)];
  }
  for (int i = 0; i < messageCount; ++i) {
    frame[static_cast<std::size_t>(blockTop - 1 + i)] = messageLines[static_cast<std::size_t>(i)];
  }

  frame[static_cast<std::size_t>(rows - 1)] = buildPromptRow(prompt, buffer, cols);

  // Padding every row (including the gaps) is what makes the frame's width equal the terminal's width: a full
  // rebuild leaves no stale columns behind, and no row can ever wrap onto the row below it.
  for (std::string& line : frame) line = padOrClip(line, cols);

  std::string out(kCursorHome);
  for (int row = 0; row < rows; ++row) {
    if (row > 0) out += kRowSeparator;
    out += frame[static_cast<std::size_t>(row)];
  }
  return out;
}

int Renderer::bandWidthFor(int cols) {
  return cols > 2 ? cols - 2 : 1;   // max(1, cols - 2): one space of margin on each side
}

}  // namespace csopesy
