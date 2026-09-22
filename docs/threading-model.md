# The threading model

One page, written for the next maintainer. This is also the source of truth for the PPT's scheduler slide.

The program runs **two threads**, coordinated by **one `std::mutex`** (`Scheduler::mu_`) and **one
`std::condition_variable`** (`Scheduler::cv_`). There is no second mutex, no atomic-per-field design, and no
`detach()` anywhere.

## 1. What each thread does

**Input / command thread (`main`, via `ConsoleApp::run`).** Owns `Terminal::readEvent()` and therefore the
keyboard. It delivers each keystroke through `Scheduler::postEvent()` and never writes a frame. It also owns
`Interpreter::quit_` — the worker never asks the interpreter whether the user left.

**Marquee / scheduler thread (the worker, `Scheduler::run`).** Owns the clock read, the frame build and the
**only** `Terminal::write()` / `flush()` in the program. It loops: wait on `cv_` until a deadline or a state
change, release the lock, step once (`tick(nullptr)`), repeat. `stop_` is its only exit signal.

## 2. What is shared

Everything follows from *who writes what*: a resource with one writer needs no lock; one with two writers does.

| State | Writer(s) | Reader(s) | Protected by |
| --- | --- | --- | --- |
| `Parameters`, `MarqueeProcess`, `Interpreter`'s line/message | input thread (interpreted commands) **and** worker (`cycles`, `lastRenderMs`, `hasRendered`) | both | `mu_` |
| `Interpreter::quit_` | input thread only | input thread only | none needed — the worker never reads it; shutdown crosses as `stop_` |
| `Scheduler::stop_`, `dirty_`, `wakeTick_` | both | both | `mu_`; the *change* is delivered by `cv_` |
| `Scheduler::worker_` | input thread only (`start`/`join`/`~Scheduler`) | input thread only | none needed — **never** `join()` from the worker (self-join) |
| the frame `std::string`, every `Renderer` call | worker only | worker only | none needed |
| `Terminal::write`/`flush`/`size` | worker only | worker only | none needed |
| `Terminal::readEvent` | input thread only | input thread only | none needed |
| the shutdown flag (a signal handler's only output) | signal handler (write) | input thread (read-and-clear) | `volatile std::sig_atomic_t` |

Taking a lock is the *exception*, not the rule: the terminal, the renderer and the frame have exactly one
owning thread. `mu_` covers only the state that genuinely crosses the boundary — which is why this design needs
one mutex rather than atomics sprinkled over every field.

## 3. How synchronization works, and the three lock rules

1. **`tick()` is never called while `mu_` is held.** `run()` releases its `unique_lock` before stepping, and
   `postEvent`/`wake`/`requestStop` hold `mu_` only for field writes.
2. **Every blocking call happens outside `mu_`.** The frame is assembled into one `std::string` under the lock
   and written *after* it is dropped, so an unbounded console write can never stall the input thread.
3. **`Terminal::size()` is read before the lock.** It is a syscall and it belongs to the worker alone, so it
   needs no protection; there is no second mutex, hence no lock order to get wrong.

`refreshMs` is the render **deadline** — a frame is due when the process is `Running` and
`!hasRendered || now - lastRenderMs >= refreshMs`; `!hasRendered` is why `start_marquee` paints immediately. A
**redraw** is also emitted for every consumed event, and `cycles` counts only *rendered* frames. Resize needs no
state: every frame is rebuilt whole from the current `Terminal::size()`, so the next frame simply uses the new
size instead of the old one being patched.

## 4. `refresh_ms` versus `polling_ms`

`refreshMs` is the only thing that drives the marquee. `pollingMs` is a **maximum idle wait** for the worker's
`cv_.wait_for`: the wait is the smaller of `pollingMs` and the time left until the next render deadline, so
`pollingMs` can never delay a frame. It is not a key-sampling interval either — `readEvent` returns the instant a
key arrives, so `pollingMs` does not add keystroke latency. What it genuinely bounds is **idle wakeups per
second**. Note the architecture's price: there are now **two** idle waiters instead of one, so at the same
`pollingMs` the idle-wakeup count is roughly doubled.

## 5. Shutdown

Intent (`quit_` / the signal flag) → **observe** (the input thread, the only reader of both) →
`requestStop()` (sets `stop_`, notifies) → `join()` (the worker `return`s from `run`) → **restore** the terminal.
The join precedes `restore()` because `restore()` must be the last writer to the terminal — the worker may still
be mid-frame otherwise. `~Scheduler` does `requestStop()` + `join()`, so a worker can never outlive the objects
it touches, and there is no detached thread to leak.

## 6. Why typing stays responsive

Echo is **event-driven**: `postEvent()` sets `dirty_` and wakes the worker, so the echo frame is not gated on
`refreshMs` (at `refreshMs = 10000` a keystroke still repaints immediately). And the two rules that keep the
critical section free of I/O mean the input thread waits on a *frame's render*, never on a console write.

## 7. The honest limits (do not overclaim these)

1. **Two idle waiters roughly double idle wakeups** at the same `pollingMs` — the price of the architecture, not
   something measurement removed.
2. **Threads do not make terminal repaint atomic.** One assembled frame per `write()` prevents *application-level*
   interleaving of two frames, and the single writer means two frames cannot even be built concurrently. It does
   **not** make the terminal's own repaint atomic, and no escape sequence is added to chase that.
3. **Command processing can still briefly wait behind one frame's render.** The guarantee is *"the unbounded
   console write can never stall the input thread"* — not *"the input thread never waits"*. Rule 2 removes the
   console write from that path, not every wait.
