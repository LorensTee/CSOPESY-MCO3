// include/csopesy/scheduler.hpp — §3.8. Two threads, one mutex, one condition variable, one owner per
// resource; it lives in app/ (composition root) because it wires terminal + renderer + interpreter + PCB.
#pragma once
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include "csopesy/keys.hpp"
#include "csopesy/parameters.hpp"
#include "csopesy/process.hpp"
#include "csopesy/renderer.hpp"
#include "csopesy/frame_buffer.hpp"
#include "csopesy/terminal.hpp"
namespace csopesy {
class Interpreter;
using Clock = std::function<long long()>;       // injected => deterministic tests

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
  Scheduler(Terminal&, Parameters&, Renderer&, Interpreter&, MarqueeProcess&, Clock);
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
  // v2.6: the return lost `quit` (see TickResult). `run()` ignores the result entirely; `rendered` is the
  // step's observable, and every preserved test currently discards the return value anyway.
  TickResult tick(const KeyEvent* ev);   // never reads input, never sleeps on a timer, never ends the loop
  int pollTimeoutMs() const;             // clamp(min(polling, msUntilNextRender), 1, 1000)
  SchedulerSnapshot snapshot() const;    // locks mu_ => race-free for tests, --diag and the debugger

 private:
  int pollTimeoutMsLocked() const;       // run() already holds mu_ (tick() must NEVER be called holding it)
  void noteEventLocked(long long now);   // caller holds mu_; records the OLDEST unconsumed keystroke stamp
  void appendMeasure(const std::string&); // marquee thread only, called OUTSIDE mu_; lazily opens measure_
  Terminal& term_; Parameters& params_; Renderer& renderer_;
  Interpreter& interp_; MarqueeProcess& proc_; Clock now_;
  FrameBuffer fb_;                       // MARQUEE-THREAD PRIVATE; never touched by the input thread
  int fbRows_ = 0, fbCols_ = 0;          // MARQUEE-THREAD PRIVATE; last size the frame was laid out for
  std::ofstream measure_;                // MARQUEE-THREAD PRIVATE; the only writer of the --measure file

  mutable std::mutex mu_;                // guards every field below (and Parameters/Interpreter/PCB)
  std::condition_variable cv_;
  std::thread worker_;                   // INPUT THREAD ONLY: start()/join()/~Scheduler. NEVER join() from run().
  bool stop_ = false, dirty_ = false;    // v2.5 deleted `started_`: worker_.joinable() already IS that state
  long long wakeTick_ = 0;               // bumped by wake(); the "re-evaluate" edge for the cv predicate
  long long pendingEventMs_ = 0;         // --measure: stamp of the OLDEST keystroke whose echo is still owed
  bool eventOwed_ = false;               // ...and whether one is owed (0 is NOT a safe sentinel: clock 0 is real)
};
}
