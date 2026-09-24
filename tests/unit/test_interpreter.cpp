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

std::string run(Harness& h, const std::string& line) { return h.interp.executeLine(line); }

bool contains(const std::string& haystack, const std::string& needle) {
  return haystack.find(needle) != std::string::npos;
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

// --- T2.4 response table -----------------------------------------------------------------------------

TEST(response_table_tokens) {
  struct Row { const char* in; const char* token; };
  const Row rows[] = {
      {"help", "Available commands:"},
      {"start_marquee", "Marquee started"},
      {"start_marquee", "already running"},                 // second call, same interpreter
      {"stop_marquee", "Marquee stopped"},
      {"stop_marquee", "is not running"},
      {"set_text HELLO WORLD", "Marquee text set to \"HELLO WORLD\""},
      {"set_speed 250", "Marquee speed set to 250 ms"},
      {"set_speed 0", "out of range [1, 10000]; clamped to 1 ms"},
      {"foo", "Command not recognized: \"foo\""},
      {"HELP", "Command not recognized"},                   // case-sensitive, per §3.7
  };
  Harness h;
  for (const auto& r : rows) {
    CHECK(contains(run(h, r.in), r.token));
  }
}

TEST(usage_lines_are_exact) {
  Harness h;
  CHECK_STR(run(h, "set_text"), "Usage: set_text <text>");
  CHECK_STR(run(h, "set_speed"), "Usage: set_speed <milliseconds>");
  CHECK_STR(run(h, "set_speed abc"), "Usage: set_speed <milliseconds>");
  CHECK_STR(run(h, "set_speed 5abc"), "Usage: set_speed <milliseconds>");   // no partial parse
  CHECK_STR(run(h, "set_speed 1.5"), "Usage: set_speed <milliseconds>");
  CHECK_STR(run(h, "set_speed +"), "Usage: set_speed <milliseconds>");
  CHECK_STR(run(h, "set_speed -"), "Usage: set_speed <milliseconds>");
  CHECK_STR(run(h, "set_speed 100 extra"), "Usage: set_speed <milliseconds>");
}

TEST(negative_speed_is_clamped_not_a_usage_error) {
  Harness h;
  CHECK_STR(run(h, "set_speed -5"), "Speed -5 ms is out of range [1, 10000]; clamped to 1 ms.");
  CHECK_EQ(h.params.refreshMs, Parameters::kRefreshMin);
}

TEST(speed_above_max_is_clamped_with_the_applied_value) {
  Harness h;
  CHECK_STR(run(h, "set_speed 10001"), "Speed 10001 ms is out of range [1, 10000]; clamped to 10000 ms.");
  CHECK_EQ(h.params.refreshMs, Parameters::kRefreshMax);
}

TEST(huge_speed_is_clamped_and_does_not_crash) {
  Harness h;
  CHECK(contains(run(h, "set_speed 99999999999999999999"), "out of range"));
  CHECK_EQ(h.params.refreshMs, Parameters::kRefreshMax);
}

TEST(plus_signed_and_boundary_speeds_apply) {
  Harness h;
  CHECK_STR(run(h, "set_speed +250"), "Marquee speed set to 250 ms.");
  CHECK_EQ(h.params.refreshMs, 250);
  CHECK_STR(run(h, "set_speed 1"), "Marquee speed set to 1 ms.");
  CHECK_STR(run(h, "set_speed 10000"), "Marquee speed set to 10000 ms.");
}

TEST(help_lists_every_required_command_and_description) {
  Harness h;
  const std::string help = run(h, "help");
  CHECK(contains(help, "Available commands:"));
  for (const char* c : {"help", "start_marquee", "stop_marquee", "set_text", "set_speed", "exit"}) {
    CHECK(contains(help, c));
  }
  CHECK(contains(help, "displays the commands and its description"));
  CHECK(contains(help, "terminates the console"));
}

TEST(argument_whitespace_rule) {
  Harness h;
  (void)run(h, "   set_text   HELLO   WORLD  ");
  CHECK_STR(h.params.text, "HELLO   WORLD");        // internal run kept, surroundings trimmed
}

TEST(set_text_message_is_exact) {
  Harness h;
  CHECK_STR(run(h, "set_text HELLO WORLD"), "Marquee text set to \"HELLO WORLD\".");
}

TEST(set_text_empty_is_a_usage_error_and_unchanged) {
  Harness h;
  h.params.setText("KEEP");
  CHECK_STR(run(h, "set_text"), "Usage: set_text <text>");
  CHECK_STR(h.params.text, "KEEP");
  CHECK_STR(run(h, "set_text    "), "Usage: set_text <text>");
  CHECK_STR(h.params.text, "KEEP");
}

TEST(set_text_non_ascii_is_rejected_and_unchanged) {
  Harness h;
  h.params.setText("KEEP");
  CHECK_STR(run(h, "set_text caf\xC3\xA9"), "set_text: only printable ASCII characters are supported.");
  CHECK_STR(h.params.text, "KEEP");
  CHECK_STR(run(h, "set_text \xE6\x97\xA5\xE6\x9C\xAC"), "set_text: only printable ASCII characters are supported.");
  CHECK_STR(h.params.text, "KEEP");
}

TEST(commands_are_case_sensitive) {
  Harness h;
  for (const char* c : {"HELP", "Start_Marquee", "SET_TEXT", "Exit"}) {
    CHECK(contains(run(h, c), "Command not recognized"));
  }
  CHECK(!h.interp.quitRequested());                 // "Exit" did not terminate
}

TEST(control_command_whitespace_does_not_change_matching) {
  Harness h;
  CHECK(contains(run(h, "   start_marquee   "), "Marquee started"));
  CHECK(contains(run(h, "start marquee"), "Command not recognized"));   // "start" is not a command
  CHECK_STR(h.params.text, "CSOPESY");                                  // no accidental set_text
}

TEST(exit_sets_quit_flag_and_prints_goodbye) {
  Harness h;
  CHECK_STR(run(h, "exit"), "Exiting CSOPESY. Goodbye!");
  CHECK(h.interp.quitRequested());
}

TEST(empty_and_long_input_do_not_crash) {
  Harness h;
  CHECK_STR(run(h, "   "), "");
  CHECK_STR(run(h, ""), "");
  CHECK(contains(run(h, "set_text " + std::string(5000, 'X')), "Marquee text set to"));
  CHECK(contains(run(h, std::string(5000, 'a')), "Command not recognized"));
}

TEST(last_message_reflects_the_last_command) {
  Harness h;
  (void)run(h, "help");
  CHECK(contains(h.interp.lastMessage(), "Available commands:"));
  (void)run(h, "   ");                              // empty line: no message
  CHECK_STR(h.interp.lastMessage(), "");
}
