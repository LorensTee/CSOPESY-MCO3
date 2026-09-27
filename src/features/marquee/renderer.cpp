// src/features/marquee/renderer.cpp — marquee scroll math and frame assembly.
//
// Each frame is one string. The renderer builds the whole frame for the current terminal size.
// A resize therefore needs no separate invalidation step.

#include "csopesy/renderer.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace csopesy {
namespace {

// Start each frame at the home cell. Raw mode disables newline translation, so the renderer
// uses CRLF between rows.
constexpr char kCursorHome[] = "\x1b[H";
constexpr char kRowSeparator[] = "\r\n";
constexpr char kEraseToEol[] = "\x1b[K";

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

// Show the tail of the input buffer so the cursor stays visible on one row.
//
// The prompt is the last frame row. Leave one column unused so the terminal never writes
// to its bottom-right cell.
std::string buildPromptRow(const std::string& prompt, const std::string& buffer, int cols) {
  const int width = cols > 1 ? cols - 1 : 0;
  const int available = width - static_cast<int>(prompt.size()) - 1;
  std::string window;
  if (available > 0 && !buffer.empty()) {
    const std::size_t w = static_cast<std::size_t>(available);
    window = buffer.size() <= w ? buffer : buffer.substr(buffer.size() - w);
  }
  return padOrClip(prompt + " " + window, width);
}

}  // namespace

int scrollOffset(long long cycles, int textWidth, int bandWidth) {
  const long long period = static_cast<long long>(textWidth) + bandWidth;
  if (period <= 0) return 0;   // No text or band width means there is nothing to move.
  const long long wrapped = cycles % period;
  return static_cast<int>(wrapped < 0 ? wrapped + period : wrapped);
}

std::string sliceRow(std::string_view text, int bandWidth, int offset) {
  if (bandWidth <= 0) return {};

  // offset is the text position shifted by one band width.
  // offset == 0 shows a blank band; offset == bandWidth shows text[0] in column 0.
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

  // Keep the band near the top on normal terminals. On short terminals, place it directly above
  // the message and prompt so the band remains visible.
  const int bandRow = rows >= kBandRow + 1 ? kBandRow : (rows >= 2 ? rows - 1 : 0);

  // Keep the newest message lines when the terminal is too short for the full message.
  std::vector<std::string> messageLines = splitLines(message);
  int messageCount = static_cast<int>(messageLines.size());
  const int messageBudget = bandRow > 0 ? rows - bandRow - 1 : 0;
  if (messageCount > messageBudget) {
    messageLines.erase(messageLines.begin(), messageLines.begin() + (messageCount - messageBudget));
    messageCount = messageBudget;
  }

  const int blockTop = rows - messageCount;       // 1-based row of the first message line.
  const int separatorRow = blockTop - 1;          // Blank row above the message block.
  const bool hasSeparator = bandRow > 0 && separatorRow > bandRow;

  std::vector<std::string> frame(static_cast<std::size_t>(rows));

  if (bandRow > 0) {
    frame[static_cast<std::size_t>(bandRow - 1)] = " " + band + " ";            // One space on each side.
    if (bandRow >= 2) frame[0] = "Welcome to CSOPESY!";
  }

  // Add the fixed text below the band. Remove lower-priority rows first when the terminal is short.
  std::vector<std::string> chrome;
  chrome.push_back("");
  chrome.push_back("Group developer:");
  for (const std::string& developer : params.developers) chrome.push_back(developer);
  chrome.push_back("");
  chrome.push_back(params.versionDate.empty() ? std::string("Version date:")
                                              : "Version date: " + params.versionDate);

  const int afterFirst = bandRow + 1;                                     // 1-based.
  const int afterLast = (hasSeparator ? separatorRow : blockTop) - 1;     // 1-based.
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

  // Pad every row to the terminal width. This prevents stale text and prevents row wrapping.
  for (std::string& line : frame) line = padOrClip(line, cols);

  // Leave the last cell unused and erase to the end of the line.
  // Writing to the bottom-right cell can scroll the terminal and leave old frames in scrollback.
  std::string out(kCursorHome);
  for (int row = 0; row < rows; ++row) {
    if (row > 0) out += kRowSeparator;
    if (row == rows - 1) {
      out += padOrClip(frame[static_cast<std::size_t>(row)], cols > 1 ? cols - 1 : 0);
      out += kEraseToEol;
    } else {
      out += frame[static_cast<std::size_t>(row)];
    }
  }
  return out;
}

int Renderer::bandWidthFor(int cols) {
  return cols > 2 ? cols - 2 : 1;   // One space of margin on each side.
}

}  // namespace csopesy
