// tests/unit/test_scroll.cpp — T4.1: scrollOffset wraps at textWidth + bandWidth and stays in
// [0, textWidth + bandWidth); sliceRow is always exactly bandWidth bytes, for short, equal, long and empty text.
//
// The v3 text contract is ASCII-only (D15), so one byte == one visible column and the only width arithmetic
// this file needs is string length.
#include "check.hpp"

#include <cstddef>
#include <string>

#include "csopesy/renderer.hpp"

using csopesy::scrollOffset;
using csopesy::sliceRow;

// --- scrollOffset: one column per cycle, wrap period textWidth + bandWidth ------------------------------

TEST(scrollOffset_starts_at_zero_and_advances_one_column_per_cycle) {
  const int textWidth = 5, bandWidth = 3, period = textWidth + bandWidth;
  for (int cycles = 0; cycles < period; ++cycles) {
    CHECK_EQ(scrollOffset(cycles, textWidth, bandWidth), cycles);
  }
}

TEST(scrollOffset_wraps_at_the_text_width_plus_band_width_period) {
  const int textWidth = 7, bandWidth = 38, period = textWidth + bandWidth;
  CHECK_EQ(scrollOffset(period, textWidth, bandWidth), 0);
  CHECK_EQ(scrollOffset(period + 1, textWidth, bandWidth), 1);
  CHECK_EQ(scrollOffset(4 * period + 5, textWidth, bandWidth), 5);
}

TEST(scrollOffset_always_lands_in_range_for_large_and_negative_cycles) {
  const int textWidth = 7, bandWidth = 5, period = textWidth + bandWidth;
  for (long long cycles = -50; cycles <= 50; ++cycles) {
    const int offset = scrollOffset(cycles, textWidth, bandWidth);
    CHECK(offset >= 0);
    CHECK(offset < period);
  }
  CHECK_EQ(scrollOffset(-1, textWidth, bandWidth), period - 1);
  CHECK_EQ(scrollOffset(-period, textWidth, bandWidth), 0);
  CHECK_EQ(scrollOffset(1000000007LL, textWidth, bandWidth), static_cast<int>(1000000007LL % period));
}

TEST(scrollOffset_never_skips_or_repeats_a_column) {
  const int textWidth = 7, bandWidth = 4, period = textWidth + bandWidth;
  for (int cycles = 0; cycles < 4 * period; ++cycles) {
    const int now = scrollOffset(cycles, textWidth, bandWidth);
    const int next = scrollOffset(cycles + 1, textWidth, bandWidth);
    CHECK_EQ(next, (now + 1) % period);
  }
}

TEST(scrollOffset_degenerate_periods_do_not_divide_by_zero) {
  CHECK_EQ(scrollOffset(3, 0, 0), 0);      // empty text in a zero-width band
  CHECK_EQ(scrollOffset(9, 0, -2), 0);     // period would be negative
  CHECK_EQ(scrollOffset(7, 0, 4), 3);      // empty text still advances through the band
}

// --- sliceRow: the always-exactly-bandWidth invariant ----------------------------------------------------

TEST(sliceRow_offset_zero_draws_the_fully_blank_band) {
  CHECK_STR(sliceRow("ABCDE", 3, 0), "   ");
  CHECK_STR(sliceRow("CSOPESY", 40, 0), std::string(40, ' '));
}

TEST(sliceRow_offset_equal_to_band_width_puts_text_zero_in_column_zero) {
  CHECK_STR(sliceRow("ABCDE", 3, 3), "ABC");
  CHECK_STR(sliceRow("CSOPESY", 7, 7), "CSOPESY");
}

TEST(sliceRow_windows_the_expected_columns_at_every_offset) {
  CHECK_STR(sliceRow("ABCDE", 3, 1), "  A");
  CHECK_STR(sliceRow("ABCDE", 3, 2), " AB");
  CHECK_STR(sliceRow("ABCDE", 3, 4), "BCD");
  CHECK_STR(sliceRow("ABCDE", 3, 5), "CDE");
  CHECK_STR(sliceRow("ABCDE", 3, 6), "DE ");
  CHECK_STR(sliceRow("ABCDE", 3, 7), "E  ");
}

TEST(sliceRow_empty_text_is_a_blank_band) {
  CHECK_STR(sliceRow("", 5, 0), "     ");
  CHECK_STR(sliceRow("", 5, 3), "     ");
  CHECK_EQ(sliceRow("", 5, 0).size(), 5u);
}

TEST(sliceRow_pads_short_text_and_windows_long_text) {
  CHECK_STR(sliceRow("AB", 6, 6), "AB    ");
  CHECK_STR(sliceRow("0123456789", 4, 4), "0123");
  CHECK_STR(sliceRow("0123456789", 4, 12), "89  ");
}

TEST(sliceRow_degenerate_band_widths_return_an_empty_string) {
  CHECK_STR(sliceRow("ABCDE", 0, 1), "");
  CHECK_STR(sliceRow("ABCDE", -3, 1), "");
}

TEST(sliceRow_result_is_exactly_band_width_bytes_for_every_offset) {
  const int widths[] = {1, 2, 3, 7, 8, 9, 40};
  const std::string texts[] = {"", "A", "ABCDE", "CSOPESY", "0123456789ABCDEF"};
  for (int bandWidth : widths) {
    for (const std::string& text : texts) {
      const int textWidth = static_cast<int>(text.size());
      const int period = textWidth + bandWidth;
      // Walk a little past the wrap boundary as well, so the boundary itself is covered.
      for (int cycles = 0; cycles <= period + 2; ++cycles) {
        const std::string row = sliceRow(text, bandWidth, scrollOffset(cycles, textWidth, bandWidth));
        CHECK_EQ(static_cast<int>(row.size()), bandWidth);
      }
    }
  }
}

TEST(sliceRow_reads_no_byte_outside_the_window_edges) {
  // offset - bandWidth is the band's left edge in text coordinates; every byte must come from [0, textWidth)
  // or be the padding space. This is the "never indexes out of range" case, checked at both edges.
  const int bandWidth = 4;
  const std::string text = "ABC";
  const int textWidth = static_cast<int>(text.size());
  for (int offset = 0; offset < textWidth + bandWidth; ++offset) {
    const std::string row = sliceRow(text, bandWidth, offset);
    for (int col = 0; col < bandWidth; ++col) {
      const int index = offset - bandWidth + col;
      const char expected = (index >= 0 && index < textWidth) ? text[static_cast<std::size_t>(index)] : ' ';
      CHECK_EQ(row[static_cast<std::size_t>(col)], expected);
    }
  }
}
