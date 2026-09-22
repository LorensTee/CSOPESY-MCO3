// tests/unit/test_scheduler.cpp — T1.3: the marquee worker, the preserved pure step, and the render deadline
// driven by term_.nowMs() (§3.8).
//
// Deterministic by construction: every wait is a condition-variable barrier on a FakeTerminal counter, and the
// clock is the double's atomic field. No real-time sleep primitive appears anywhere — a sleep here would be a
// flake, not a synchronization.
//
// SCOPE: this suite pins only what the scheduler owns. Terminal::write() is observable only once
// Renderer::buildFrame returns a real frame (T4.2), and a command's effect only once Interpreter::feed works
// (T2.4), so those assertions are deliberately absent rather than faked; the list to add when those land is in
// docs/PLAN_V3_PROGRESS.md §6. Pinned here: cycles, hasRendered/lastRenderMs, snapshot(), pollTimeoutMs(),
// TickResult, stop_ + join, and the tick barrier (tick() calls Terminal::size() once, as its first action).
#include <atomic>
#include <cstddef>
#include <string>
#include <thread>
#include "check.hpp"
#include "fake_terminal.hpp"
#include "csopesy/interpreter.hpp"
#include "csopesy/renderer.hpp"
#include "csopesy/scheduler.hpp"
using namespace csopesy;

namespace {
struct Harness {   // declaration order == wiring order
  FakeTerminal term;
  Parameters params;
  MarqueeProcess proc;
  Renderer renderer;
  Interpreter interp{params, proc};
  Scheduler sched{term, params, renderer, interp, proc};
};

constexpr int kHangGuardMs = 5000;   // a hang guard for a lost notify / missing join — never a clock

// The test thread plays the input thread: readEvent -> postEvent, exactly as ConsoleApp::run() does.
void drainInput(FakeTerminal& t, Scheduler& s) {
  KeyEvent ev;
  while (t.readEvent(ev, 0)) { s.postEvent(ev); }
}
}  // namespace

// ---------------------------------------------------------------------------------------------------------
// Pure step: tick() with no thread at all (the production shape, ev == nullptr).
// ---------------------------------------------------------------------------------------------------------

TEST(no_render_while_stopped) {
  Harness h;
  h.term.clock = 1000;
  CHECK(!h.sched.tick(nullptr).rendered);
  CHECK_EQ(h.proc.cycles, 0);
  CHECK(!h.proc.hasRendered);
}

TEST(first_frame_is_immediate_and_the_refresh_deadline_is_respected) {
  Harness h;
  h.proc.state = ProcessState::Running;
  h.params.refreshMs = 100;
  h.term.clock = 0;    CHECK(h.sched.tick(nullptr).rendered);  CHECK_EQ(h.proc.cycles, 1);   // fresh => immediate
  h.term.clock = 99;   CHECK(!h.sched.tick(nullptr).rendered); CHECK_EQ(h.proc.cycles, 1);   // one ms early
  h.term.clock = 100;  CHECK(h.sched.tick(nullptr).rendered);  CHECK_EQ(h.proc.cycles, 2);   // exactly due
  h.term.clock = 250;  CHECK(h.sched.tick(nullptr).rendered);  CHECK_EQ(h.proc.cycles, 3);
  CHECK(h.proc.hasRendered);
  CHECK_EQ(h.proc.lastRenderMs, 250);
}

TEST(set_speed_moves_the_next_deadline_without_recompiling) {
  Harness h;
  h.proc.state = ProcessState::Running;
  h.params.refreshMs = 1000;
  h.term.clock = 0;  (void)h.sched.tick(nullptr);
  h.params.refreshMs = 50;                                          // == what `set_speed 50` lands as once
                                                                    // T2.1/T2.4 apply it (no worker here,
                                                                    // so touching the field is race-free)
  h.term.clock = 60;   CHECK(h.sched.tick(nullptr).rendered);       // 60 < 1000 but >= the NEW 50
  h.term.clock = 100;  CHECK(!h.sched.tick(nullptr).rendered);      // 40 ms since the last frame: too early
  CHECK_EQ(h.proc.cycles, 2);
}

TEST(start_and_restart_render_immediately) {
  Harness h;
  h.params.refreshMs = 500;
  CHECK(h.proc.start());
  h.term.clock = 0;      CHECK(h.sched.tick(nullptr).rendered);  CHECK_EQ(h.proc.cycles, 1);
  CHECK(h.proc.stop());
  h.term.clock = 10000;  CHECK(!h.sched.tick(nullptr).rendered); CHECK_EQ(h.proc.cycles, 1);   // stopped
  CHECK(h.proc.start());
  CHECK(h.sched.tick(nullptr).rendered);                        CHECK_EQ(h.proc.cycles, 2);   // immediate again
}

TEST(a_redraw_is_not_a_rendered_frame) {
  Harness h;
  h.proc.state = ProcessState::Running;
  h.params.refreshMs = 10000;
  h.term.clock = 0;  (void)h.sched.tick(nullptr);
  CHECK_EQ(h.proc.cycles, 1);
  h.term.clock = 5;
  h.sched.postEvent(KeyEvent{KeyType::Char, 'x'});          // marks a redraw owed, like a keystroke does
  CHECK(!h.sched.tick(nullptr).rendered);                   // a redraw is not a deadline frame
  CHECK_EQ(h.proc.cycles, 1);                               // cycles counts RENDERED frames, not redraws
  CHECK_EQ(h.proc.lastRenderMs, 0);                         // ...and the redraw did not move the deadline
}

TEST(tick_never_reads_input_itself) {   // the double-poll regression guard
  Harness h;
  h.proc.state = ProcessState::Running;
  h.term.events.push_back(KeyEvent{KeyType::Char, 'x'});
  (void)h.sched.tick(nullptr);
  CHECK_EQ(h.term.events.size(), size_t{1});   // tick consumed nothing: the input thread owns readEvent
}

TEST(poll_timeout_is_capped_by_the_render_deadline) {
  Harness h;
  h.proc.state = ProcessState::Running;
  h.params.refreshMs = 30;
  h.params.pollingMs = 1000;
  h.term.clock = 0;  (void)h.sched.tick(nullptr);   // lastRenderMs = 0
  h.term.clock = 10;
  CHECK(h.sched.pollTimeoutMs() <= 20);             // ~msUntilNextRender, NOT pollingMs
  CHECK(h.sched.pollTimeoutMs() >= 1);
  h.term.clock = 29;
  CHECK_EQ(h.sched.pollTimeoutMs(), 1);             // one ms left, and never zero (zero would busy-spin)
}

TEST(poll_timeout_is_never_zero_and_never_exceeds_polling) {
  Harness h;
  h.params.pollingMs = 10;
  h.params.refreshMs = 100;
  h.proc.state = ProcessState::Running;
  h.term.clock = 0;  (void)h.sched.tick(nullptr);
  for (int t = 0; t <= 100; t += 7) {
    h.term.clock = t;
    const int p = h.sched.pollTimeoutMs();
    CHECK(p >= 1);
    CHECK(p <= h.params.pollingMs);
  }
  h.proc.stop();                     // no render deadline remains, so pollingMs is the whole bound
  h.term.clock = 5000;
  CHECK_EQ(h.sched.pollTimeoutMs(), 10);
}

TEST(poll_timeout_never_exceeds_the_contract_bound) {
  Harness h;
  h.params.pollingMs = Parameters::kPollMax;
  CHECK_EQ(h.sched.pollTimeoutMs(), 1000);
  h.params.pollingMs = 5000;         // the setter cannot produce this, but the clamp is the contract
  CHECK_EQ(h.sched.pollTimeoutMs(), 1000);
}

TEST(snapshot_is_a_race_free_view_of_the_live_state) {
  Harness h;
  h.proc.state = ProcessState::Running;
  h.params.refreshMs = 7;
  h.params.pollingMs = 3;
  h.term.clock = 42;
  (void)h.sched.tick(nullptr);
  const SchedulerSnapshot s = h.sched.snapshot();
  CHECK(s.state == ProcessState::Running);
  CHECK_EQ(s.cycles, 1);
  CHECK_EQ(s.lastRenderMs, 42);
  CHECK_EQ(s.now, 42);               // `now` comes from the terminal clock, not std::chrono
  CHECK(s.hasRendered);
  CHECK_EQ(s.refreshMs, 7);
  CHECK_EQ(s.pollingMs, 3);
}

// ---------------------------------------------------------------------------------------------------------
// Threaded: the real worker, driven only by the tick barrier and the double's clock.
// ---------------------------------------------------------------------------------------------------------

TEST(threaded_worker_renders_immediately_then_only_on_the_refresh_deadline) {
  Harness h;
  h.params.refreshMs = 100;
  h.params.pollingMs = 1000;
  CHECK(h.proc.start());
  h.sched.start();
  // tick n+1 STARTING proves tick n's body returned — the worker steps strictly in sequence, and size() is
  // the first thing a step does. So this is a completion barrier. Asserting about tick n as soon as tick n
  // starts would race it: size() runs BEFORE the step takes mu_, so the render may not have happened yet.
  CHECK(h.term.waitForTickStarted(2, kHangGuardMs));
  CHECK_EQ(h.sched.snapshot().cycles, 1);               // !hasRendered => the first frame is immediate
  h.term.clock = 50;                                    // 50 < 100: not due yet
  h.sched.wake();                                       // explicit re-evaluate: no real-time wait
  size_t t = h.term.tickCount();
  CHECK(h.term.waitForTickStarted(t + 2, kHangGuardMs));
  CHECK_EQ(h.sched.snapshot().cycles, 1);               // too early: no render
  h.term.clock = 100;                                   // past the deadline
  h.sched.wake();
  t = h.term.tickCount();
  CHECK(h.term.waitForTickStarted(t + 2, kHangGuardMs));
  CHECK_EQ(h.sched.snapshot().cycles, 2);               // rendered
  h.sched.requestStop();
  h.sched.join();
}

TEST(threaded_marquee_keeps_animating_across_refresh_deadlines) {
  Harness h;
  h.params.refreshMs = 100;
  h.params.pollingMs = 1000;
  CHECK(h.proc.start());
  h.sched.start();
  CHECK(h.term.waitForTickStarted(1, kHangGuardMs));
  for (int i = 1; i <= 3; ++i) {
    h.term.clock = i * 100;
    h.sched.wake();
    // Two barriers: the first tick that began after the clock moved, plus one more so that tick has
    // *completed*. Requiring only one more would leave the assertion racing that tick.
    const size_t t = h.term.tickCount();
    CHECK(h.term.waitForTickStarted(t + 2, kHangGuardMs));
  }
  CHECK_EQ(h.sched.snapshot().cycles, 4);               // 1 immediate + 3 deadline frames
  CHECK(h.sched.snapshot().hasRendered);
  h.sched.requestStop();
  h.sched.join();
}

TEST(request_stop_is_the_workers_only_exit_signal_and_join_is_idempotent) {
  Harness h;
  h.params.refreshMs = 10000;
  h.params.pollingMs = 1000;
  h.sched.start();
  CHECK(h.term.waitForTickStarted(1, kHangGuardMs));
  CHECK(h.sched.joinable());
  h.sched.requestStop();                                // must wake an idle wait, not wait out pollingMs
  h.sched.join();
  CHECK(!h.sched.joinable());
  h.sched.requestStop();                                // idempotent: safe to repeat
  h.sched.join();
  CHECK(!h.sched.joinable());
  const size_t ticksAfterJoin = h.term.tickCount();
  h.sched.wake();                                       // must not resurrect a worker
  CHECK_EQ(h.term.tickCount(), ticksAfterJoin);
}

TEST(destroying_a_live_scheduler_joins_instead_of_detaching) {
  FakeTerminal term;
  Parameters params;
  MarqueeProcess proc;
  Renderer renderer;
  Interpreter interp{params, proc};
  params.refreshMs = 1;
  params.pollingMs = 1;
  {
    Scheduler sched{term, params, renderer, interp, proc};
    sched.start();
    CHECK(term.waitForTickStarted(3, kHangGuardMs));    // deliberately NOT stopped before the scope ends
  }
  // Reaching this line IS the assertion: ~Scheduler had to requestStop() + join() a running worker, or this
  // test would still be blocked when ctest's 60 s TIMEOUT fires.
  CHECK(term.tickCount() >= 3);
}

TEST(threaded_typing_stress_does_not_break_the_scheduler) {
  Harness h;
  h.params.refreshMs = 1;
  h.params.pollingMs = 1;
  CHECK(h.proc.start());
  h.sched.start();
  CHECK(h.term.waitForTickStarted(1, kHangGuardMs));
  for (int batch = 0; batch < 20; ++batch) {            // 400 keystrokes, drained in batches
    for (int i = 0; i < 20; ++i) { h.term.push(KeyEvent{KeyType::Char, 'x'}); }
    drainInput(h.term, h.sched);
    h.sched.wake();
  }
  h.term.push(KeyEvent{KeyType::Backspace, 0});
  drainInput(h.term, h.sched);
  h.sched.wake();
  h.term.clock = 5;                                     // past the 1 ms deadline: a frame is owed
  const size_t t = h.term.tickCount();
  CHECK(h.term.waitForTickStarted(t + 2, kHangGuardMs));
  h.sched.requestStop();
  h.sched.join();
  CHECK(h.sched.snapshot().cycles >= 1);                // it animated throughout, and never crashed
  CHECK(!h.sched.joinable());
}

// The ONE test with a second REAL thread (not the test thread) driving input while the worker animates.
// The deterministic suite above proves serializability; this proves the two activities actually overlapped in
// time. Still no sleep: the barrier is waitForTickStarted, and the input thread cannot exit before it because
// it only stops when stopInput is set, which happens after the barrier returns.
TEST(threaded_input_thread_and_animation_overlap_in_real_time) {
  Harness h;
  h.params.refreshMs = 1;
  h.params.pollingMs = 1;
  CHECK(h.proc.start());
  h.sched.start();
  CHECK(h.term.waitForTickStarted(2, kHangGuardMs));    // tick 1 completed — see the barrier note above
  CHECK_EQ(h.sched.snapshot().cycles, 1);               // the immediate first frame

  std::atomic<bool> stopInput{false};
  std::atomic<long long> posted{0};
  std::thread input([&] {                               // a REAL input thread, playing ConsoleApp::run()
    KeyEvent ev;
    while (!stopInput.load()) {
      h.term.push(KeyEvent{KeyType::Char, 'x'});
      if (h.term.readEvent(ev, 0)) {
        h.sched.postEvent(ev);
        posted.fetch_add(1);
      }
    }
  });

  // Advance simulated time so the marquee has a second frame to draw, then wait for a tick that began after
  // that store (tick t+1) AND completed (tick t+2) — the store is sequenced before the tickCount() read, and
  // every later tick loads the clock, so exactly one further render is guaranteed here.
  h.term.clock = 1;
  const size_t t = h.term.tickCount();
  CHECK(h.term.waitForTickStarted(t + 2, kHangGuardMs));
  CHECK_EQ(h.sched.snapshot().cycles, 2);

  // The overlap evidence: 100 further ticks start while the input thread is still posting.
  const size_t t2 = h.term.tickCount();
  CHECK(h.term.waitForTickStarted(t2 + 100, kHangGuardMs));
  stopInput = true;
  input.join();
  h.sched.requestStop();
  h.sched.join();

  CHECK(posted.load() > 0);                             // input really was processed, not merely queued
  CHECK_EQ(h.sched.snapshot().cycles, 2);               // the clock never moved again: no extra frames
  CHECK(!h.sched.joinable());
}
