// tests/support/fake_terminal.hpp — the in-process Terminal double (§T1.1, upgraded for §T1.3).
#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include "csopesy/terminal.hpp"
namespace csopesy {

// Public plain fields (`sz`, `clock`, `events`, `out`) are for the single-threaded pure-step tests, where no
// worker exists and nothing can race. Any test with a live worker must use push()/outCopy()/waitFor*()
// instead, and must read scheduler state through Scheduler::snapshot() or after join().
//
// Only this double needed a threading upgrade: the real backends read fd 0 / write fd 1, while this one is
// in-process state, so the two-thread read/write split has to be made explicitly safe here.
class FakeTerminal : public Terminal {
 public:
  std::atomic<Size> sz{Size{24, 80}};
  std::atomic<long long> clock{0};
  std::deque<KeyEvent> events;   // test thread only; use push() once a worker may be running
  std::string out;               // appended under m_ by the marquee thread; read it with outCopy()

  void enterRawMode() override {}
  void restore() override {}

  // size() is called exactly once per tick() and it is tick()'s first action — so a count of size() calls is
  // a precise "tick n has started" barrier, and no sleep is needed anywhere in the suite.
  Size size() const override {
    std::lock_guard<std::mutex> lk(m_);
    ++ticks_;
    tickCv_.notify_all();
    return sz.load();
  }

  // No input source of its own: a double cannot block on a console, so an empty queue is an immediate
  // "timeout" (false). The threaded tests ARE the input thread and call this synchronously, which is what
  // keeps the suite free of real-time sleeps.
  bool readEvent(KeyEvent& e, int) override {
    std::lock_guard<std::mutex> lk(m_);
    if (events.empty()) { return false; }
    e = events.front();
    events.pop_front();
    return true;
  }

  void write(std::string_view bytes) override {
    std::lock_guard<std::mutex> lk(m_);
    out.append(bytes);
    writes_.emplace_back(bytes);
    writers_.push_back(std::this_thread::get_id());
    ++frames_;
    frameCv_.notify_all();
  }

  void flush() override {}
  bool isTty() const override { return false; }
  long long nowMs() const override { return clock.load(); }

  // --- explicit synchronization for the threaded tests: barriers, never timers ---
  void push(KeyEvent e) {
    std::lock_guard<std::mutex> lk(m_);
    events.push_back(e);
  }
  // True once tick #n has STARTED, i.e. ticks 1..n-1 have completed. The `ms` bound is a hang guard for a
  // missing join or a lost notify — never a clock the test depends on.
  bool waitForTickStarted(size_t n, int ms) {
    std::unique_lock<std::mutex> lk(m_);
    return tickCv_.wait_for(lk, std::chrono::milliseconds(ms), [&] { return ticks_ >= n; });
  }
  bool waitForFrames(size_t n, int ms) {
    std::unique_lock<std::mutex> lk(m_);
    return frameCv_.wait_for(lk, std::chrono::milliseconds(ms), [&] { return frames_ >= n; });
  }
  size_t tickCount() const { std::lock_guard<std::mutex> lk(m_); return ticks_; }
  size_t frameCount() const { std::lock_guard<std::mutex> lk(m_); return frames_; }
  std::string outCopy() const { std::lock_guard<std::mutex> lk(m_); return out; }
  std::vector<std::string> writesCopy() const {
    std::lock_guard<std::mutex> lk(m_);
    return writes_;
  }
  std::vector<std::thread::id> writersCopy() const {
    std::lock_guard<std::mutex> lk(m_);
    return writers_;
  }

 private:
  // m_ is a LEAF lock and it is never held together with Scheduler::mu_: size() is called before mu_ is taken
  // and write()/flush() after it is released, so the two locks never nest and there is no lock order to obey.
  // The real backends take no lock here at all.
  mutable std::mutex m_;
  mutable std::condition_variable tickCv_, frameCv_;   // size() is const but it is the tick barrier
  std::vector<std::string> writes_;
  std::vector<std::thread::id> writers_;
  size_t frames_ = 0;
  mutable size_t ticks_ = 0;                            // incremented from the const size()
};
}  // namespace csopesy
