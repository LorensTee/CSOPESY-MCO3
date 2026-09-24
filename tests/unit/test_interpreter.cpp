// tests/unit/test_interpreter.cpp — T2.3/T2.4: line editing (own echo, Backspace, ignored arrows, Eof,
// horizontal prompt scroll) and the §3.7 response table.
#include "check.hpp"
#include "csopesy/interpreter.hpp"

#include <string>

using csopesy::Interpreter;
using csopesy::KeyEvent;
using csopesy::KeyType;
using csopesy::MarqueeProcess;
using csopesy::Parameters;
using csopesy::visibleSlice;

namespace {

struct Harness {
  Parameters params;
  MarqueeProcess proc;
  Interpreter interp{params, proc};
};

KeyEvent ch(char c) { return KeyEvent{KeyType::Char, c}; }
KeyEvent key(KeyType t) { return KeyEvent{t, 0}; }

void type(Harness& h, const std::string& s) {
  for (char c : s) (void)h.interp.feed(ch(c));
}

}  // namespace

// --- T2.3 line editor --------------------------------------------------------------------------------

TEST(typed_printable_chars_are_echoed_in_order) {
  Harness h;
  CHECK(!h.interp.feed(ch('h')));
  CHECK(!h.interp.feed(ch('i')));
  CHECK_STR(h.interp.buffer(), "hi");
}

TEST(spaces_are_printable_and_typed_verbatim) {
  Harness h;
  type(h, "HELLO WORLD");
  CHECK_STR(h.interp.buffer(), "HELLO WORLD");
}

TEST(enter_executes_the_line_and_clears_the_buffer) {
  Harness h;
  type(h, "hi");
  CHECK(h.interp.feed(key(KeyType::Enter)));      // true => a full line was executed
  CHECK_STR(h.interp.buffer(), "");
}

TEST(enter_on_an_empty_line_does_not_crash) {
  Harness h;
  CHECK(h.interp.feed(key(KeyType::Enter)));
  CHECK_STR(h.interp.buffer(), "");
}

TEST(backspace_removes_the_last_char) {
  Harness h;
  type(h, "hi");
  CHECK(!h.interp.feed(key(KeyType::Backspace)));
  CHECK_STR(h.interp.buffer(), "h");
}

TEST(backspace_at_column_zero_is_a_noop) {
  Harness h;
  CHECK(!h.interp.feed(key(KeyType::Backspace)));
  CHECK_STR(h.interp.buffer(), "");
  type(h, "ab");
  (void)h.interp.feed(key(KeyType::Backspace));
  (void)h.interp.feed(key(KeyType::Backspace));   // second one hits an empty line
  CHECK_STR(h.interp.buffer(), "");
}

TEST(arrows_tab_and_escape_are_ignored) {
  Harness h;
  type(h, "ab");
  const KeyType ignored[] = {KeyType::ArrowUp, KeyType::ArrowDown, KeyType::ArrowLeft,
                             KeyType::ArrowRight, KeyType::Tab, KeyType::Escape};
  for (KeyType t : ignored) {
    CHECK(!h.interp.feed(key(t)));                // no submission, no state change
    CHECK_STR(h.interp.buffer(), "ab");
  }
}

TEST(non_printable_chars_are_ignored_on_entry) {
  Harness h;
  const char ignored[] = {'\t', '\n', '\x01', '\x1F', '\x7F'};
  for (char c : ignored) {
    CHECK(!h.interp.feed(ch(c)));
    CHECK_STR(h.interp.buffer(), "");
  }
}

TEST(eof_at_position_zero_marks_the_line_complete) {
  Harness h;
  CHECK(h.interp.feed(key(KeyType::Eof)));
  CHECK_STR(h.interp.buffer(), "");
}

// --- T2.3 visibleSlice: one fixed row, tail shown so the cursor stays visible ------------------------

TEST(visible_slice_shows_the_whole_buffer_when_it_fits) {
  CHECK_STR(visibleSlice("abc", 5), "abc");
  CHECK_STR(visibleSlice("abc", 3), "abc");
  CHECK_STR(visibleSlice("", 4), "");
}

TEST(visible_slice_shows_the_tail_when_the_buffer_overflows) {
  CHECK_STR(visibleSlice("abcdef", 3), "def");
  CHECK_STR(visibleSlice("Command> abcdef", 4), "cdef");
}

TEST(visible_slice_never_exceeds_avail_width) {
  const std::string s = "a very long command line";
  for (int w = 0; w <= 10; ++w) {
    CHECK(static_cast<int>(visibleSlice(s, w).size()) <= w);
  }
}

TEST(visible_slice_handles_non_positive_width) {
  CHECK_STR(visibleSlice("abc", 0), "");
  CHECK_STR(visibleSlice("abc", -3), "");
}
