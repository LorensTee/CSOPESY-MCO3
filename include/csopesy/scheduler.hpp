// include/csopesy/scheduler.hpp — frozen contract, v3.0 §3.8. Two threads, one mutex, one condition
// variable, one owner per resource; it lives in app/ (composition root) because it wires terminal +
// renderer + interpreter + PCB.
//
// v3.0 DELTAS (T0.6, ratified 2026-09-22) — three removals, each with its reason:
//   - the injected `Clock` functor is GONE (D7): `term_.nowMs()` is the single clock source, so `AGENTS.md`
//     §5's invariant holds and no std::chrono call enters scheduler logic;
//   - `FrameBuffer fb_` / `fbRows_` / `fbCols_` are GONE (D6): the worker builds one string with
//     `Renderer::buildFrame` and issues ONE `write()`, so a resize needs no invalidation state;
//   - `std::ofstream measure_` / `appendMeasure()` / `pendingEventMs_` / `eventOwed_` and the keystroke
//     stamping in `noteEventLocked()` are GONE (D4): the PPT's measurement is a manual sweep on the frozen
//     binary (D14).
//
// Everything else is UNCHANGED from v2.6 and remains normative (v2 §3.8): the three lock rules — `tick()` is
// never called holding `mu_`, every blocking I/O call happens outside `mu_`, `Terminal::size()` is read before
// the lock — plus `stop_` as the worker's ONLY exit signal, the `TickResult` narrowing to `{ bool rendered }`,
// `no detach() anywhere`, `~Scheduler` joining, and the test-only `tick(const KeyEvent*)` overload.
#pragma once
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include "csopesy/keys.hpp"
#include "csopesy/parameters.hpp"
#include "csopesy/process.hpp"
#include "csopesy/renderer.hpp"
#include "csopesy/terminal.hpp"
namespace csopesy {
class Interpreter;

struct TickResult { bool rendered = false; };   // `quit` removed in v2.6: the worker never reads quit_ (§3.8)
struct SchedulerSnapshot {                      // the ONLY sanctioned cross-thread read of live scheduler state
  ProcessState state = ProcessState::Stopped;
  long long cycles = 0, lastRenderMs = 0, now = 0;
  bool hasRendered = false;
  int refreshMs = 0, pollingMs = 0;
};

// Two threads, one mutex, one condition variable, one owner per resource.
//   INPUT   (main)   : readEvent -> postEvent -> Interpreter::feed        (NEVER writes a frame)
//                       ...and the ONLY reader/writer of Interpreter::quit_ (v2.6)
//   MARQUEE (worker) : run() -> cv_.wait_for(deadline | wake) -> tick()    (the ONLY writer of term_)
class Scheduler {
 public:
  // No `Clock` parameter in v3.0: deadlines come from `term_.nowMs()` (D7).
  Scheduler(Terminal&, Parameters&, Renderer&, Interpreter&, MarqueeProcess&);
  ~Scheduler();                          // requestStop() + join(); NEVER detaches

  // --- input/command thread ---
  void postEvent(const KeyEvent& ev);    // feeds the interpreter, marks a redraw, wakes the worker
  void wake();                           // "state may have changed, re-evaluate"; test/teardown sync hook
  void requestStop();                    // sets stop_ and wakes the worker; idempotent
  void join();                           // joins the worker; idempotent
  bool joinable() const;                 // worker_.joinable()

  // --- marquee/scheduler thread ---
  void start();                          // spawns exactly ONE worker; no detach anywhere
  int run();                             // the worker body; returns 0 on clean shutdown

  // --- pure step: contract preserved from v2.3 (return narrowed in v2.6); the worker's single step --------
  // TEST-ONLY overload. `ev != nullptr` exists so the preserved Step 1 tests can inject an event with no
  // thread at all; PRODUCTION NEVER CALLS IT — ConsoleApp delivers keystrokes through postEvent() and run()
  // always steps with nullptr. Both paths share one locked body by construction; if you change one, change
  // both (that is the maintenance trap `gpt-v5.md` §6 named, and this comment is the mitigation).
  // v2.6: the return lost `quit` (see TickResult). `run()` ignores the result entirely.
  TickResult tick(const KeyEvent* ev);   // never reads input, never sleeps on a timer, never ends the loop
  int pollTimeoutMs() const;             // clamp(min(polling, msUntilNextRender), 1, 1000)
  SchedulerSnapshot snapshot() const;    // locks mu_ => race-free for tests and the debugger

 private:
  int pollTimeoutMsLocked() const;       // run() already holds mu_ (tick() must NEVER be called holding it)
  Terminal& term_; Parameters& params_; Renderer& renderer_;
  Interpreter& interp_; MarqueeProcess& proc_;

  mutable std::mutex mu_;                // guards every field below (and Parameters/Interpreter/PCB)
  std::condition_variable cv_;
  std::thread worker_;                   // INPUT THREAD ONLY: start()/join()/~Scheduler. NEVER join() from run().
  bool stop_ = false, dirty_ = false;    // v2.5 deleted `started_`: worker_.joinable() already IS that state
  long long wakeTick_ = 0;               // bumped by wake(); the "re-evaluate" edge for the cv predicate
};
}
