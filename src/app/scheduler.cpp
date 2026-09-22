// src/app/scheduler.cpp — the marquee worker + the input-thread API (§3.8).
//
// Two threads, one mutex, one condition variable, one owner per resource:
//   input (main)     : readEvent -> postEvent -> Interpreter::feed   (never writes a frame)
//   marquee (worker) : run() -> cv_.wait_for(deadline | wake) -> tick()   (the only writer of term_)
//
// Three rules keep that deadlock-free and bound the critical section:
//   1. tick() is never called while mu_ is held;
//   2. every blocking call happens outside mu_ — the frame is built under the lock and written after it;
//   3. Terminal::size() is read before the lock (it is a syscall and it belongs to the marquee thread alone).
// Deadlines come from term_.nowMs(), the single clock source: <chrono> is used here only as the wait_for
// duration type, never as a clock.
#include <chrono>
#include <string>

#include "csopesy/interpreter.hpp"   // app/ -> features/commands/ is a downward import (§3.1)
#include "csopesy/scheduler.hpp"

namespace csopesy {

Scheduler::Scheduler(Terminal& term, Parameters& params, Renderer& renderer, Interpreter& interp,
                     MarqueeProcess& proc)
    : term_(term), params_(params), renderer_(renderer), interp_(interp), proc_(proc) {}

Scheduler::~Scheduler() {
  requestStop();
  join();   // never detach: the worker must not outlive the objects it touches
}

// --- input/command thread -------------------------------------------------------------------------
void Scheduler::postEvent(const KeyEvent& ev) {
  {
    std::lock_guard<std::mutex> lk(mu_);
    interp_.feed(ev);
    dirty_ = true;          // "an echo frame is owed" — this is what keeps typing latency off refreshMs
  }
  cv_.notify_all();         // one waiter today; §3.8 and requestStop use notify_all, so match them
}

void Scheduler::wake() {
  {
    std::lock_guard<std::mutex> lk(mu_);
    ++wakeTick_;
  }
  cv_.notify_one();
}

void Scheduler::requestStop() {
  {
    std::lock_guard<std::mutex> lk(mu_);
    stop_ = true;
  }
  cv_.notify_all();
}

void Scheduler::join() {
  if (worker_.joinable()) { worker_.join(); }
}

bool Scheduler::joinable() const { return worker_.joinable(); }

// --- marquee/scheduler thread ---------------------------------------------------------------------
void Scheduler::start() {
  // worker_ is the input thread's own field, so joinable() IS the started/stopped state and the spawn is
  // exception-safe: if std::thread's constructor throws, nothing was mutated and the scheduler stays unstarted.
  if (worker_.joinable()) { return; }
  worker_ = std::thread([this] { run(); });
}

int Scheduler::run() {
  for (;;) {
    {
      std::unique_lock<std::mutex> lk(mu_);
      if (stop_) { return 0; }
      const long long seen = wakeTick_;
      // A bounded TIMED WAIT, never a spin: it wakes on the render deadline, on a postEvent/stop/wake notify,
      // or when pollingMs elapses. It never holds a core while idle.
      cv_.wait_for(lk, std::chrono::milliseconds(pollTimeoutMsLocked()),
                   [this, seen] { return stop_ || dirty_ || wakeTick_ != seen; });
      if (stop_) { return 0; }              // release mu_ BEFORE stepping — rule 1
    }
    (void)tick(nullptr);                    // the input thread already fed the event via postEvent
  }
}

TickResult Scheduler::tick(const KeyEvent* ev) {
  TickResult res;
  // Rule 3: size() is a syscall and a marquee-thread-only resource, so it is read BEFORE the lock. It is also
  // tick()'s first action, which makes a count of size() calls an exact "this tick started" barrier — that is
  // what lets the concurrency tests synchronize without sleeping.
  const Size sz = term_.size();
  std::string frame;                        // assembled UNDER the lock, written OUTSIDE it (rule 2)
  {
    std::lock_guard<std::mutex> lk(mu_);
    const long long now = term_.nowMs();
    bool redraw = false;
    if (ev != nullptr) { interp_.feed(*ev); redraw = true; }
    // dirty_ is how a keystroke's echo frame is requested. The worker always steps with ev == nullptr, so
    // posting and stepping are decoupled and echo stays un-gated by refreshMs.
    if (dirty_) { dirty_ = false; redraw = true; }

    const bool running = proc_.state == ProcessState::Running;
    // !hasRendered makes a freshly started (or restarted) process draw its FIRST frame immediately.
    const bool due = running && (!proc_.hasRendered || now - proc_.lastRenderMs >= params_.refreshMs);
    if (due || redraw) {
      frame = renderer_.buildFrame(params_, proc_, interp_.prompt(), interp_.buffer(),
                                   interp_.lastMessage(), sz.rows, sz.cols);
      if (due) {
        proc_.lastRenderMs = now;
        proc_.hasRendered = true;
        proc_.cycles += 1;                  // cycles counts RENDERED frames, not redraws
        res.rendered = true;
      }
    }
    // The worker never asks the interpreter whether the user left: quit_ belongs to the input thread, and
    // shutdown reaches this thread solely through stop_ (set by requestStop()).
  }
  // The only terminal write in the program, on the marquee thread only, and outside mu_ so that a slow
  // console write can never stall the input thread (rule 2).
  if (!frame.empty()) {
    term_.write(frame);
    term_.flush();
  }
  return res;
}

int Scheduler::pollTimeoutMs() const {
  std::lock_guard<std::mutex> lk(mu_);
  return pollTimeoutMsLocked();
}

SchedulerSnapshot Scheduler::snapshot() const {
  std::lock_guard<std::mutex> lk(mu_);
  return {proc_.state,     proc_.cycles,     proc_.lastRenderMs,  term_.nowMs(),
          proc_.hasRendered, params_.refreshMs, params_.pollingMs};
}

// --- private --------------------------------------------------------------------------------------
int Scheduler::pollTimeoutMsLocked() const {   // run() already holds mu_ (tick() must NEVER be called holding it)
  long long ms = params_.pollingMs;
  if (proc_.state == ProcessState::Running) {
    const long long until = params_.refreshMs - (term_.nowMs() - proc_.lastRenderMs);
    if (until > 0 && until < ms) { ms = until; }
  }
  if (ms < 1)    { ms = 1; }                   // zero would busy-spin a core
  if (ms > 1000) { ms = 1000; }
  return static_cast<int>(ms);
}

}  // namespace csopesy
