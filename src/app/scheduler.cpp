// src/app/scheduler.cpp — the marquee worker + the input-thread API (v3.0 §3.8).
//
// TODO(T1.3): the two-thread model (one mutex, one condition variable, one owner per resource) and the three
// lock rules — tick() is never called holding mu_, every blocking I/O call happens outside mu_, and
// Terminal::size() is read before the lock. Deadlines come from term_.nowMs() (D7), and the worker publishes
// the frame as ONE string from Renderer::buildFrame through ONE term_.write() (D6).
#include "csopesy/scheduler.hpp"

namespace csopesy {

Scheduler::Scheduler(Terminal& term, Parameters& params, Renderer& renderer, Interpreter& interp,
                     MarqueeProcess& proc)
    : term_(term), params_(params), renderer_(renderer), interp_(interp), proc_(proc) {}

Scheduler::~Scheduler() {
  requestStop();
  join();   // NEVER detaches; a worker can never outlive the objects it touches (§3.8)
}

// --- input/command thread -------------------------------------------------------------------------
void Scheduler::postEvent(const KeyEvent&) { /* TODO(T1.3): feed the interpreter + dirty_ = true + notify */ }

void Scheduler::wake() { /* TODO(T1.3): bump wakeTick_ + notify */ }

void Scheduler::requestStop() { /* TODO(T1.3): stop_ = true + notify_all */ }

void Scheduler::join() { /* TODO(T1.3): join worker_ if joinable */ }

bool Scheduler::joinable() const { return worker_.joinable(); }

// --- marquee/scheduler thread ---------------------------------------------------------------------
void Scheduler::start() { /* TODO(T1.3): spawn exactly ONE worker */ }

int Scheduler::run() {
  // TODO(T1.3): wait_for(deadline | wake) then tick(nullptr); stop_ is the only exit condition.
  return 0;
}

TickResult Scheduler::tick(const KeyEvent*) {
  // TODO(T1.3): build the frame under mu_, then write that ONE string outside mu_ (§3.9).
  return {};
}

int Scheduler::pollTimeoutMs() const {
  // TODO(T1.3): clamp(min(polling, msUntilNextRender), 1, 1000). Never 0: a 0 would busy-spin a core.
  return 1;
}

SchedulerSnapshot Scheduler::snapshot() const {
  // TODO(T1.3): lock mu_ and copy the sanctioned cross-thread view. `now` comes from term_.nowMs() (D7).
  return {};
}

// --- private --------------------------------------------------------------------------------------
int Scheduler::pollTimeoutMsLocked() const { return 1; }   // TODO(T1.3)

}  // namespace csopesy
