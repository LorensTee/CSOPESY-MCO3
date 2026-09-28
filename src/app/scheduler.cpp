// Run the marquee worker and coordinate input and terminal output.
// The input thread handles events.
// The worker thread builds and writes frames.
// Use the mutex for shared state and the condition variable for worker wake-ups.
// Read the terminal size before locking the mutex.
// Do not perform terminal I/O while the mutex is locked.
// Use term_.nowMs() for application time.
#include <chrono>
#include <string>

#include "csopesy/interpreter.hpp"
#include "csopesy/scheduler.hpp"

namespace csopesy {

Scheduler::Scheduler(Terminal& term, Parameters& params, Renderer& renderer, Interpreter& interp,
                     MarqueeProcess& proc)
    : term_(term), params_(params), renderer_(renderer), interp_(interp), proc_(proc) {}

Scheduler::~Scheduler() {
  requestStop();
  join();   // Wait for the worker before dependent objects are destroyed.
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
  // A joinable worker means the scheduler has started.
  if (worker_.joinable()) { return; }
  worker_ = std::thread([this] { run(); });
}

int Scheduler::run() {
  for (;;) {
    {
      std::unique_lock<std::mutex> lk(mu_);
      if (stop_) { return 0; }
      const long long seen = wakeTick_;

      // Wake for a render deadline, input, an explicit wake request, a stop request, or pollingMs.
      // The timed wait prevents a busy loop.
      cv_.wait_for(lk, std::chrono::milliseconds(pollTimeoutMsLocked()),
                   [this, seen] { return stop_ || dirty_ || wakeTick_ != seen; });
      if (stop_) { return 0; }
    }
    (void)tick(nullptr);                    // Render due frame or process pending state.
  }
}

TickResult Scheduler::tick(const KeyEvent* ev) {
  TickResult res;

  // Read the terminal size before locking the mutex.
  const Size sz = term_.size();
  std::string frame;                        // Build the frame under the lock and write it after the lock.
  {
    std::lock_guard<std::mutex> lk(mu_);
    const long long now = term_.nowMs();
    bool redraw = false;
    if (ev != nullptr) { interp_.feed(*ev); redraw = true; }

    // dirty_ requests an echo frame for a posted keystroke.
    // The worker can render it without waiting for refreshMs.
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

  }

  // Keep terminal output outside the mutex.
  // Slow console I/O must not block input handling.
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
int Scheduler::pollTimeoutMsLocked() const {
  long long ms = params_.pollingMs;
  if (proc_.state == ProcessState::Running) {
    const long long until = params_.refreshMs - (term_.nowMs() - proc_.lastRenderMs);
    if (until > 0 && until < ms) { ms = until; }
  }
  if (ms < 1)    { ms = 1; }                   // Prevent a busy loop.
  if (ms > 1000) { ms = 1000; }
  return static_cast<int>(ms);
}

}  // namespace csopesy
