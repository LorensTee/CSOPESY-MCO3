// include/csopesy/glyphs.hpp — §3.9.
#pragma once
#include <string>
#include <string_view>
#include <vector>
namespace csopesy {
constexpr int kCellCols = 5;   // every glyph row is EXACTLY 5 columns wide
constexpr int kCellRows = 5;
constexpr int kArtRows  = kCellRows;   // band height in art mode; 1 in plain mode

struct Glyph { const char* rows[kCellRows]; };
const Glyph* glyphFor(char c);          // never nullptr (unknown -> box glyph)
std::vector<std::string> renderBlockText(std::string_view text);  // kCellRows rows, 1-col gap
}
