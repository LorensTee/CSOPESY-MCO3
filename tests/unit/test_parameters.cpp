// tests/unit/test_parameters.cpp — T2.1: clamping with explicit ClampReports, setText's trim + internal-run
// rules, and the three TextResult outcomes (Ok / Empty / NonAscii) with the state unchanged on refusal.
#include "check.hpp"
#include "csopesy/parameters.hpp"

using csopesy::ClampReport;
using csopesy::Parameters;
using csopesy::TextResult;

TEST(defaults_match_the_handout_mock) {
  const Parameters p = Parameters::defaults();
  CHECK_STR(p.text, "CSOPESY");
  CHECK_EQ(p.refreshMs, 100);
  CHECK_EQ(p.pollingMs, 10);
  CHECK(!p.noTty);
  CHECK_EQ(p.developers.size(), size_t{2});
  CHECK_STR(p.developers[0], "De La Cruz, Juan");
  CHECK_STR(p.versionDate, "");
}

// --- setRefresh: clamp + explicit report -------------------------------------------------------------

TEST(set_refresh_zero_is_clamped_to_one) {
  Parameters p;
  const ClampReport r = p.setRefresh(0);
  CHECK(r.clamped);
  CHECK_EQ(r.requested, 0);
  CHECK_EQ(r.applied, 1);
  CHECK_STR(std::string(r.field), "refreshMs");
  CHECK_EQ(p.refreshMs, Parameters::kRefreshMin);
}

TEST(set_refresh_negative_is_clamped_to_one) {
  Parameters p;
  const ClampReport r = p.setRefresh(-5);       // A5: a well-formed negative clamps, never a usage error
  CHECK(r.clamped);
  CHECK_EQ(r.requested, -5);
  CHECK_EQ(r.applied, 1);
  CHECK_EQ(p.refreshMs, 1);
}

TEST(set_refresh_in_range_is_not_clamped) {
  Parameters p;
  const ClampReport r = p.setRefresh(250);
  CHECK(!r.clamped);
  CHECK_EQ(r.requested, 250);
  CHECK_EQ(r.applied, 250);
  CHECK_STR(std::string(r.field), "refreshMs");
  CHECK_EQ(p.refreshMs, 250);
}

TEST(set_refresh_bounds_are_inclusive) {
  Parameters p;
  CHECK(!p.setRefresh(Parameters::kRefreshMin).clamped);
  CHECK_EQ(p.refreshMs, 1);
  CHECK(!p.setRefresh(Parameters::kRefreshMax).clamped);
  CHECK_EQ(p.refreshMs, 10000);
}

TEST(set_refresh_above_max_is_clamped_to_max) {
  Parameters p;
  const ClampReport r = p.setRefresh(10001);
  CHECK(r.clamped);
  CHECK_EQ(r.requested, 10001);
  CHECK_EQ(r.applied, 10000);
  CHECK_EQ(p.refreshMs, Parameters::kRefreshMax);
}

// --- setPolling: same contract, different range -------------------------------------------------------

TEST(set_polling_zero_is_clamped_to_one) {
  Parameters p;
  const ClampReport r = p.setPolling(0);
  CHECK(r.clamped);
  CHECK_EQ(r.requested, 0);
  CHECK_EQ(r.applied, 1);
  CHECK_STR(std::string(r.field), "pollingMs");
  CHECK_EQ(p.pollingMs, 1);
}

TEST(set_polling_in_range_is_not_clamped) {
  Parameters p;
  const ClampReport r = p.setPolling(10);
  CHECK(!r.clamped);
  CHECK_EQ(r.applied, 10);
  CHECK_EQ(p.pollingMs, 10);
}

TEST(set_polling_above_max_is_clamped_to_max) {
  Parameters p;
  const ClampReport r = p.setPolling(2000);
  CHECK(r.clamped);
  CHECK_EQ(r.requested, 2000);
  CHECK_EQ(r.applied, 1000);
  CHECK_STR(std::string(r.field), "pollingMs");
  CHECK_EQ(p.pollingMs, Parameters::kPollMax);
}

// --- setText: trim, keep internal runs, three outcomes -----------------------------------------------

TEST(set_text_trims_and_preserves_internal_runs) {
  Parameters p;
  const TextResult r = p.setText("  HELLO   WORLD  ");
  CHECK(r == TextResult::Ok);
  CHECK_STR(p.text, "HELLO   WORLD");           // surroundings trimmed, internal run verbatim (§3.7 rule 4)
}

TEST(set_text_single_word_and_punctuation) {
  Parameters p;
  CHECK(p.setText("hello!") == TextResult::Ok);
  CHECK_STR(p.text, "hello!");
  CHECK(p.setText("a,b;c.") == TextResult::Ok);
  CHECK_STR(p.text, "a,b;c.");
}

TEST(set_text_accepts_the_printable_ascii_bounds) {
  Parameters p;
  CHECK(p.setText(" ~") == TextResult::Ok);     // 0x20 and 0x7E are both inside [0x20, 0x7E]
  CHECK_STR(p.text, "~");
}

TEST(set_text_empty_is_refused_and_state_unchanged) {
  Parameters p;
  p.setText("KEEP");
  CHECK(p.setText("") == TextResult::Empty);
  CHECK_STR(p.text, "KEEP");
  CHECK(p.setText("     ") == TextResult::Empty);
  CHECK_STR(p.text, "KEEP");
}

TEST(set_text_non_ascii_is_refused_and_state_unchanged) {
  Parameters p;
  p.setText("KEEP");
  CHECK(p.setText("caf\xC3\xA9") == TextResult::NonAscii);          // UTF-8 'é'
  CHECK_STR(p.text, "KEEP");
  CHECK(p.setText("\xE6\x97\xA5\xE6\x9C\xAC") == TextResult::NonAscii);  // UTF-8 CJK
  CHECK_STR(p.text, "KEEP");
  CHECK(p.setText("HELLO\tWORLD") == TextResult::NonAscii);        // 0x09 tab is not printable
  CHECK_STR(p.text, "KEEP");
  CHECK(p.setText("A\nB") == TextResult::NonAscii);                // 0x0A newline
  CHECK_STR(p.text, "KEEP");
  CHECK(p.setText("A\x7F") == TextResult::NonAscii);               // 0x7F DEL is above 0x7E
  CHECK_STR(p.text, "KEEP");
  CHECK(p.setText("A\x1F") == TextResult::NonAscii);               // 0x1F is below 0x20
  CHECK_STR(p.text, "KEEP");
}

TEST(set_text_refusal_never_mangles_a_high_byte_into_the_marquee) {
  Parameters p;
  p.setText("OK");
  p.setText("bad\xC3\xA9" "byte");   // concatenated so \xA9 does not absorb the following 'b' as a hex digit
  CHECK_STR(p.text, "OK");                        // no mojibake, no partial write: D15 rejects, never normalizes
}
