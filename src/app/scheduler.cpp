// src/app/scheduler.cpp — the marquee worker and input-thread scheduler API.
//
// The scheduler uses two threads, one mutex, and one condition variable.
//   Input thread   : reads events and sends them to the interpreter.
//                    It never writes a frame.
//   Marquee thread : runs the worker loop and writes frames to the terminal.
//
// These rules prevent deadlocks and keep the lock scope small:
//   1. Do not call tick() while mu_ is locked.
//   2. Do not perform blocking terminal I/O while mu_ is locked.
//   3. Read Terminal::size() before locking mu_.
//
// Use term_.nowMs() as the time source. <chrono> is only used to set the wait duration.
#include <chrono>
#include <string>

#include "csopesy/interpreter.hpp"   // Keep app/ imports above features/commands imports.
#include "csopesy/scheduler.hpp"

namespace csopesy {

Scheduler::Scheduler(Terminal& term, Parameters& params, Renderer& renderer, Interpreter& interp,
                     MarqueeProcess& proc)
    : term_(term), params_(params), renderer_(renderer), interp_(interp), proc_(proc) {}

Scheduler::~Scheduler() {
  requestStop();
  join();   // Keep the worker alive only until all objects it uses are still valid.
}

// --- input/command thread -------------------------------------------------------------------------
void Scheduler::postEvent(const KeyEvent& ev) {
  {
    std::lock_guard<std::mutex> lk(mu_);
    interp_.feed(ev);
    dirty_ = true;          // Request an echo frame without waiting for refreshMs.
  }
  cv_.notify_all();         // Wake the worker after the input is ready.
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
  // worker_.joinable() is the started state. If thread creation fails, the scheduler stays unstarted.
  if (worker_.joinable()) { return; }
  worker_ = std::thread([this] { run(); });
}

int Scheduler::run() {
  for (;;) {
    {
      std::unique_lock<std::mutex> lk(mu_);
      if (stop_) { return 0; }
      const long long seen = wakeTick_;

      // Wait for the render deadline, an input event, a wake request, a stop request, or pollingMs.
      // The timed wait prevents a busy loop while the program is idle.
      cv_.wait_for(lk, std::chrono::milliseconds(pollTimeoutMsLocked()),
                   [this, seen] { return stop_ || dirty_ || wakeTick_ != seen; });
      if (stop_) { return 0; }              // Release mu_ before tick().
    }
    (void)tick(nullptr);                    // postEvent() already delivered the input event.
  }
}

TickResult Scheduler::tick(const KeyEvent* ev) {
  TickResult res;

  // Read the terminal size before locking. This call belongs to the marquee thread.
  // Reading it first also gives the concurrency tests a clear point at which tick() has started.
  const Size sz = term_.size();
  std::string frame;                        // Build the frame under the lock and write it after the lock.
  {
    std::lock_guard<std::mutex> lk(mu_);
    const long long now = term_.nowMs();
    bool redraw = false;
    if (ev != nullptr) { interp_.feed(*ev); redraw = true; }

    // dirty_ requests the echo frame for a posted keystroke. The worker calls tick() with nullptr,
    // so input echo does not wait for refreshMs.
    if (dirty_) { dirty_ = false; redraw = true; }

    const bool running = proc_.state == ProcessState::Running;

    // Draw the first frame immediately after a process starts or restarts.
    const bool due = running && (!proc_.hasRendered || now - proc_.lastRenderMs >= params_.refreshMs);
    if (due || redraw) {
      frame = renderer_.buildFrame(params_, proc_, interp_.prompt(), interp_.buffer(),
                                   interp_.lastMessage(), sz.rows, sz.cols);
      if (due) {
        proc_.lastRenderMs = now;
        proc_.hasRendered = true;
        proc_.cycles += 1;                  // Count rendered frames, not redraws.
        res.rendered = true;
      }
    }

    // The input thread owns interpreter quit state. The worker stops only when stop_ is set.
  }

  // The marquee thread performs the only terminal write.
  // Keep the write outside mu_ so slow console I/O cannot block input handling.
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
int Scheduler::pollTimeoutMsLocked() const {   // run() already holds mu_.
  long long ms = params_.pollingMs;
  if (proc_.state == ProcessState::Running) {
    const long long until = params_.refreshMs - (term_.nowMs() - proc_.lastRenderMs);
    if (until > 0 && until < ms) { ms = until; }
  }
  if (ms < 1)    { ms = 1; }                   // Prevent a zero-duration busy loop.
  if (ms > 1000) { ms = 1000; }
  return static_cast<int>(ms);
}

}  // namespace csopesy
