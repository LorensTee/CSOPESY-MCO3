// tests/unit/test_process.cpp — T1.2: the MarqueeProcess PCB. start()/stop() report whether they changed
// state, and start() clears hasRendered so a restarted marquee draws on its next tick.
#include "check.hpp"
#include "csopesy/process.hpp"
using namespace csopesy;

TEST(process_defaults_match_the_contract) {
  MarqueeProcess p;
  CHECK_EQ(p.pid, 1);
  CHECK_STR(p.name, "marquee");
  CHECK(p.state == ProcessState::Stopped);
  CHECK_EQ(p.cycles, 0);
  CHECK_EQ(p.lastRenderMs, 0);
  CHECK(!p.hasRendered);
}

TEST(start_from_stopped_runs_and_returns_true) {
  MarqueeProcess p;
  CHECK(p.start());
  CHECK(p.state == ProcessState::Running);
}

TEST(start_while_running_returns_false_and_leaves_state_running) {
  MarqueeProcess p;
  CHECK(p.start());
  CHECK(!p.start());
  CHECK(p.state == ProcessState::Running);
}

TEST(start_clears_hasRendered) {
  MarqueeProcess p;
  p.hasRendered = true;          // as if the scheduler had already drawn a frame
  CHECK(p.start());
  CHECK(!p.hasRendered);
  CHECK(p.state == ProcessState::Running);
}

TEST(stop_from_running_stops_and_returns_true) {
  MarqueeProcess p;
  CHECK(p.start());
  CHECK(p.stop());
  CHECK(p.state == ProcessState::Stopped);
}

TEST(stop_while_stopped_returns_false_and_leaves_state_stopped) {
  MarqueeProcess p;
  CHECK(!p.stop());
  CHECK(p.state == ProcessState::Stopped);
}

TEST(restart_after_stop_clears_hasRendered_again) {
  MarqueeProcess p;
  CHECK(p.start());
  p.hasRendered = true;          // the running marquee drew at least one frame
  CHECK(p.stop());
  CHECK(p.hasRendered);          // stopping alone must not clear it
  CHECK(p.start());
  CHECK(!p.hasRendered);
  CHECK(p.state == ProcessState::Running);
}
