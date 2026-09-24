// tests/unit/test_cli.cpp — T2.2: the three-flag parser. A typo must never cost the quiz, so every failure
// mode warns and continues; only a well-formed out-of-range value is clamped and reported.
#include "check.hpp"
#include "csopesy/cli.hpp"

#include <string>
#include <vector>

using csopesy::CliResult;
using csopesy::parseCli;

namespace {
CliResult run(std::vector<std::string> args) { return parseCli(args); }
bool hasWarning(const CliResult& r, const std::string& needle) {
  for (const auto& w : r.warnings) if (w.find(needle) != std::string::npos) return true;
  return false;
}
// Safe accessor: a red run against the stub has no warnings, and indexing an empty vector aborts the suite
// instead of reporting the failure. Return a sentinel so the assertion fails loudly but the run continues.
std::string warningAt(const CliResult& r, std::size_t i) {
  return i < r.warnings.size() ? r.warnings[i] : std::string("<no such warning>");
}
}  // namespace

TEST(no_args_yields_defaults_and_no_warnings) {
  const CliResult r = run({});
  CHECK_EQ(r.params.refreshMs, 100);
  CHECK_EQ(r.params.pollingMs, 10);
  CHECK(!r.params.noTty);
  CHECK_EQ(r.warnings.size(), size_t{0});
}

TEST(no_tty_flag_sets_plain_line_mode) {
  const CliResult r = run({"--no-tty"});
  CHECK(r.params.noTty);
  CHECK_EQ(r.warnings.size(), size_t{0});
}

TEST(refresh_flag_applies_in_range_value) {
  const CliResult r = run({"--refresh-ms=250"});
  CHECK_EQ(r.params.refreshMs, 250);
  CHECK_EQ(r.warnings.size(), size_t{0});
}

TEST(poll_flag_applies_in_range_value) {
  const CliResult r = run({"--poll-ms=500"});
  CHECK_EQ(r.params.pollingMs, 500);
  CHECK_EQ(r.warnings.size(), size_t{0});
}

TEST(all_three_flags_together) {
  const CliResult r = run({"--no-tty", "--refresh-ms=50", "--poll-ms=5"});
  CHECK(r.params.noTty);
  CHECK_EQ(r.params.refreshMs, 50);
  CHECK_EQ(r.params.pollingMs, 5);
  CHECK_EQ(r.warnings.size(), size_t{0});
}

// --- clamping: reported, never a usage error ---------------------------------------------------------

TEST(refresh_zero_is_clamped_and_reported) {
  const CliResult r = run({"--refresh-ms=0"});
  CHECK_EQ(r.params.refreshMs, 1);
  CHECK_EQ(r.warnings.size(), size_t{1});
  CHECK_STR(warningAt(r, 0), "--refresh-ms=0 clamped to 1 ms.");
}

TEST(refresh_above_max_is_clamped_and_reported) {
  const CliResult r = run({"--refresh-ms=99999"});   // §3.6's own example
  CHECK_EQ(r.params.refreshMs, 10000);
  CHECK_EQ(r.warnings.size(), size_t{1});
  CHECK_STR(warningAt(r, 0), "--refresh-ms=99999 clamped to 10000 ms.");
}

TEST(negative_refresh_is_clamped_not_rejected) {
  const CliResult r = run({"--refresh-ms=-5"});
  CHECK_EQ(r.params.refreshMs, 1);
  CHECK(hasWarning(r, "clamped to 1 ms"));
}

TEST(poll_above_max_is_clamped_and_reported) {
  const CliResult r = run({"--poll-ms=2000"});
  CHECK_EQ(r.params.pollingMs, 1000);
  CHECK_EQ(r.warnings.size(), size_t{1});
  CHECK_STR(warningAt(r, 0), "--poll-ms=2000 clamped to 1000 ms.");
}

TEST(a_huge_well_formed_number_is_clamped_not_malformed) {
  const CliResult r = run({"--refresh-ms=99999999999999999999"});
  CHECK_EQ(r.params.refreshMs, 10000);
  CHECK(hasWarning(r, "clamped to 10000 ms"));       // no crash, no silent reset
}

TEST(leading_plus_is_a_valid_sign) {
  const CliResult r = run({"--refresh-ms=+250"});    // contract: [-+]?[0-9]+
  CHECK_EQ(r.params.refreshMs, 250);
  CHECK_EQ(r.warnings.size(), size_t{0});
}

// --- malformed / absent values: warn and keep the default ---------------------------------------------

TEST(malformed_number_keeps_default_and_warns) {
  const CliResult r = run({"--refresh-ms=abc"});
  CHECK_EQ(r.params.refreshMs, 100);
  CHECK_EQ(r.warnings.size(), size_t{1});
  CHECK_STR(warningAt(r, 0), "--refresh-ms=abc: not a number; keeping the default.");
}

TEST(malformed_number_never_partially_parses) {
  const CliResult r = run({"--refresh-ms=5abc"});    // std::stoi would silently accept 5
  CHECK_EQ(r.params.refreshMs, 100);
  CHECK_EQ(r.warnings.size(), size_t{1});
}

TEST(empty_value_is_malformed) {
  const CliResult r = run({"--refresh-ms="});
  CHECK_EQ(r.params.refreshMs, 100);
  CHECK(hasWarning(r, "not a number"));
}

TEST(absent_value_warns_and_keeps_default) {
  const CliResult r = run({"--refresh-ms"});
  CHECK_EQ(r.params.refreshMs, 100);
  CHECK_EQ(r.warnings.size(), size_t{1});
  CHECK_STR(warningAt(r, 0), "--refresh-ms: missing value; keeping the default.");
}

TEST(space_separated_form_is_two_tokens_both_warn) {
  const CliResult r = run({"--refresh-ms", "50"});   // §3.6 rule 1
  CHECK_EQ(r.params.refreshMs, 100);
  CHECK_EQ(r.warnings.size(), size_t{2});
  CHECK_STR(warningAt(r, 0), "--refresh-ms: missing value; keeping the default.");
  CHECK_STR(warningAt(r, 1), "Unknown option ignored: \"50\".");
}

// --- unknown tokens: warn and continue -----------------------------------------------------------------

TEST(unknown_flag_warns_and_parsing_continues) {
  const CliResult r = run({"--foo", "--refresh-ms=250"});
  CHECK_EQ(r.params.refreshMs, 250);                 // the known flag still took effect
  CHECK(hasWarning(r, "Unknown option ignored: \"--foo\"."));
}

TEST(old_config_flag_warns_like_any_unknown) {
  const CliResult r = run({"--config=config/csopesy.ini"});
  CHECK_EQ(r.params.refreshMs, 100);
  CHECK(hasWarning(r, "Unknown option ignored"));
  CHECK(hasWarning(r, "--config"));
}

TEST(no_tty_with_a_value_is_an_unknown_option) {
  const CliResult r = run({"--no-tty=1"});
  CHECK(!r.params.noTty);                            // the flag takes no value, so nothing was set
  CHECK(hasWarning(r, "Unknown option ignored: \"--no-tty=1\"."));
}

TEST(single_dash_token_is_unknown) {
  const CliResult r = run({"-x"});
  CHECK(hasWarning(r, "Unknown option ignored: \"-x\"."));
}

// --- repetition ----------------------------------------------------------------------------------------

TEST(repeated_flag_last_value_wins_without_warning) {
  const CliResult r = run({"--refresh-ms=100", "--refresh-ms=250"});
  CHECK_EQ(r.params.refreshMs, 250);
  CHECK_EQ(r.warnings.size(), size_t{0});            // a repeat is not bad input
}
