// tests/unit/test_scheduler.cpp — T1.3: the marquee worker, the preserved pure step, and the render deadline
// driven by term_.nowMs() (§3.8).
//
// Deterministic by construction: every wait is a condition-variable barrier on a FakeTerminal counter, and the
// clock is the double's atomic field. No real-time sleep primitive appears anywhere — a sleep here would be a
// flake, not a synchronization.
//
// SCOPE: this suite pins what the scheduler owns. Two groups of assertions were unobservable until their
// dependencies existed — Terminal::write() does nothing until Renderer::buildFrame (T4.2) returns a real
// frame, and a command has no effect until Interpreter::feed (T2.4) executes it — so they sit at the end:
// exactly one writer (the worker), exactly one whole frame per write(), a keystroke's echo ungated by
// refreshMs, and a posted command's effect visible in the next frame. Pinned from the start: cycles,
// hasRendered/lastRenderMs, snapshot(), pollTimeoutMs(), TickResult, stop_ + join, and the tick barrier
// (tick() calls Terminal::size() once, as its first action).
#include <atomic>
#include <cstddef>
#include <string>
#include <thread>
#include <vector>
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

// Type a whole line down the same path: one postEvent per keystroke, then Enter. Nothing here reaches the
// interpreter except through the production input entry point, which is the point of the command-effect tests.
void typeCommand(Scheduler& s, const std::string& line) {
  for (const char c : line) { s.postEvent(KeyEvent{KeyType::Char, c}); }
  s.postEvent(KeyEvent{KeyType::Enter, 0});
}

// Split a frame back into rows on its CRLF separators, so a test can tell one whole frame from a fragment or
// from two concatenated frames.
std::vector<std::string> splitFrame(const std::string& frame) {
  std::vector<std::string> rows(1);
  for (std::size_t i = 0; i < frame.size(); ++i) {
    if (frame[i] == '\r' && i + 1 < frame.size() && frame[i + 1] == '\n') { rows.emplace_back(); ++i; }
    else { rows.back().push_back(frame[i]); }
  }
  return rows;
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
  // tick n+1 STARTING proves tick n's body returned (see the barrier note above), so this completion barrier
  // also pins WHEN the immediate frame is stamped. tick 1 reads the clock AFTER size() (rule 3), so a start
  // barrier would let the loop's first clock store of 100 land in that gap: the immediate frame would then be
  // stamped at 100, every deadline would sit one refresh period late, and the loop would render 3 frames, not
  // 4. lastRenderMs below is the checked precondition that makes the final count deterministic.
  CHECK(h.term.waitForTickStarted(2, kHangGuardMs));
  CHECK_EQ(h.sched.snapshot().cycles, 1);               // !hasRendered => the first frame is immediate...
  CHECK_EQ(h.sched.snapshot().lastRenderMs, 0);         // ...stamped at the pre-loop clock the deadlines count from
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

// ---------------------------------------------------------------------------------------------------------
// The frame writes (T4.2) and the input thread's command path (T2.4).
// ---------------------------------------------------------------------------------------------------------

TEST(every_write_carries_exactly_one_whole_frame) {
  Harness h;
  h.proc.state = ProcessState::Running;
  h.params.refreshMs = 100;
  h.term.clock = 0;    (void)h.sched.tick(nullptr);          // deadline frame 1
  h.sched.postEvent(KeyEvent{KeyType::Char, 'a'});
  h.term.clock = 5;    (void)h.sched.tick(nullptr);          // echo frame: owed by dirty_, not by a deadline
  h.term.clock = 100;  (void)h.sched.tick(nullptr);          // deadline frame 2

  const std::vector<std::string> writes = h.term.writesCopy();
  CHECK_EQ(writes.size(), size_t{3});
  CHECK_EQ(h.term.frameCount(), writes.size());              // one write() per frame, never a partial one
  std::size_t total = 0;
  for (const std::string& w : writes) {
    CHECK(w.rfind("\x1b[H", 0) == 0);                       // every write opens at the home cell...
    const std::vector<std::string> rows = splitFrame(w.substr(3));
    CHECK_EQ(rows.size(), size_t{24});                       // ...carries every row of one frame...
    for (const std::string& row : rows) { CHECK_EQ(row.size(), size_t{80}); }   // ...each exactly cols wide
    total += w.size();
  }
  CHECK_EQ(h.term.outCopy().size(), total);                  // nothing reached the terminal outside write()
}

TEST(posted_keystroke_echo_is_written_without_waiting_for_the_refresh_deadline) {
  Harness h;
  h.proc.state = ProcessState::Running;
  h.params.refreshMs = 10000;                                // far beyond any echo a human would call prompt
  h.term.clock = 0;
  CHECK(h.sched.tick(nullptr).rendered);
  CHECK_EQ(h.term.frameCount(), size_t{1});

  h.term.clock = 5;                                          // 5 << 10000: no render deadline is due
  h.sched.postEvent(KeyEvent{KeyType::Char, 'a'});
  CHECK(!h.sched.tick(nullptr).rendered);                    // the echo is not a RENDERED frame...
  CHECK_EQ(h.proc.cycles, 1);                                // ...cycles counts only deadline frames...
  CHECK_EQ(h.term.frameCount(), size_t{2});                  // ...but a frame WAS written, on dirty_

  h.term.clock = 6;
  CHECK(!h.sched.tick(nullptr).rendered);
  CHECK_EQ(h.term.frameCount(), size_t{2});                  // the redraw is consumed once, not re-emitted
}

TEST(only_the_marquee_thread_ever_writes_the_terminal) {
  Harness h;
  h.params.refreshMs = 1;
  h.params.pollingMs = 1;
  CHECK(h.proc.start());
  h.sched.start();
  CHECK(h.term.waitForTickStarted(2, kHangGuardMs));

  // Drive the input path from THIS thread — the one ConsoleApp::run() uses. It feeds the interpreter and
  // posts events, and must never touch the terminal.
  h.term.push(KeyEvent{KeyType::Char, 'a'});
  drainInput(h.term, h.sched);
  typeCommand(h.sched, "help");
  h.term.clock = 5;                                          // past the 1 ms deadline: at least one more frame
  const size_t t = h.term.tickCount();
  CHECK(h.term.waitForTickStarted(t + 2, kHangGuardMs));
  h.sched.requestStop();
  h.sched.join();

  const std::vector<std::thread::id> writers = h.term.writersCopy();
  CHECK(!writers.empty());
  if (!writers.empty()) {                                   // guarded: a stub writer list must red, not abort
    const std::thread::id worker = writers.front();
    CHECK(worker != std::this_thread::get_id());             // the input thread never wrote a frame
    for (const std::thread::id& id : writers) { CHECK(id == worker); }   // exactly ONE writer: the worker
  }
  CHECK_EQ(h.term.frameCount(), writers.size());
}

TEST(posted_stop_marquee_command_stops_the_process) {
  Harness h;
  CHECK(h.proc.start());
  CHECK(h.proc.state == ProcessState::Running);
  typeCommand(h.sched, "stop_marquee");
  CHECK(h.proc.state == ProcessState::Stopped);
  CHECK_STR(h.interp.lastMessage(), "Marquee stopped.");
}

TEST(posted_set_text_command_changes_the_animated_text) {
  Harness h;
  h.proc.state = ProcessState::Running;
  typeCommand(h.sched, "set_text HELLO WORLD");
  CHECK_STR(h.params.text, "HELLO WORLD");                   // trimmed, internal space run preserved
  CHECK(h.interp.lastMessage().find("HELLO WORLD") != std::string::npos);

  // offset == bandWidth puts text[0] in the band's first column, so the new text is visible in this frame.
  h.proc.cycles = Renderer::bandWidthFor(80);
  h.term.clock = 0;
  (void)h.sched.tick(nullptr);
  CHECK(h.term.outCopy().find("HELLO WORLD") != std::string::npos);
}

TEST(posted_set_speed_command_moves_the_next_render_deadline) {
  Harness h;
  h.proc.state = ProcessState::Running;
  h.params.refreshMs = 1000;
  h.term.clock = 0;
  CHECK(h.sched.tick(nullptr).rendered);                     // the immediate frame; the deadline is now 1000
  CHECK_EQ(h.proc.cycles, 1);

  typeCommand(h.sched, "set_speed 50");                      // the COMMAND, not the field the pure test sets
  CHECK_EQ(h.params.refreshMs, 50);
  CHECK_STR(h.interp.lastMessage(), "Marquee speed set to 50 ms.");

  h.term.clock = 40;
  CHECK(!h.sched.tick(nullptr).rendered);                    // 40 < 50: the new deadline is in force...
  h.term.clock = 60;
  CHECK(h.sched.tick(nullptr).rendered);                     // ...and 60 < 1000, so only the command moved it
  CHECK_EQ(h.proc.cycles, 2);
}

TEST(posted_exit_command_requests_quit_and_leaves_the_worker_alone) {
  Harness h;
  h.proc.state = ProcessState::Running;
  h.params.refreshMs = 100;
  typeCommand(h.sched, "exit");
  CHECK(h.interp.quitRequested());
  CHECK_STR(h.interp.lastMessage(), "Exiting CSOPESY. Goodbye!");
  // quit_ belongs to the input thread: only stop_ ends the worker's loop, so the marquee keeps rendering
  // and shutdown is ConsoleApp's requestStop() -> join(), not the worker reading quit_ (§3.8).
  h.term.clock = 200;
  CHECK(h.sched.tick(nullptr).rendered);
  CHECK(h.proc.state == ProcessState::Running);
}
