// include/csopesy/scheduler.hpp — scheduler and worker-thread coordination.
//
// Scheduler owns the coordination between the input thread and the marquee worker.
// It uses one mutex and one condition variable.
// The worker is the only thread that writes to the terminal.
// Terminal::size() is read before the mutex is locked.
// Terminal I/O is performed after the mutex is released.
// Terminal::nowMs() is the only clock source.

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

struct TickResult {
  bool rendered = false;
};

struct SchedulerSnapshot {
  ProcessState state = ProcessState::Stopped;
  long long cycles = 0;
  long long lastRenderMs = 0;
  long long now = 0;
  bool hasRendered = false;
  int refreshMs = 0;
  int pollingMs = 0;
};

class Scheduler {
 public:
  Scheduler(Terminal&, Parameters&, Renderer&, Interpreter&, MarqueeProcess&);
  ~Scheduler();

  // Input thread operations.
  void postEvent(const KeyEvent& ev);
  void wake();          // Tell the worker to re-check its state.
  void requestStop();   // Request a worker stop and wake the worker.
  void join();          // Wait for the worker to stop.
  bool joinable() const;

  // Worker thread operations.
  void start();         // Start one worker thread.
  int run();             // Run the worker loop until a stop is requested.

  // Advance the scheduler by one step.
  // Production calls tick(nullptr) from the worker. The event form is for deterministic tests.
  TickResult tick(const KeyEvent* ev);

  // Return the wait time before the worker checks again.
  int pollTimeoutMs() const;

  // Return a synchronized copy of scheduler state.
  SchedulerSnapshot snapshot() const;

 private:
  // Called while mu_ is locked.
  int pollTimeoutMsLocked() const;

  Terminal& term_;
  Parameters& params_;
  Renderer& renderer_;
  Interpreter& interp_;
  MarqueeProcess& proc_;

  // Protect scheduler state and the shared Parameters, Interpreter, and process state.
  mutable std::mutex mu_;
  std::condition_variable cv_;

  // The input thread starts and joins the worker.
  std::thread worker_;

  // stop_ ends the worker loop. dirty_ requests a frame for input echo.
  bool stop_ = false;
  bool dirty_ = false;

  // Changes when wake() requests another scheduler check.
  long long wakeTick_ = 0;
};
}
