# CSOPESY MCO3 — Technical Report (PPT Outline)

**Project:** Marquee Console — a two-thread C++17 terminal program
**Repository:** https://github.com/LorensTee/CSOPESY-MCO3
**Graded platform:** Windows (Windows 11, x86_64, real Win32 console)
**Contracts:** v3.0 (`CONTRACTS.md`, 11 frozen headers)
**Entry point:** `src/main.cpp` → `csopesy::ConsoleApp` in `src/app/console_app.cpp`

---

## How to read this report

Every claim is tagged so a reviewer can tell implementation from inference from observation:

| Tag | Meaning |
| --- | --- |
| **[CODE]** | Directly implemented in a named source file / function. |
| **[TEST]** | Asserted by a committed unit test or the CI/smoke suite. |
| **[OBSERVED]** | Recorded by the frozen-binary measurement sweep in `docs/measurements/`. |
| **[INFERRED]** | A conclusion that follows from the code or the recorded data, but is not itself directly asserted. |
| **[RECOMMENDED]** | A recommendation for the graded configuration; not a measured requirement. |

**Honest-limitation banner (do not omit on the slide):**
> The refresh/polling numbers are **observed thresholds from a manual sweep**, not per-keystroke latency
> measurements. v3 has no in-process telemetry by design (`docs/measurements/README.md`, D4). Only the
> **Windows** tables are COMPLETE; the Linux and macOS tables are still `PENDING (T6.3)`.

---

## Slide A — System architecture in one picture

**[CODE] Two threads, one mutex, one condition variable, one terminal writer.**

```
              +-------------------------------------------------------+
              |  main()  (src/main.cpp)                               |
              |  parseCli -> Terminal::create() -> signal handlers ->  |
              |  TerminalGuard(raw mode) -> ConsoleApp::run()          |
              +---------------------------+---------------------------+
                                          |
                        ConsoleApp::run() (src/app/console_app.cpp)
                                          |
        (TTY mode) draws initial header, then START Scheduler and
        BECOMES the input/command thread          (non-tty / --no-tty)
              |            \                        runPlainLineMode():
              |             \                       no raw mode, no ANSI,
              v              v                       no frames, NO worker
   +------------------+   +--------------------------+
   | INPUT (main)     |   | MARQUEE WORKER           |
   | Terminal::       |   | Scheduler::run()         |
   |   readEvent()    |   |  cv_.wait_for(...)       |
   | Scheduler::      |   |  tick(nullptr)           |
   |   postEvent()    |   |  Renderer::buildFrame()  |
   | Interpreter::feed|   |  Terminal::write()       |
   | (never writes)   |   | (ONLY writer)            |
   +--------+---------+   +-------------+------------+
            |                            |
            +---------- Scheduler::mu_ ---+
                       Scheduler::cv_
   data: Parameters, MarqueeProcess, Interpreter line/message, stop_/dirty_/wakeTick_
```

- **Layer rules (enforced by `scripts/check_layers.sh`):** `app > features > entities > shared`; `platform/`
  is a peer of `shared/` and may import `shared/` only. `features/marquee` and `features/commands` may not
  cross-import. A new contract header not in the guard's `layer_of_header()` map is a **hard failure**.
- **Why two threads:** professor answer #3 mandates one thread for the marquee/scheduler and one for the
  command interpreter (`docs/IMPLEMENTATION_PLAN_v3.md` §1.2, D8).
- **Why one clock:** `Terminal::nowMs()` is the only clock source; `Scheduler` never calls `std::chrono` as a
  clock (`scheduler.cpp` header comment).
- **Why one writer:** the worker is the only `Terminal::write()` caller while it lives; the input thread never
  draws a frame (`console_app.cpp`, `docs/threading-model.md`).

---

# Section 1 — Command Recognition

## Slide 1.1 — What recognition is and where it lives

**[CODE]** Recognition is a two-stage process:

1. **Line editor** (`src/features/commands/line_editor.cpp`, member `Interpreter::feed`) turns `KeyEvent`s
   into a current line buffer. It is not the recognizer; it validates *entry*.
2. **Command dispatcher** (`src/features/commands/interpreter.cpp`, `Interpreter::executeLine`) splits the
   line into `cmd` + `arg` and looks `cmd` up.

The command table is a `static const std::unordered_map<std::string, Cmd>` in an anonymous namespace of
`interpreter.cpp`:

| Token | `Cmd` |
| --- | --- |
| `help` | `Help` |
| `start_marquee` | `Start` |
| `stop_marquee` | `Stop` |
| `set_text` | `SetText` |
| `set_speed` | `SetSpeed` |
| `exit` | `Exit` |
| anything else | `Unknown` |

## Slide 1.2 — Recognition rules (exact, testable)

**[CODE]** From `interpreter.cpp`:

- **Case-sensitive, exact match.** `lookup()` finds the map key verbatim; `HELP` returns `Unknown`
  (test `test_interpreter.cpp`; acceptance A7). Rationale: the handout names six exact strings.
- **Outer whitespace trimmed, internal runs preserved.** `trim()` removes leading/trailing ASCII whitespace
  (`isSpace`: space, tab, `\n`, `\r`, `\f`, `\v`); `set_text` keeps internal spaces verbatim
  (`Parameters::setText`).
- **Split is at the first whitespace run:** `cmd` is the first token, `arg` is the trimmed remainder.
- **Empty line:** cleared message, no command executed (`executeLine` early return on `line.empty()`).
- **No-argument commands reject extra text.** `help`, `start_marquee`, `stop_marquee`, `exit` all use
  `Usage: <cmd>` when `arg` is non-empty; state is unchanged (`exit hello` does **not** quit). This was
  pre-freeze issue #1, fixed in commit `334d37b`.
- **`set_speed` argument must match `[-+]?[0-9]+` in full.** `parseFullInteger()` rejects `5abc`, `1.5`,
  `100 200`, and `+`/`-` alone. This is deliberately *not* `std::stoi`, which would silently parse `5abc`.
- **`set_text` accepts printable ASCII `[0x20, 0x7E]` only.** `Parameters::setText` returns
  `TextResult::NonAscii` and leaves text unchanged; rule 7 / D15.

## Slide 1.3 — Why it is built this way (trade-offs and edge cases)

- **Why a table, not a chain of `if`s:** one lookup, one place to see the whole recognised set; `Unknown` is
  the default, so a typo can never fall through to a wrong command.
- **Why full-match numeric parsing instead of `std::stoi`:** `set_speed 5abc` must be a `Usage:` error, not a
  silent `5 ms` (`test_interpreter.cpp`).
- **Why a well-formed out-of-range number is *clamped*, not a usage error:** `-5`, `0`, `10001`, and huge
  literals become [`1`, `10000`] with an explicit report. `CliReport`/`ClampReport` carries
  `requested`, `applied`, `clamped` (`parameters.cpp`).
- **Why `HELP` is unrecognised:** matching the spec literally is graded ("varying inputs"); accepting `HELP`
  would be an invented behaviour.
- **Edge case — quote in the echo line:** `set_text` success echoes
  `Marquee text set to "<text>".` verbatim; the text is never normalized.
- **[INFERRED]** Recognition is intentionally cheap: the dispatcher does no I/O and touches no lock. It is
  called by the input thread inside `Scheduler::postEvent`'s critical section.

---

# Section 2 — Console UI Implementation

## Slide 2.1 — One frame, one string, one writer

**[CODE]** `src/features/marquee/renderer.cpp`, `Renderer::buildFrame(...) -> std::string`.

- Builds the **entire** screen as one `std::string`: cursor-home (`ESC[H`), then exactly `rows` rows
  separated by `\r\n`.
- Every row is padded or clipped to exactly `cols` columns (`padOrClip`). Width equals terminal width, so no
  line wraps and no stale columns survive.
- Raw mode on both platforms disables newline translation (POSIX clears `OPOST`/`ONLCR`; Win32 sets
  `DISABLE_NEWLINE_AUTO_RETURN`), so **the renderer owns its own carriage returns**.
- The worker calls `Terminal::write(frame)` **once** and `flush()` **once** per frame
  (`Scheduler::tick`). One assembled frame per `write()` prevents application-level interleaving of two
  frames.
- **[INFERRED]** This is *not* atomic terminal repaint. The plan explicitly forbids claiming that
  (`IMPLEMENTATION_PLAN_v3.md` §3.9; `docs/threading-model.md` §7.2). No escape sequence is added to chase
  it.

## Slide 2.2 — Layout (from `buildFrame`)

`Renderer::kBandRow = 3` (fixed, near the top; professor answer #5 / D3). Band width is
`bandWidthFor(cols) = max(1, cols - 2)` — one column of margin each side.

```
row 1        Welcome to CSOPESY!                 <- capital W
row 2        (blank)
row 3        band: ONE row of scrolling text     <- kBandRow
row 4        (blank)
row 5        Group developer:
row 6..n     one developer name per row
             (blank)
             Version date: <versionDate>          <- no trailing space when blank
             (blank)
last row     Command> <tail-window-of-typed-text>
```

- **Message rows** (`interpreter.lastMessage()`) are bottom-anchored directly above the prompt, newest lines
  kept; `help`'s six-line block is visible.
- **Prompt row is always last** and shows the **tail** of the buffer (`buildPromptRow`) so the cursor stays
  in view on one fixed row.
- **Tight terminals — defined priority, not an accident:** chrome is dropped bottom-up in the order
  `Version date:` → developer names → `Group developer:` → welcome. **The band and the prompt are never
  dropped.** On a `20×5` terminal the band falls back to the row above the block; the prompt stays last
  (acceptance A11).
- **[CODE]** The frame never writes the terminal's **bottom-right cell**: the last row stops at `cols - 1`
  and is closed with erase-to-EOL (`ESC[K`). This is pre-freeze issue #3: writing that cell made some
  terminals scroll a line per frame and accumulate old frames.

## Slide 2.3 — The marquee scroll math (pure functions)

**[CODE]** `renderer.cpp`:

- `scrollOffset(cycles, textWidth, bandWidth)`: wrap period `textWidth + bandWidth`; result in
  `[0, period)`. `offset == 0` is a fully blank band (text just left); `offset == bandWidth` puts `text[0]`
  in column 0. The text scrolls **fully off** before re-entering, so the band reads as a marquee, not a
  smear. A negative modulus is normalized.
- `sliceRow(text, bandWidth, offset)`: `windowLeft = offset - bandWidth`; returns **exactly** `bandWidth`
  bytes, padding out-of-range positions with spaces. This width invariant is what the whole layout depends
  on (`test_scroll.cpp`).
- `cycles` is `MarqueeProcess::cycles` and increments **only on rendered deadline frames**, never on redraws
  (`Scheduler::tick`).

## Slide 2.4 — Plain line mode (`--no-tty` or non-tty stdin)

**[CODE]** `ConsoleApp::runPlainLineMode` (`console_app.cpp`):

- No raw mode, no ANSI, no frames, **no worker thread**.
- Reads stdin lines with `std::getline`, prints `interpreter.executeLine(line)` responses as ordinary lines.
- Selector is decided once in `main()` before any thread exists: `cli.params.noTty || !term->isTty()`.
- **Why it exists:** it makes the real-binary smoke test (`tests/smoke/`) byte-portable across the three CI
  images (Linux/Windows/macOS). It is **dev/CI only** (D13) and never part of the graded run.

## Slide 2.5 — Platform surface (the only `#ifdef`-free abstraction)

**[CODE]** `include/csopesy/terminal.hpp` is the platform contract; `src/platform/terminal_posix.cpp` and
`src/platform/terminal_win32.cpp` are the two implementations. CMake selects exactly one; there is no runtime
`#ifdef` anywhere else.

| Concern | POSIX | Win32 |
| --- | --- | --- |
| Raw mode | `tcgetattr`/`tcsetattr`; clear `ICANON/ECHO/ISIG`, clear `OPOST`, clear `ICRNL`/`IXON` | `SetConsoleMode`; clear cooked input + QuickEdit/mouse/window; add VT output |
| Read with timeout | `poll()` + `read()` | `WaitForSingleObject(consoleIn, timeoutMs)` then `_kbhit`/`_getch` |
| Size | `ioctl(TIOCGWINSZ)` | `GetConsoleScreenBufferInfo` visible window (`srWindow`) |
| Clock | `clock_gettime(CLOCK_MONOTONIC)` | `GetTickCount64()` |
| VT failure | n/a | **loud startup error** (v3 replacement for `--diag`, D5) |

- **[INFERRED]** Both backends re-read `size()` every tick, so resize needs no event and no invalidation
  flag: the next frame is simply laid out for the new size (`docs/clion-run-config.md`).
- **[CODE]** `_getch()` arrow keys arrive as `0`/`224` scan-code prefixes; VT **input** is deliberately not
  enabled (risk 17). POSIX decodes CSI/SS3 with a 5 ms follow-up deadline for a bare `Esc`.

---

# Section 3 — Command Interpreter Implementation

## Slide 3.1 — Two responsibilities, one owner of state

**[CODE]** The interpreter is split across two translation units but one class:

- `Interpreter` (`include/csopesy/interpreter.hpp`): editing state `line_`, `message_`, `quit_`; methods
  `feed`, `executeLine`, `quitRequested`, `prompt`, `buffer`, `lastMessage`.
- `line_editor.cpp`: `Interpreter::feed` + the free function `visibleSlice(buffer, availWidth)`.
- `interpreter.cpp`: `Interpreter::executeLine` + the response table.
- `line_editor.hpp` deliberately declares **nothing of its own** — state has one owner.

**Why this matters:** `Interpreter` takes **no lock itself**; the caller owns it
(`IMPLEMENTATION_PLAN_v3.md` §3.11). The input thread owns `quit_` exclusively; the worker never reads it.

## Slide 3.2 — `feed()`: keystroke to line

**[CODE]** `line_editor.cpp`:

| Key | Behaviour |
| --- | --- |
| `Char` `[0x20,0x7E]` | append; other bytes ignored on entry |
| `Backspace` | pop last char (no-op when empty) |
| `Enter` | copy buffer, clear it, `message_ = executeLine(submitted)`, return `true` |
| `Eof` | same as Enter, then `quit_ = true`; return `true` |
| arrows / `Tab` / `Escape` / `None` | ignored, no state change |

- **Why echo is manual:** raw mode disables kernel echo, so the app redraws the buffer in the frame's prompt
  row.
- **Why `Eof` means quit:** POSIX raw mode has `ISIG` off, so Ctrl+C, Ctrl+D and a closed stdin all arrive
  as `KeyType::Eof`. This makes the non-signal exit paths work (§3.10).
- `visibleSlice(buffer, availWidth)` shows the buffer's **tail** when it exceeds the window so the cursor
  stays visible. **[CODE]** The renderer keeps a local copy of the same tail rule
  (`buildPromptRow`) because `features/commands` and `features/marquee` may not cross-import.

## Slide 3.3 — `executeLine()`: the response table

**[CODE]** `interpreter.cpp`. Responses (asserted by `test_interpreter.cpp`, acceptance A1–A9):

| Command | Response |
| --- | --- |
| `help` | six-line `Available commands:` block |
| `start_marquee` | `Marquee started.` / `Marquee is already running.` |
| `stop_marquee` | `Marquee stopped.` / `Marquee is not running.` |
| `set_text <t>` | `Marquee text set to "<t>".` |
| `set_text` (empty) | `Usage: set_text <text>` |
| `set_text` (non-ASCII) | `set_text: only printable ASCII characters are supported.` |
| `set_speed <n>` | `Marquee speed set to <applied> ms.` |
| `set_speed <out-of-range>` | `Speed <requested> ms is out of range [1, 10000]; clamped to <applied> ms.` |
| `set_speed` (missing/bad) | `Usage: set_speed <milliseconds>` |
| no-arg cmd with extra text | `Usage: <cmd>` |
| `exit` | `Exiting CSOPESY. Goodbye!` + `quit_ = true` |
| anything else | `Command not recognized: "<cmd>". Type "help" for the list of commands.` |

- **[CODE]** `start_marquee`/`stop_marquee` delegate to `MarqueeProcess::start()/stop()`; the interpreter
  owns the wording, the PCB owns the state.
- **[INFERRED]** The response strings are the user-visible CLI contract; the test rule is to assert the
  *token* for state messages and the *whole line* for `Usage:`/goodbye (plan §3.7).

## Slide 3.4 — Data flow for one typed command

```
keystroke bytes
  -> Terminal::readEvent()            (input thread; POSIX poll / Win32 WaitForSingleObject)
  -> ConsoleApp: Scheduler::postEvent()
        { lock mu_; interp_.feed(ev); dirty_ = true; }  cv_.notify_all()
  -> worker wakes; tick(nullptr)
        { lock mu_; build frame (prompt + message + band); dirty_=false; }
        term_.write(frame); term_.flush()   (outside mu_)
```

- **Why this design:** posting and stepping are decoupled. The echo frame is requested via `dirty_`, so
  keystroke echo is **not gated by `refreshMs`** (test
  `posted_keystroke_echo_is_written_without_waiting_for_the_refresh_deadline`; at `refreshMs = 10000` a
  keystroke still repaints immediately).
- **No I/O inside `mu_`:** the blocking `write()` is after the lock is dropped, so an unbounded console write
  can never stall the input thread (`docs/threading-model.md` §3 rule 2).

---

# Section 4 — Process Representation

## Slide 4.1 — The PCB is `MarqueeProcess`

**[CODE]** `include/csopesy/process.hpp` — a deliberately minimal Process Control Block:

```cpp
enum class ProcessState { Stopped, Running };   // Ready/Finished unused

struct MarqueeProcess {            // PCB
  int pid = 1;
  std::string name = "marquee";
  ProcessState state = ProcessState::Stopped;
  long long cycles = 0;            // rendered frames == "instructions executed"
  long long lastRenderMs = 0;
  bool hasRendered = false;        // false => next tick draws immediately
  bool start();                    // false if already Running; clears hasRendered
  bool stop();                     // false if already Stopped
};
```

- `start()`/`stop()` are implemented in `src/entities/process.cpp`. They are **idempotent with a reported
  result**: a second `start()` returns `false`, which is how the interpreter distinguishes
  `Marquee started.` from `Marquee is already running.`
- **`cycles` is the "instructions executed" analogue**: it increments once per **rendered** frame, not per
  redraw (`Scheduler::tick`). `cycles` drives `scrollOffset`.
- **`hasRendered`** is the freshness bit that makes `start_marquee` (or a restart) paint on the very next
  tick instead of waiting out a stale deadline. `start()` clears it.
- **[INFERRED]** `pid`/`name` are stable identifiers for the single marquee process; there is exactly one
  PCB, owned by `ConsoleApp::run` as a local and referenced by `Interpreter` and `Scheduler`.

## Slide 4.2 — State machine and ownership

**[CODE]** Only two states exist.

```
            MarqueeProcess::start()                 MarqueeProcess::stop()
 Stopped  --------------------------->  Running  --------------------------->  Stopped
          (returns false if Running)              (returns false if Stopped)
          clears hasRendered
```

- **Render deadline condition** (`Scheduler::tick`):
  `due = (state == Running) && (!hasRendered || now - lastRenderMs >= refreshMs)`.
- **Redraw is separate from rendering:** a consumed event sets `dirty_`; `dirty_` forces a frame build but
  does **not** touch `cycles` and does **not** update `lastRenderMs` (`test_scheduler.cpp`,
  `a_redraw_is_not_a_rendered_frame`).
- **Ownership ([CODE], `docs/threading-model.md` §2):** `Parameters`, `MarqueeProcess`, and the interpreter's
  `line_`/`message_` are written by the input thread (commands) **and** the worker (`cycles`,
  `lastRenderMs`, `hasRendered`), so all accesses are under `Scheduler::mu_`. `quit_` is single-threaded.

## Slide 4.3 — Why a tiny PCB (trade-off)

- **What problem it solves:** the assignment grades "process representation". A real OS PCB is far larger
  than this emulator needs; the small struct keeps the shared state obvious and the lock boundaries trivial.
- **Trade-off:** no `Ready`/`Finished`, no priorities, no per-process stack, no multiple processes — v1 kept
  them "as decoration" and v3 removed them.
- **Out of scope explicitly:** a thread per marquee process is *not* planned at any scope
  (`IMPLEMENTATION_PLAN_v3.md` §10.5).
- **Edge case — restart:** because `start()` clears `hasRendered`, a stopped-then-started marquee repaints
  immediately (test `start_and_restart_render_immediately`).

---

# Section 5 — Scheduler Implementation

## Slide 5.1 — The contract: two threads, one mutex, one cv

**[CODE]** `include/csopesy/scheduler.hpp` + `src/app/scheduler.cpp`.

| Resource | Owner / access |
| --- | --- |
| `Scheduler::mu_` | guards `Parameters`, `MarqueeProcess`, interpreter line/message, `stop_`, `dirty_`, `wakeTick_` |
| `Scheduler::cv_` | one condition variable; predicates `stop_ \|\| dirty_ \|\| wakeTick_ != seen` |
| `Scheduler::worker_` | input thread only (`start`/`join`/`~Scheduler`); **never** `join()` from the worker |
| `Terminal::readEvent` | input thread only |
| `Terminal::write`/`flush`/`size` | marquee worker only |
| frame `std::string`, `Renderer` | worker only |
| shutdown flag | signal handler writes, input thread reads-and-clears |
| `Interpreter::quit_` | input thread only; worker never reads it |

- **[CODE]** There is exactly **one** mutex and **one** cv. No atomics-per-field, no second mutex, no
  `detach()` anywhere. `~Scheduler` does `requestStop()` + `join()`.

## Slide 5.2 — The worker loop and the three lock rules

**[CODE]** `Scheduler::run`:

```cpp
for (;;) {
  {
    std::unique_lock<std::mutex> lk(mu_);
    if (stop_) return 0;
    const long long seen = wakeTick_;
    cv_.wait_for(lk, std::chrono::milliseconds(pollTimeoutMsLocked()),
                 [this, seen] { return stop_ || dirty_ || wakeTick_ != seen; });
    if (stop_) return 0;          // release mu_ BEFORE stepping
  }
  (void)tick(nullptr);
}
```

**The three lock rules (from `scheduler.cpp` and `docs/threading-model.md` §3):**

1. **`tick()` is never called while `mu_` is held.** The `unique_lock` is scoped so it is released before
   the step.
2. **Every blocking call happens outside `mu_`.** The frame is assembled into one string under the lock and
   written **after** the lock is dropped.
3. **`Terminal::size()` is read before the lock.** It is a syscall and belongs to the worker alone; there is
   no second mutex, hence no lock order to get wrong.

- **[CODE]** `tick()` never reads input, never sleeps on a timer, and never ends the loop. `stop_` is the
  worker's **only** exit signal; the worker never asks the interpreter whether the user left.

## Slide 5.3 — The step function `tick()`

**[CODE]** `Scheduler::tick(const KeyEvent*)`:

```
sz = term_.size()                          // rule 3, before lock
lock mu_:
   now = term_.nowMs()                     // single clock source
   if ev: feed(*ev), redraw = true         // test-only overload; production run() passes nullptr
   if dirty_: dirty_ = false, redraw = true
   running = (proc_.state == Running)
   due     = running && (!hasRendered || now - lastRenderMs >= refreshMs)
   if due || redraw:
       frame = renderer_.buildFrame(...)
       if due:
           lastRenderMs = now; hasRendered = true; cycles += 1; rendered = true
unlock
if frame: term_.write(frame); term_.flush()  // outside mu_
```

- **[CODE]** `postEvent()` is the production delivery path: it locks, calls `interp_.feed(ev)`, sets
  `dirty_ = true`, unlocks, then `cv_.notify_all()`. `run()` always steps with `nullptr`, so posting and
  stepping are decoupled.
- **[INFERRED]** Consequence: command-processing echo is responsive even at `refreshMs = 10000`, because the
  echo frame is driven by `dirty_`, not by the render deadline.
- **[CODE]** `TickResult` carries only `{ bool rendered }`; the v2 `quit` field was removed so the worker
  never reads `quit_`.

## Slide 5.4 — `pollTimeoutMs`: the only timer in either thread

**[CODE]** `pollTimeoutMsLocked()`:

```
ms = pollingMs
if state == Running:
    until = refreshMs - (nowMs() - lastRenderMs)
    if 0 < until < ms: ms = until
clamp ms to [1, 1000]        // 0 would busy-spin a core
```

- **Why the minimum:** the worker wakes at the sooner of the render deadline and `pollingMs`, so `pollingMs`
  can **never delay a frame**.
- **[CODE]** `pollTimeoutMs()` public accessor and `snapshot()` take `mu_`; `snapshot()` is the only
  sanctioned cross-thread read of live scheduler state (`SchedulerSnapshot`).

## Slide 5.5 — Start, stop, shutdown (no detached thread)

**[CODE]** `console_app.cpp` + `scheduler.cpp` + `shutdown.hpp`:

```
intent (quit_ / signal flag)
  -> observe   (input thread; only reader of both)
  -> Scheduler::requestStop()   { lock; stop_ = true; } cv_.notify_all()
  -> Scheduler::join()          (worker returns from run())
  -> TerminalGuard::~TerminalGuard() restore()
```

- **[CODE]** `start()` spawns exactly one worker and is idempotent via `worker_.joinable()`. The spawn is
  exception-safe: if `std::thread`'s constructor throws, the scheduler stays unstarted.
- **[CODE]** The join precedes `restore()` because `restore()` must be the last writer to the terminal —
  otherwise the worker could write into a restored terminal. The goodbye line is written after `join()`,
  when the program is single-threaded again.
- **[CODE]** `ConsoleApp::run` re-checks `interp.quitRequested() || shutdownRequested()` after **every**
  `readEvent` (including the timeout/EINTR path), or `exit` would leave the worker animating.
- **Signal handling:** POSIX `sigaction(SIGINT/SIGTERM)` sets a `volatile std::sig_atomic_t` flag only (no
  allocation, no I/O, no restore, no notify); `sa_flags = 0` (no `SA_RESTART`) so `poll()` returns `EINTR`.
  Win32 `SetConsoleCtrlHandler` does the same on its own thread.

## Slide 5.6 — Concurrency evidence (not just green tests)

**[TEST]/[OBSERVED]** The project distinguishes race-freedom from real-time overlap:

- **Deterministic suite** (`tests/unit/test_scheduler.cpp`): barrier-synchronized on
  `FakeTerminal::size()` call counts (`waitForTickStarted`), **no `sleep_for`/`sleep_until` anywhere**. The
  task step `grep -n 'sleep_' tests/unit/test_scheduler.cpp` prints nothing.
- **Real-overlap test:** `threaded_input_thread_and_animation_overlap_in_real_time` — 100 further ticks
  start while the input thread is still posting.
- **Acceptance A8 (frozen/real console):** 29 distinct band positions over 29 one-key-at-a-time keystrokes at
  `--refresh-ms=40`, and 13/13 at `set_speed 1` (`docs/t5.1-windows-acceptance.md`;
  `docs/frozen-artifact.md` T3.4: 19 band positions in 2.0 s).
- **Invariant tests:** only the marquee thread ever writes; every `write()` carries exactly one whole frame
  (cursor-home prefix, exactly `rows` rows each exactly `cols` wide, `ESC[K`-closed last row).

---

# Section 6 — Refresh Rate vs. Polling Rate Balance

## Slide 6.1 — The two quantities are different jobs

**[CODE]** From `parameters.hpp`, `scheduler.cpp`, `docs/threading-model.md` §4:

| | `refreshMs` | `pollingMs` |
| --- | --- | --- |
| Meaning | marquee render **deadline** | **maximum idle wait** in the worker's `cv_.wait_for` |
| Default | `100` | `10` |
| Range | `[1, 10000]` | `[1, 1000]` |
| Runtime command | `set_speed <ms>` | none (flag only) |
| CLI flag | `--refresh-ms=N` | `--poll-ms=N` |
| What it controls | how often the band advances | idle **wakeups per second** |
| Does it delay keys? | **No** | **No** |

- **[CODE]** The worker's wait is `min(pollingMs, msUntilNextRender)`, so polling can never delay a frame.
- **[CODE]** `readEvent` returns the instant a key arrives, so `pollingMs` adds no keystroke latency.
- **[CODE]** `pollingMs` is the **only timer in either thread**. Everything else is event-driven.
- **[CODE]** There are **two idle waiters** (the input thread in `readEvent`, the worker in `cv_.wait_for`),
  so at the same `pollingMs` the idle-wakeup count is roughly doubled. This is the price of the
  professor-mandated two-thread design (risk 24).

## Slide 6.2 — How the balance is struck in code

**[CODE]** `Scheduler::tick`:

- **Fluid animation** comes from the `due` deadline: `now - lastRenderMs >= refreshMs`.
- **Responsive typing** comes from `dirty_`: every `postEvent` sets it and notifies, so a keystroke forces a
  frame immediately regardless of `refreshMs`.
- **Cheap idling** comes from `pollTimeoutMsLocked()` clamping to `[1, 1000]`: the worker never spins, and at
  `pollingMs = 1000` it wakes about once a second.

**[INFERRED]** Therefore the two knobs are almost independent in the common case: choose `refreshMs` for
visual fluidity, choose `pollingMs` for idle CPU / worst-case shutdown latency.

## Slide 6.3 — Measured interaction (Windows)

**[OBSERVED]** `docs/measurements/windows-refresh.md` (frozen binary, `--poll-ms=10`):

| `refreshMs` | observed band changes/s | fluid? |
| --- | --- | --- |
| 1 | ~64 | fastest the console sustains; console-write bound |
| 16 | ~40 | yes |
| 50 | ~16 | yes |
| 100 | ~9 | yes, slight stepping (shipped default) |
| 250 | ~4 | choppy, clearly stepped |
| 1000 | ~1 | one step per second |
| 10000 | 0.08 | motion effectively frozen |

**[OBSERVED]** `docs/measurements/windows-polling.md` (`--refresh-ms=100`):

| `pollingMs` | idle CPU | effective wakeups/s | Ctrl+C latency | echo latency |
| --- | --- | --- | --- | --- |
| 1 | 0.31 % | ~64 (coalesced by 15.6 ms quantum) | ~3.3 ms | ~1.9 ms |
| 5 | 0.42 % | ~64 | ~4.3 ms | ~1.6 ms |
| 10 | 0.36 % | ~64 | ~3.4 ms | ~1.8 ms |
| 50 | 0.05 % | ~20 | ~3.2 ms | ~1.3 ms |
| 1000 | ~0.00 % | ~1 | ~3.2 ms | ~2.0 ms |

- **[OBSERVED]** Sub-quantum `pollingMs` values (1, 5, 10) are **indistinguishable** on Windows because the
  OS timer quantum (~15.625 ms) coalesces them; the input thread is blocked in `ReadConsoleInput` and
  consumes no measurable CPU.
- **[OBSERVED]** Echo latency is ~1–2 ms at **every** value, including `refreshMs = 10000` — confirming the
  `dirty_`/cv mechanism.
- **[INFERRED]** `pollingMs` therefore trades idle CPU against shutdown-wakeup latency, not against typing
  responsiveness.

---

# Section 7 — Hardware Recommendations

## Slide 7.1 — What "current hardware" is (and what the record does not contain)

**[OBSERVED]** The repository records the machine class used for the frozen sweep:

| Item | Value | Source |
| --- | --- | --- |
| OS | **Windows 11** `[Version 10.0.26200.8875]`, x86_64 | `docs/measurements/windows-*.md` |
| Console | real Win32 console created with `CREATE_NEW_CONSOLE`; graded config uses the CLion **external console** | `docs/frozen-artifact.md` T3.4 |
| Frozen toolchain | GCC 14.2.0 (MinGW-W64 UCRT, Brecht Sanders r2), `-O2` | `docs/frozen-artifact.md` |
| Build tools | CMake 4.4.3, Ninja 1.12.1 | `docs/frozen-artifact.md` |
| IDE | CLion 2026.2.3 (`CL-262.10968.117`) | `docs/clion-run-config.md` |
| Frozen artifact | `frozen/csopesy.exe`, SHA-256 `ec86bb06…03e90b0`, 187 519 bytes | `docs/frozen-artifact.md` |

**Honest gap:** the repository does **not** record CPU model, RAM, GPU, or display refresh rate
(`grep -i 'CPU|processor|RAM|GHz'` finds no machine spec). So the recommendations below are tied to the
recorded machine class (**Windows 11 x86_64, real Win32 console**), not to a named CPU. Do not invent a CPU
spec on the slide; state "the recorded Windows 11 test machine".

## Slide 7.2 — Recommended values (Windows, the graded platform)

**[RECOMMENDED]** From `docs/measurements/windows-refresh.md` and `windows-polling.md`:

| Parameter | Recommended | Why | Alternatives |
| --- | --- | --- | --- |
| `refreshMs` | **16 ms** | band is fluid and no longer console-write bound; ~40 band changes/s | `50` or `100` if fewer redraws wanted |
| `pollingMs` | **10 ms (shipped default)** | idle CPU stays under 1 %; Ctrl+C and echo are immediate | `50` (~20 wakeups/s) or `1000` (~1 wakeup/s) cut wakeups with no keystroke-latency cost |

- **[OBSERVED]** `refreshMs = 16` is the knee of the curve: below it the app is limited by console write
  throughput (~64 changes/s measured at `refresh = 1`), not by `refreshMs`; above it the motion visibly steps.
- **[OBSERVED]** `pollingMs = 10` is a good default across machines because sub-quantum values coalesce,
  so its idle CPU equals `1` and `5` while still keeping the worst-case wakeup tight.
- **[RECOMMENDED]** For the **graded run** the run configuration passes **no arguments**; these are the
  values to use for measurement takes (`--refresh-ms=16 --poll-ms=10`) and as the runtime `set_speed`
  baseline.

## Slide 7.3 — What is *not* yet a recommendation (guard against overclaiming)

- **[OBSERVED]** `docs/measurements/linux-{refresh,polling}.md` and `macos-{refresh,polling}.md` are all
  **PENDING (T6.3)**. There is no POSIX frozen artifact yet, so **no Linux/macOS recommended value exists**.
  Do not present one.
- **[OBSERVED]** The default is `refreshMs = 100`, but the swept recommendation is `16`; the two coexist:
  `100` is the built-in default, `16` is the observed fluid sweet spot on the test machine.
- **[INFERRED]** Because the renderer rebuilds the whole frame every frame, the cost of a low `refreshMs`
  scales with terminal size (`rows × cols` bytes per write); a very large terminal may hit the console-write
  ceiling sooner. This was not swept at multiple terminal sizes; state it as a limit, not a measured curve.

---

# Section 8 — Performance Limits

> **Wording that binds this section (D4 / §3.9):** these are **observed thresholds and recommendations**,
> not per-keystroke latency figures. No in-process telemetry exists in v3.

## Slide 8.1 — Screen tearing

**[CODE] Why tearing is structurally hard here:**
`Renderer::buildFrame` produces one complete frame string and the single marquee thread issues exactly one
`Terminal::write()` per frame. There is one writer thread, so two frames cannot even be *built*
concurrently; there is no partial-frame update. The invariant is pinned by
`every_write_carries_exactly_one_whole_frame`.

**[OBSERVED]** On the Windows frozen sweep, **tearing/flicker was never observed at any `refreshMs` from 1
to 10000**; `frame_consistent = true` at every take. If tearing were seen, the likely sources are (a) the
terminal emulator's own repaint, or (b) a non-VT console — not the application, which emits no partial
frames.

**[CODE] Hard limit to state:** one frame per `write()` prevents *application-level* interleaving of two
frames; it does **not** make the terminal's own repaint atomic, and no escape sequence is added to chase
that (`docs/threading-model.md` §7.2). Do not claim "threads fix tearing".

## Slide 8.2 — Fluidity threshold (motion stops looking smooth)

**[OBSERVED]** Windows frozen sweep:

| Threshold | Observation |
| --- | --- |
| `≤ ~100 ms` (≥ ~9 band changes/s) | still reads as fluid; slight stepping at `100` |
| `> ~100 ms` | motion stops looking fluid |
| `250 ms` | choppy, clearly stepped (~4 changes/s) |
| `1000 ms` | one step per second |
| `10000 ms` | effectively frozen (~1 change per 12 s) |

**[INFERRED]** The practical ceiling on this machine class is ~64 band changes/s at `refreshMs = 1`, i.e.
console-write bound, not timer bound. A slide-safe statement: *"below ~16 ms the console write is the
bottleneck; above ~100 ms the eye sees stepping."*

## Slide 8.3 — Input latency / typing delay

**[OBSERVED]** Windows frozen sweep (echo = time from injected printable key to the prompt row showing it):

- Keystroke echo was **~1.0–2.0 ms at every `refreshMs`** (1 → 10000) and **every `pollingMs`** (1 → 1000).
- Control-input latency was ~3.2–4.3 ms across `pollingMs` values.

**[CODE] Why echo does not track `refreshMs`:** `Scheduler::postEvent` sets `dirty_` and notifies `cv_` under
`mu_`; the worker's wait predicate returns immediately and `tick` builds an echo frame outside the deadline
condition. The unit test `posted_keystroke_echo_is_written_without_waiting_for_the_refresh_deadline` sets
`refreshMs = 10000` and still observes a written frame while `cycles` stays put. `docs/threading-model.md` §6.

**[OBSERVED]** Acceptance A8 is the real-overlap evidence: at `--refresh-ms=40`, 29 one-key-at-a-time
keystrokes produced 29 distinct band positions and the command echoed; at `set_speed 1`, 13/13. The T3.4
live gate observed 19 band positions in 2.0 s with 20 keystrokes echoed in ~5 ms.

**Practical typing-delay limits to present:**
- **[CODE] Worst case is not the console write:** rule 2 removes the unbounded `write()` from the input
  thread's path. The input thread can still **briefly wait behind one frame's render** while the worker holds
  `mu_`; the guarantee is *"the unbounded console write can never stall the input thread"*, not *"the input
  thread never waits"* (`docs/threading-model.md` §7.3, risk 26).
- **[OBSERVED] Worst realistic input stress:** a 220-character `set_text` / a fully-buffered command is
  accepted without crash; the confirmation line is width-clipped to `cols` (documented, not a defect,
  `docs/t5.1-windows-acceptance.md` §6).
- **[OBSERVED] Shutdown latency:** on Windows the `SetConsoleCtrlHandler` runs on its own thread and cannot
  interrupt the input thread's blocked `WaitForSingleObject`; the flag can wait up to `pollingMs`
  (`terminal_win32.cpp` comment; `docs/threading-model.md`). Measured Ctrl+C latency stayed ~3.2–4.3 ms even
  at `pollingMs = 1000`, but treat `pollingMs` as the bound (risk: at 1000 ms a signal could in principle be
  read up to ~1 s late if no input record also wakes the wait). **[INFERRED]** This is a reason to keep
  `pollingMs` around `10–50` for the graded run rather than `1000`.

## Slide 8.4 — Idle cost and CPU / wakeup limits

**[OBSERVED]** Windows frozen sweep:

- Idle CPU (marquee never started): **0.31 % / 0.42 % / 0.36 % / 0.05 % / ~0.00 %** at `pollingMs = 1 / 5 / 10
  / 50 / 1000`.
- Sub-quantum `pollingMs` (1, 5, 10) all wake at ~64/s because the ~15.625 ms Windows timer quantum
  coalesces them; the difference only appears above the quantum (50 → ~20/s, 1000 → ~1/s).
- The **input thread consumes 0 %** (blocked in `ReadConsoleInput`); the idle cost is the worker's timed wait.
- `GetProcessTimes` has a 15.625 ms quantum, so values below ~0.05 % are at the measurement floor.

**[CODE] Why zero is impossible and undesirable:** `pollTimeoutMsLocked()` clamps `ms` to a minimum of `1`
("0 would busy-spin a core"). Both threads wait on bounded timed primitives; there is no spin, no `sleep(0)`,
no `_kbhit` polling loop (risk 7, risk 18).

## Slide 8.5 — Limits observed at the extremes (acceptance A10/A11)

**[OBSERVED]** `docs/t5.1-windows-acceptance.md`:

| Setting | Result |
| --- | --- |
| `--refresh-ms=1 --poll-ms=1` | no input loss, no crash, responsive; ~45 band samples/s |
| `--refresh-ms=10000` | 1 sample (motion frozen); typing still responsive |
| `--poll-ms=1000` / `--poll-ms=1` | 10 samples each; no input loss |
| Terminal `20×5` mid-animation | band row and prompt row both survive; no row exceeds width; layout recovers on resize back |

**Cross-reference — the A4 non-ASCII tension (do not hide it):**
`set_text` non-ASCII is rejected by contract (§3.7 rule 7 / D15) and is reachable on `--no-tty` raw stdin
(verified: `printf 'set_text caf\xc3\xa9\n...'` prints the rejection line). It is **unreachable from the
interactive keyboard by design** because the line editor appends printable ASCII only, so typing `café`
yields `caf`. The record flags this as an acceptance/spec inconsistency, **not patched**
(`docs/t5.1-windows-acceptance.md` §6.1).

---

# Appendix — Evidence index

| Claim area | Source of truth |
| --- | --- |
| Command table + responses | `src/features/commands/interpreter.cpp`, `tests/unit/test_interpreter.cpp` |
| Line editing + echo | `src/features/commands/line_editor.cpp`, `Interpreter::feed` |
| ASCII / clamp rules | `src/entities/parameters.cpp`, `include/csopesy/parameters.hpp` |
| CLI flags | `src/entities/cli.cpp`, `include/csopesy/cli.hpp` |
| Frame layout / band / tight terminals | `src/features/marquee/renderer.cpp`, `tests/unit/test_renderer.cpp`, `tests/unit/test_scroll.cpp` |
| PCB | `include/csopesy/process.hpp`, `src/entities/process.cpp`, `tests/unit/test_process.cpp` |
| Scheduler / locking | `src/app/scheduler.cpp`, `include/csopesy/scheduler.hpp`, `tests/unit/test_scheduler.cpp`, `tests/support/fake_terminal.hpp` |
| Threading rationale + honest limits | `docs/threading-model.md` |
| Shutdown | `src/app/console_app.cpp`, `src/platform/terminal_posix.cpp`, `src/platform/terminal_win32.cpp` |
| Platform backends | `src/platform/terminal_posix.cpp`, `src/platform/terminal_win32.cpp` |
| Layer rules | `scripts/check_layers.sh`, plan §3.1 |
| Acceptance A1–A12 | `docs/t5.1-windows-acceptance.md`, plan §6.2 |
| Cross-OS `ctest` | `docs/t5.2-cross-os-ctest.md` |
| Frozen artifact + T3.4 gate | `docs/frozen-artifact.md`, `docs/clion-run-config.md` |
| Measurement sweep (Windows, COMPLETE) | `docs/measurements/windows-refresh.md`, `docs/measurements/windows-polling.md` |
| Measurement sweep (Linux/macOS, PENDING) | `docs/measurements/linux-*.md`, `docs/measurements/macos-*.md`, `docs/measurements/README.md` |
| Design decisions / rationale | `docs/IMPLEMENTATION_PLAN_v3.md` §1.2, §3.5–§3.11, §6, §8–§9; `docs/PLAN_V3_PROGRESS.md` |

## Closing statement (suggested final slide)

> The program runs the professor-mandated **two threads** with **one mutex, one condition variable and one
> terminal writer**. Commands are recognized case-sensitively against a six-entry table; the renderer builds
> one whole frame per `write()` so no two frames can interleave at the application level; the PCB is a
> minimal `MarqueeProcess`; the scheduler controls fluidity with `refreshMs` (a deadline) and idle cost with
> `pollingMs` (a bounded wait). On the recorded Windows 11 test machine the recommended values are
> **`refreshMs = 16`, `pollingMs = 10`**; tearing was **never observed**, motion stops looking fluid above
> ~100 ms, and keystroke echo stayed at ~1–2 ms regardless of either value. Those are **observed thresholds,
> not per-keystroke latency measurements**, and the Linux/macOS baselines are still pending a POSIX frozen
> artifact.
