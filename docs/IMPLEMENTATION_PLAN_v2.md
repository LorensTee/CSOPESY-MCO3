# CSOPESY Marquee Operator — Implementation & Workload Plan (v2)

> **v2.1 (2026-09-16).** Replaces `IMPLEMENTATION_PLAN.md` (v1). v1 remains in the repo only as the reviewed
> artifact; **do not implement from v1**. Every change is listed in `REVIEW_ADJUDICATION.md` with its
> evidence.
>
> **Scope is smaller — the document is not.** v2 removes a second scheduler mode, the CSV-telemetry and
> pty-replay subsystems, three dev commands (`ps`/`params`/`stats`), four knobs (`step_cols`, `scheduler`,
> `dev_commands`, `stats_path`: 11 config keys → 7), two unused process states (`Ready`/`Finished`), and three
> library translation units (`ansi.cpp`, `responses.cpp`, and `fake_terminal.cpp`, which moved to
> `tests/support/`). It is nevertheless a *longer* file than v1, because the contracts and every test are now
> spelled out in full. It contains no contradictions: the input-ownership, whitespace, tearing-wording,
> signal-restore, runbook-vs-video and `T4.6` defects of v1 are gone, the seven defects the review missed are
> fixed, and the glyph/scroll/scheduler assertions written out below were executed against the reference
> algorithm in simulation before this file was committed. **Simulation is not verification** — it shows the
> specified algorithm is self-consistent, not that C++ compiles or that a terminal behaves (§6.3).
>
> **v2.1 (same day)** applies the second review round (`REVIEW_ADJUDICATION.md` §6): the quiz run
> configuration is split from the dev one and can no longer rebuild at Run time; `polling_ms` is documented as
> a **maximum idle wait** rather than a typing-delay knob; the measurement metric is renamed honestly; the
> input line scrolls horizontally instead of wrapping; "not observed" is an allowed measurement result; and
> six further defects found in v2's own text are fixed — Windows resize events were dropped, POSIX
> `EINTR` contradicted `SIGWINCH`, `FrameBuffer::resize` was never called from the loop, `set_speed -5` was
> ambiguous, `--no-tty` had no defined behaviour, and `enterRawMode()` had no stated call site.
>
> **v2.2 (same day)** applies the third review round (`REVIEW_ADJUDICATION.md` §7): the CMake floor is
> corrected to **3.21** (the presets use schema v3 and the `Visual Studio 17 2022` generator, both added in
> 3.21); `Scheduler` moves from `entities/` to `app/`, because it holds a `Renderer&` and an `Interpreter&`
> (FSD rules 4-1 and 4-3: no upward imports, no cross-imports between `features/` slices); the resize path is
> simplified to live `Terminal::size()` detection each tick — resize events are neither delivered to `_getch`
> nor needed — and a size change now *forces* the repaint it previously only prepared; the CI smoke test becomes
> a portable CTest job instead of a `printf |` shell pipe; and `scripts/check_layers.sh` turns §3.1's
> "enforced" layer rule into an actual check.
>
> **v2.3 (same day)** applies the fourth — and final paper — review round (`REVIEW_ADJUDICATION.md` §8): a
> **build-blocking** CMake bug is fixed (the combined `add_compile_options($<…>/W4$<…>-Wall -Wextra>)` one-liner
> is split by CMake's argument parser at the space inside the generator expression, so `-Wextra>` reaches the
> compiler as a literal flag and the build dies — reproduced on CMake 4.3 + GCC 16 — and it is replaced by a
> plain `if(MSVC)`/`else()`); the **quiz-case → `config/csopesy.ini` procedure is made explicit**, because the
> frozen Run always loads that one literal path and nothing previously said how `config/quiz_case_<n>.ini`
> becomes it; the definition of done's "failing test first" rule gets a stated carrier carve-out for the
> platform-surface tasks (T1.1/T3.1/T0.5), whose real test is the hand-run checklist; and two stale labels are
> corrected (T1.1's resize history is trimmed to the coding instruction, and the §10 navigation row).
>
> **v2.4 (2026-09-16).** Applies the architecture change requested by `threading.md`: the scheduler is no
> longer a single-threaded cooperative loop. It now runs **two** threads — an input/command thread (the `main`
> thread) and a marquee/scheduler worker — coordinated by one `std::mutex` and one `std::condition_variable`,
> with the marquee thread as the **only** terminal writer. Nothing else moves: the FSD layout, every frozen
> contract in §3, all six commands, config → CLI → runtime precedence, `--no-tty`/`--diag`/`--measure`, the
> quiz/frozen-binary workflow and the layer guard are untouched. §3.8 is rewritten, §3.3/§3.5/§3.6/§3.9/§3.10
> gain a concurrency clause, T1.3 gains the worker and its tests, and T0.2 gains `Threads::Threads`. The exact
> list of sections that went stale is in §10.10. The pure `Scheduler::tick()` contract is deliberately
> **preserved**, so every existing deterministic scheduler test stays green: this change *added* tests, it did
> not invalidate them.

> **v2.5 (2026-09-16).** Applies the **two threading reviews** (`gpt-v5.md`, `gpt-v6.md`) — rounds 5 and 6 of
> `REVIEW_ADJUDICATION.md`. **The architecture is unchanged**: still two threads, one mutex, one condition
> variable, one terminal writer. What changes is the *contract's honesty* and the *critical section's extent*:
>
> 1. **`--measure` is now specified rather than described.** One `eventMs,echoMs` pair per **rendered echo
>    frame**, carrying the **oldest** unconsumed keystroke (queued keys coalesce onto one frame and one pair),
>    emitted **once**. v2.4's sketch both overwrote the stamp — understating worst-case latency — and never
>    cleared it, so a stale pair would have repeated on every later frame. The frame line belongs to a refresh
>    (`due`) frame only, so the interval series stays the refresh cadence.
> 2. **The critical section contains no syscall.** `Terminal::size()` is read *before* `mu_` (rule 3) and the
>    `--measure` append happens *after* it, so the only wait another thread can meet is one frame's render —
>    stated plainly in §3.8 instead of the v2.4 claim that typing simply stays responsive.
> 3. **`started_` is deleted** in favour of the input-thread-only `worker_.joinable()`, which makes `start()`
>    exception-safe by construction (v2.4 set `started_ = true` before spawning).
> 4. **The plan no longer implies its deterministic suite proves concurrency.** A real two-thread overlap test
>    and the named A8 transcript are the evidence; §6.1/§6.3 say so.
>
> Review points that were **rejected with reasons** — deleting `wakeTick_`, trimming the concurrency matrix,
> treating `SchedulerSnapshot` as dispensable, and the FSD/CI scope pressure — are recorded in §10.11 rather
> than silently dropped. The status claim itself is corrected: "frozen for paper review" never meant
> "implementation-verified" (see §10.10's closing status).

> **v2.6 (2026-09-16).** Applies the **seventh review run** (`gpt-v7.md`, `REVIEW_ADJUDICATION.md` §11) — the
> first review that read `REVIEW_ADJUDICATION.md` itself. One point, **accepted in direction but not as filed**:
> the worker no longer reads `Interpreter::quit_`. v7 filed the cross-thread `quitRequested()` access as a data
> race; it was **not** one — the sole writer (the input thread, via `postEvent`→`feed`) always held `mu_`, the
> worker's read held `mu_`, and a writer reading its own field needs no lock, so the writer→worker edge was
> already ordered. The *fix* is still taken, because it removes the last cross-thread read the plan had to hedge
> about: `quit_` is now input-thread-only, production `run()` exits solely on `stop_`, and `TickResult::quit` is
> deleted (nothing read it — every preserved test discards the return value). No other architecture change: v7
> explicitly endorsed keeping `wakeTick_`, `SchedulerSnapshot`, the `--measure` mechanism, the CI matrix and the
> FSD layout, so nothing from §10.11 was reverted.
>
> **That closes paper review.** The next change to this file is an *evidence* change (§10.12 status): a green
> `ctest`, a three-image compile, a terminal transcript or a failing acceptance case.

**For agentic workers:** implement task-by-task. Each task lists its files, its failing test, the real code,
the exact command and the expected result. Steps are `- [ ]`. Tasks inside a workstream are ordered;
workstreams run in parallel only after Phase 0 contracts are frozen.

**Goal:** A cross-platform C++17 "OS emulator" console that accepts `help`, `start_marquee`, `stop_marquee`,
`set_text`, `set_speed`, `exit` and keeps a scrolling ASCII-art marquee animating while the prompt stays
typeable, with every quiz-relevant knob changeable **without recompiling**.

**Architecture:** two threads, one mutex, one condition variable. The **input/command thread** (the `main`
thread) owns `Terminal::readEvent` and the `Interpreter`; the **marquee/scheduler thread** owns timing, the
`FrameBuffer`, the `Renderer` calls and **every** `Terminal::write()`/`flush()`/`size()`. State crosses between them
only under `Scheduler::mu_`, and a wakeup crosses as a `cv_.notify_*()` on a condition variable — never as a
`sleep_for` used as the synchronization mechanism. A thin `Terminal` interface still hides `termios`/`poll`
(Linux+macOS) and the Win32 console API (Windows). An injected monotonic clock still drives a deterministic
`Scheduler` step. Rendering still goes through a diffing `FrameBuffer` that emits **one assembled frame per
`write()`**, so the application never interleaves two frames. Knobs live in one `Parameters` struct fed by
config file → CLI args → runtime commands.

**Tech stack:** C++17, including `std::thread`/`std::mutex`/`std::condition_variable` (standard-library
threading, linked portably through CMake's `Threads::Threads`), CMake ≥ 3.21 + presets, CTest, GitHub Actions
(ubuntu/windows/macos), no third-party runtime dependencies, POSIX `termios`/`poll`, Win32 console API,
ANSI/VT escapes.

**Spec source:** `MO3 - Marquee_Operator.pdf` ("CSOPESY Semi-Major Output 1", updated 2026-09-04).

**Deadline:** **Wednesday 2026-09-23** — 7 days. Day-by-day gates and the cut line are in §5.0.
*(Provenance: the handout itself only says "Week 3 — Project submission / See AnimoSpace for specific dates";
this exact date comes from the group's AnimoSpace reading. If AnimoSpace disagrees, §5.0's gates and the cut
line still stand — the dates shift, the order does not.)*

**Team:** W1 Lorens (Linux) · W2 Byron (Linux+Windows) · W3 Nathan (Windows) · W4 Kim (macOS+Windows).
The split is justified by OS capability in §4; all four use CLion (§2.4).

---

## 0. How to read this document

| Section | Read it if you are… |
| --- | --- |
| §1 | anyone — what is actually graded, verified against the handout |
| §2 | anyone — how the four OS setups change the work |
| §3 | W2 first, everyone else before Phase 1 — **frozen contracts** |
| §4 | anyone — the workload split and *why* each part went to a person |
| §5 | the person owning that task |
| §6–§7 | anyone — definition of done and quiz-day runbook |
| §8–§9 | the group lead |
| §10 | anyone auditing the plan revision history (v1 → v2 → v2.5 → v2.6) |

Workstream shorthand: **W1 = Lorens**, **W2 = Byron**, **W3 = Nathan**, **W4 = Kim**. Task IDs (`T0.x`…`T6.x`)
and the RACI table (§4.5) are keyed to it.

---

## 1. Spec decode — what is actually graded

### 1.1 Functional requirements (traceability)

| # | Requirement (verbatim intent) | Where it is satisfied |
| --- | --- | --- |
| R1 | "OS emulator" that accepts command input with display output | `src/app/console_app.cpp` + `src/features/commands/interpreter.cpp` |
| R2 | Main menu console: welcome line, group developer names, version date, prompt | `src/features/marquee/renderer.cpp` header region (§3.9) |
| R3 | Text marquee **or** ASCII graphics marquee | `src/features/marquee/glyphs.cpp` (§3.9), default text `CSOPESY` |
| R4 | Commands that change the marquee's behavior | `set_text`, `set_speed`, `start_marquee`, `stop_marquee` |
| R5 | `help` — displays the commands and their descriptions | §3.7 response table |
| R6 | `start_marquee` / `stop_marquee` | `MarqueeProcess::start()/stop()` |
| R7 | `set_text` — accepts text input, displays it as a marquee | `Parameters::text` → re-rasterize glyph rows |
| R8 | `set_speed` — marquee refresh **in milliseconds** | `Parameters::refreshMs` (clamped) |
| R9 | `exit` — terminates the console | must also restore the terminal (raw mode off, cursor shown), exit 0 |

### 1.2 The assessment method, decoded (quoted, not inferred)

The handout: *"The project will be assessed through a black box quiz system in a time-pressure format. This
is to minimize drastic changes or 'hacking' your project to ensure the test cases are met. You should only
modify the parameters and no longer recompile the project when taking the quiz."*

1. **The graded artifact is a frozen build.** Building at quiz time is a fail. So the release binary is built
   once, copied into a submission package (`frozen/`, **never committed to git**), and the run configuration
   points at it. Source edits stop at the `quiz-frozen` tag (§T6.3).
2. **The grading table gives three outcomes, quoted verbatim:**

   | No points | Partial points | Full points |
   | --- | --- | --- |
   | "The CLI did not pass the test case. **NO WORKAROUND is available** to produce the expected output." | "The CLI did not pass the test case. **A workaround is available** to produce the expected output." | "The CLI passed the test case using **varying inputs** and produced the expected output." |

   Consequence, stated as an engineering precaution rather than an invented rule: *major tunables should be
   reachable through more than one interface* (`text`, `refresh_ms`, `ascii_art` via config file **and** CLI
   flag **and** their runtime command), so a failed command interaction can still be worked around on the
   frozen binary. This is the partial-credit hedge — it is **not** a claim that every knob needs three paths.
   Knobs with no command (`polling_ms`, `marquee_row`, `developers`, `version_date`) get config + CLI only,
   and none of them is graded by a quiz case.
3. **"Varying inputs" means robustness is graded.** Empty input, whitespace, very long text, punctuation,
   mixed case, both `set_speed` extremes, repeated start/stop, `set_text` while stopped, unknown commands,
   resize mid-animation, `exit` while animating — §6.2.
4. **Time pressure ⇒ the runbook is a script** (§7), not improvisation.

### 1.3 Submission deliverables (hard constraints)

| Deliverable | Constraint from the handout | Owner |
| --- | --- | --- |
| **SOURCE** | source + **README.txt** with member name(s), run instructions, and **the entry file where `main` is**; or a GitHub link | W2 |
| **PPT** | technical report covering *command recognition*, *console UI implementation*, *command interpreter implementation*, *process representation*, *scheduler implementation*, and *the balance between a fluid refresh rate and a polling rate* — recommended values **for your current hardware** plus the limits where **screen tearing** and/or **noticeable typing delay** appear | §4.5 RACI |
| **VIDEO** | seamless/uncut; 480p min, 720p max; ≤ 1 GB; readable IDE font; **must always show pressing Run/Debug from the IDE** which initializes the program; no code access/modification once running; MP4 **embedded inside the PPTX** and reflected in the total size | W3 runs, W4 directs |

> "Entry class file" is Java-flavoured wording; the professor confirmed C++. README.txt answers it literally:
> entry file **`src/main.cpp`**, function `main`, class `csopesy::ConsoleApp` in `src/app/console_app.cpp`.

### 1.4 The handout's own mock console (transcribed from a 600-dpi render)

Neither v1 nor the review had this. It is the reason for three contract details in §3:

```text
Welcome to CSOPESY!          <- capital W                (§3.9 header, M2)

Group developer:
De La Cruz, Juan
Santos, Alex

Version date:                <- blank in the mock        (§3.6 default)

Command>|                    <- cursor on the prompt row
```

and the marquee state, whose art band sits **above** the `Group developer:` block:

```text
/ ___/ ___/  _ \/  _ \/ ___/ ___/ \ / /
| |   \__ \ | | | | | |\__ \\__ \  | | |
| |__ ___/ | |_| | |_| |___/ ___/ | | |
\____/____/ \____/\____/(___/(___/  |_|_|
```

Two consequences:

1. **The mock's art is a slanted outline font** (`/ \ _ |`), not filled `###` block letters. The handout asks
   for *"Text marquee **or** ASCII graphics marquee"*, so the style is not graded. v2 keeps a **fixed-width
   glyph cell** because uniform cell width is what makes the scroll math safe, and adds a width-invariant
   test for all ~45 hand-typed glyphs (§T4.1). Style may be changed later; ragged cells may not.
2. **The two mocks disagree about where the band sits** (empty between `Welcome` and `Group developer:` in
   one, the art there in the other). Hence `marquee_row` stays as a config-only knob — one `int`, cheap
   insurance against a layout surprise during a graded recording.

---

## 2. Your four machines — what the OS spread actually changes

### 2.1 Coverage matrix

| Member | Linux | Windows | macOS | Can build & run for | Cannot test at all |
| --- | --- | --- | --- | --- | --- |
| W1 — Lorens | ✅ | ❌ | ❌ | Linux (native + CI) | Windows, macOS |
| W2 — Byron | ✅ | ✅ | ❌ | Linux **and** Windows | macOS |
| W3 — Nathan | ❌ | ✅ | ❌ | Windows | Linux, macOS |
| W4 — Kim | ❌ | ✅ | ✅ | Windows **and** macOS | Linux |

### 2.2 The five real consequences

1. **Windows is the primary target** — 3 of 4 members have it and the graded video is recorded on our own
   hardware. Windows must be verified by hand early; macOS/Linux must never gate the submission.
2. **Nobody can test everything ⇒ CI verifies the build, not people.** Only W2 compiles for both a POSIX tty
   and a Win32 console, and nobody here can test macOS. A GitHub Actions matrix is the only way we learn that
   an `#ifdef` branch still compiles on a machine nobody is sitting in front of.
3. **Two console backends, each verifiable by exactly one person by hand:** `termios`+`poll` (W1; macOS
   shares the path, verified by W4) and Win32 console (W3).
4. **macOS is a bonus target.** The handout's shell reference names Linux / PowerShell / cmd — not macOS. One
   member has a Mac, so macOS stays a side effect of a clean POSIX backend plus one verification pass. Its
   real value is a second independent hardware baseline for the PPT's measurements.
5. **Hardware differences are a deliverable.** "Recommended values for your **current hardware**" makes the
   refresh/polling numbers per-machine evidence, not one group answer. Four machines ⇒ four tables (§T5.3).

### 2.3 The rule that falls out of this

**One platform surface = one owner.** Nobody edits a console backend they cannot build and run. Header
changes are the only cross-owner coupling, so they are frozen in Phase 0 (§3) and changed only via the
freeze protocol (§4.5).

### 2.4 Shared IDE: CLion (all four) — what it solves, and the one thing to verify

Everyone has CLion: it bundles CMake ≥ 3.21 and Ninja, reads `CMakePresets.json` directly, and its `ctest`
integration makes the Phase 0 verification a button instead of four different shell incantations.

**What it solves**

- **One build story** on all four machines. No member needs a second toolchain to participate.
- **Two shared run configurations, both committed.** `.idea/runConfigurations/csopesy-dev.xml` (CMake target
  `csopesy`, Debug preset, working dir `$ProjectFileDir$`, `--config=config/csopesy.ini` — used while
  developing) and `.idea/runConfigurations/csopesy-quiz.xml` (a **Custom Build Application** configuration
  whose `Executable` is `$ProjectFileDir$/frozen/csopesy.exe` and whose custom build target is deliberately
  **empty** — used for the recorded run). Committing both prevents run-argument drift across four members.
- **Why the quiz configuration is not a CMake target.** JetBrains documents that *"Build is the default
  pre-launch step for CMake applications."* A CMake-Application configuration therefore rebuilds — or at
  minimum relinks — when Run is pressed, which is exactly what the handout forbids at quiz time (*"no longer
  recompile the project when taking the quiz"*). CLion's *Debug arbitrary executables* page documents the
  correct route: create a custom build target and *"leave the other fields empty"*, then *"Specify the
  application binary in the Executable field"* — with no build tool configured, **nothing is built on Run**.
  (Alternative if a custom target proves awkward: a CMake-Application configuration with its `Executable`
  overridden to the frozen path **and the `Build` entry removed from Before-launch**; the gate in T0.5 Step 5
  accepts either.)
- **The video requirement is native to it.** The handout requires *pressing Run/Debug from your IDE*; CLion's
  green ▶ / 🐞 button **is** that artifact.
- **PPT evidence for free.** Attach the debugger, break inside `Scheduler::run()` (the **marquee thread**) and
  watch `proc_.cycles` / `proc_.lastRenderMs` advance there while the *main* thread sits blocked in
  `Terminal::readEvent` — a screenshot that proves two threads are live at once, which is stronger evidence
  for "process representation" and "scheduler implementation" than the v2.3 single-threaded breakpoint was.

**The thing to verify (not to assume)**

Whether CLion's Run tool window gives the program a real console is **toolchain- and debugger-dependent**.
JetBrains' *Terminal in the output console* page states that availability *"depends on the OS, debugger, and
toolchain you are using"*, and its table marks emulation **supported for MSVC LLDB on Windows** and **not
supported for GDB**. So: the option *usually works*, and we **verify it anyway** per OS before Phase 1 (T0.5).

| Option | Use it for |
| --- | --- |
| **Emulate terminal in the output console** | Lorens' Linux, Kim's macOS — and on Windows it is documented as supported with **MSVC + LLDB** (not GDB) |
| **Run in external console (Windows)** | Nathan's Windows build — yields a real console handle for `_kbhit`/`SetConsoleMode` |

Two more CLion facts that shape the tasks: presets load **read-only** and preset profiles are **disabled by
default** (enable in *Settings → Build, Execution, Deployment → CMake*; local overrides go in
`CMakeUserPresets.json`, gitignored); and CLion's default toolchain may not be the one that built the frozen
binary (bundled MinGW vs MSVC), so T0.5 pins and records it.

**Rule:** the graded run configuration must be *verified to give a real terminal* **and to build nothing**
before the freeze, and the outcome recorded in `docs/clion-run-config.md`. If neither terminal switch works,
use the T0.5b fallback: the run config launches `scripts/run_external.sh|bat`, which opens a real terminal
window and starts the **frozen** binary — the video still shows Run/Debug in CLion initializing the program,
which is what the handout asks for.

The `Terminal` contract (§3.3) is unaffected: CLion's console is just another `Terminal` implementation, and
`--no-tty` keeps replay/CI mode working regardless.

---

## 3. Frozen contracts (Phase 0 — write these before any feature work)

### 3.1 Layer rules (FSD-adapted, per `AGENTS.md`)

The project instructions require Feature-Sliced structure. FSD targets frontend frameworks, so only its
invariants are adopted — **layer order and downward-only imports**:

```text
app/       composition root: main.cpp, ConsoleApp, Scheduler. Imports everything below.
             Threading lives here: Scheduler owns the marquee std::thread, the mutex and the condition
             variable. <thread>/<mutex>/<condition_variable> are standard-library headers, so they add no
             layer and no import edge; the worker thread is an implementation detail of app/.
  ↓
features/  marquee rendering, command handling. Imports entities + shared.
  ↓
entities/  Parameters, MarqueeProcess (PCB), config I/O. Imports shared only.
  ↓
shared/    Terminal contract, FrameBuffer, shutdown contract. Imports nothing above.
  ↑
platform/  *implements* shared::Terminal + csopesy::installShutdownHandlers() for POSIX/Win32.
           May import shared only.
```

**Why `Scheduler` is in `app/`, not `entities/`:** it holds a `Renderer&` (features/marquee) and an
`Interpreter&` (features/commands) and includes `renderer.hpp` + `interpreter.hpp`. In `entities/` that would
violate the downward-only rule *and* FSD's bar on cross-imports between `features/` slices (rules 4-1 and
4-3), so `app/` — the composition root — is the only correct home for an object that wires terminal +
renderer + interpreter + process together. Moving it is the fix; introducing an interface layer to keep it in
`entities/` would be exactly the over-engineering this plan keeps cutting. (Found by the round-3 review, §7 of
the adjudication.)

**Enforced by `scripts/check_layers.sh`** (a ~15-line grep guard, wired into T0.1 and CI), not by good
intentions: no `#include "platform/..."` above `shared/`; no `entities/` header may include a `features/`
header; no `features/` header may include another `features/` slice; `platform/` may include `shared/` only.
Test doubles live in `tests/support/`, never in `shared/`. The guard exists because this exact rule was
silently violated for three review rounds.

### 3.2 Repository layout

```text
csopesy-marquee/
├─ CMakeLists.txt                  # if(WIN32) picks terminal_win32.cpp
├─ CMakePresets.json               # debug / release / windows-vs  (read-only in CLion)
├─ CMakeUserPresets.json           # LOCAL overrides only; gitignored
├─ CONTRACTS.md                    # freeze marker (§T0.4)
├─ .idea/runConfigurations/{csopesy-dev,csopesy-quiz}.xml  # COMMITTED: dev target + frozen-binary run
├─ README.txt                      # REQUIRED by the handout: names, run steps, entry file
├─ config/csopesy.ini              # the quiz-editable parameter file (the ONE live path)
├─ config/quiz_case_<n>.ini        # per-case parameter sets, copied OVER csopesy.ini to select a case (§3.6)
├─ include/csopesy/
│   ├─ keys.hpp  terminal.hpp  shutdown.hpp  parameters.hpp  config_io.hpp
│   ├─ glyphs.hpp  renderer.hpp  frame_buffer.hpp  line_editor.hpp  interpreter.hpp
│   └─ process.hpp  scheduler.hpp  console_app.hpp
├─ src/
│   ├─ main.cpp                            # ENTRY FILE (main)
│   ├─ app/{console_app.cpp,scheduler.cpp} # composition root: wires terminal + renderer + interpreter + PCB
│   ├─ features/marquee/{renderer.cpp,glyphs.cpp}
│   ├─ features/commands/{interpreter.cpp,line_editor.cpp}
│   ├─ entities/{parameters.cpp,config_io.cpp,process.cpp}
│   ├─ shared/terminal/frame_buffer.cpp
│   └─ platform/{terminal_posix.cpp,terminal_win32.cpp}   # each also defines installShutdownHandlers()
├─ tests/
│   ├─ support/{check.hpp,check.cpp,fake_terminal.hpp}    # zero-dep harness + test double
│   ├─ smoke/{basic.txt,run_smoke.cmake}                  # portable end-to-end CI test (T2.5)
│   └─ unit/{test_glyphs,test_scroll,test_parameters,test_config,
│            test_interpreter,test_scheduler,test_renderer}.cpp
├─ scripts/{build.sh,build.bat,check_layers.sh,rehearse_windows.bat,measure.sh,measure.ps1,
│            run_external.sh,run_external.bat}
├─ docs/{measurements/*.md,ppt-outline.md,clion-run-config.md,clion-toolchain.md,
│         frozen-artifact.md,threading-model.md}      # threading-model.md = T1.3 Step 6, the PPT's source
└─ .github/workflows/ci.yml
```

**Removed from v1 (deliberate):** `src/shared/terminal/ansi.cpp`, `src/features/commands/responses.cpp`,
`src/shared/terminal/fake_terminal.cpp` (→ `tests/support/`), `tests/replay/` (→ P2), `scripts/quiz_windows.bat`
(→ `rehearse_windows.bat`, rehearsal only per §7).

`tests/support/check.hpp` is a 20-line assertion harness, not a third-party framework: zero install friction
on four machines, no network, works on all three CI images.

### 3.3 `Terminal` — the only platform-dependent contract

```cpp
// include/csopesy/keys.hpp
#pragma once
namespace csopesy {
enum class KeyType { None, Char, Enter, Backspace, Tab, Escape,
                     ArrowUp, ArrowDown, ArrowLeft, ArrowRight, Eof };
struct KeyEvent { KeyType type = KeyType::None; char ch = 0; };
}

// include/csopesy/terminal.hpp
#pragma once
#include <memory>
#include <string_view>
#include "csopesy/keys.hpp"
namespace csopesy {
struct Size { int rows = 24; int cols = 80; };

class Terminal {
 public:
  virtual ~Terminal() = default;
  virtual void enterRawMode() = 0;   // no echo, no line buffering, VT output on
  virtual void restore() = 0;        // idempotent
  virtual Size size() const = 0;     // live size (resize aware)
  virtual bool readEvent(KeyEvent& out, int timeoutMs) = 0;  // BLOCKS up to timeoutMs; false on timeout
  virtual void write(std::string_view bytes) = 0;            // one call == one frame
  virtual void flush() = 0;
  virtual bool isTty() const = 0;    // false => plain line mode: no raw mode, no ANSI, no frames (§3.6)
  virtual long long nowMs() const = 0;  // monotonic; the ONLY clock source (injected into Scheduler)
  static std::unique_ptr<Terminal> create();   // defined once, in src/platform/*
};
}
```

Rationale: `readEvent`'s timeout is what makes the **polling rate** an honest, measurable parameter (see
§3.8 for what it does and does not bound); `write` taking the whole frame is what removes application-level
interleaving; `nowMs()` on the terminal is what lets the scheduler be deterministic in tests through an
injected `Clock`. There is exactly **one** clock authority (`Terminal::nowMs()`), reached by the scheduler
through `Clock`.

**Concurrency clause (v2.4).** The contract gains **one** requirement: a backend must tolerate `readEvent()`
being called from one thread while `write()`/`flush()`/`size()` are called from another. Both platform
backends already do — POSIX reads fd 0 and writes fd 1, Win32 reads `STD_INPUT_HANDLE` and writes
`STD_OUTPUT_HANDLE` — so **no backend code changes for threading**. The caller side is stricter than the
contract and is what actually prevents interleaving: `readEvent` is called **only** by the input thread, and
`write`/`flush`/`size` **only** by the marquee thread (§3.8). `FakeTerminal` is the one implementation that
must be made thread-aware, because it is in-process state rather than two OS handles (§T1.3).

`isTty() == false` (or `--no-tty`) selects **plain line mode**, defined in §3.6: the program reads lines from
stdin, prints plain responses, performs no rendering, and never emits an escape sequence. There is deliberately
no ANSI-free *animated* fallback: without cursor addressing a scrolling band repaints by scrolling the screen,
so a console that cannot enable VT must fail loudly at startup instead (risk 3).

### 3.4 Platform selection

```cmake
if(WIN32)
  list(APPEND PLATFORM_SOURCES src/platform/terminal_win32.cpp)
else()
  list(APPEND PLATFORM_SOURCES src/platform/terminal_posix.cpp)   # Linux + macOS
endif()
```

`Terminal::create()` **and** `csopesy::installShutdownHandlers()` are defined exactly once each, in the
selected file. No runtime `#ifdef` elsewhere.

### 3.5 `Parameters` — one struct, layered precedence

```cpp
// include/csopesy/parameters.hpp
#pragma once
#include <string>
#include <vector>
namespace csopesy {
struct ClampReport { bool clamped = false; int requested = 0; int applied = 0; const char* field = ""; };

struct Parameters {
  // marquee
  std::string text   = "CSOPESY";
  int  refreshMs     = 100;    // set_speed target. range [1, 10000]
  int  pollingMs     = 10;     // MAX idle wait when nothing is ready. [1, 1000]; does NOT delay keys (§3.8)
  bool asciiArt      = true;   // false => single-row plain scroll (CLI: --plain)
  int  marqueeRow    = 3;      // 1-based first band row. config/CLI only (§1.4)
  // chrome  (matches the handout's mock exactly)
  std::vector<std::string> developers = {"De La Cruz, Juan", "Santos, Alex"};
  std::string versionDate = "";          // blank in the mock; blank is valid
  // infra
  bool noTty = false;                    // force plain line mode (no raw mode, no ANSI, no frames)
  std::string measurePath;               // --measure=FILE (§3.6); empty => off. CLI-only, set before threads start

  static constexpr int kRefreshMin = 1, kRefreshMax = 10000;
  static constexpr int kPollMin = 1,    kPollMax = 1000;

  ClampReport setRefresh(int ms);      // clamps + reports; never throws
  ClampReport setPolling(int ms);
  bool setText(const std::string& t);  // false if empty after trim (state unchanged)
  static Parameters defaults();
};
}
```

**Precedence, highest wins:** runtime command > CLI flag > config file > built-in default. Each layer assigns
only what it explicitly provides. Implemented as: load config → apply CLI → the interpreter mutates the live
struct.

**Shared-state rule (v2.4).** `Parameters` is now read by the marquee thread at the same time as the input
thread mutates it through the interpreter, so every access originating in `tick()` happens under
`Scheduler::mu_` (§3.8). The input thread reads its own `pollingMs` for the `readEvent` timeout without the
lock, which is safe because that thread is also the only writer — the rule is *one writer, all cross-thread
readers under the mutex*. `measurePath` is the exception that proves the rule: `main` writes it once **before
any thread exists** and nothing writes it again, so the marquee thread reads it without the lock.

**Which knobs have runtime commands:** only the four the handout names (`set_text` → `text`, `set_speed` →
`refreshMs`, `start_marquee`/`stop_marquee` → process state). `pollingMs`, `asciiArt`, `marqueeRow`,
`developers`, `versionDate` are config/CLI only. Only `text`, `refreshMs` and `asciiArt` carry the
partial-credit redundancy (config + CLI + command where applicable).

**Deliberately absent from v1:** `stepCols` (no spec value; scroll is 1 column per frame), `scheduler` as a
*config knob* (there is exactly one scheduler and it is now threaded — the architecture is §3.8, not a
parameter), `devCommands`, `statsPath` (replaced by `--diag` and `--measure`, §3.6).

### 3.6 Config file (`config/csopesy.ini`) and CLI — the no-recompile levers

```ini
# CSOPESY marquee parameters. Edit freely; no rebuild required.
# '#' begins a comment only at the start of a line (after optional spaces).
text          = CSOPESY
refresh_ms    = 100
polling_ms    = 10
ascii_art     = true
marquee_row   = 3
developers    = De La Cruz, Juan; Santos, Alex
version_date  =
```

Parser rules that must be unit-tested: unknown key → warn to stderr **and keep going** (a typo must never cost
the quiz); malformed number → warn + keep default; missing file → run on defaults (never fatal); `--config=`
path honoured; values trimmed; `developers` split on `;` with each element trimmed.

**Quiz case files — the one-live-path rule.** A *case* is one graded scenario (`n` is the professor's
case number, or a §6.2 acceptance case while rehearsing). Each case's parameter set lives in
`config/quiz_case_<n>.ini`, same schema as above. These files are *inputs*, never a second live path: the
graded run configuration passes the literal `--config=config/csopesy.ini` (§T0.5), so a case is selected
by **copying its file over `config/csopesy.ini`** (§7 T-0:02). Nothing loads `quiz_case_<n>.ini` directly.
This matters because a missing file is non-fatal by design (above): a stale `config/csopesy.ini` from the
previous case would silently run the *wrong* parameters with no error at all — precisely the failure the
copy step exists to prevent.

CLI flags: `--config=PATH  --text=STR  --refresh=N  --polling=N  --plain  --art  --row=N
--developers="A;B"  --version-date=STR  --no-tty  --diag  --measure=FILE`

- `--diag` prints a one-shot environment report to stderr (isTty, size, VT enabled, terminal kind, effective
  refresh/polling) and exits 0. It exists solely for the T0.5 verification gate and the runbook's T-0:02
  check. It is the *only* diagnostic flag; `ps`, `params`, `stats`, `--self-test` are cut.
- `--measure=FILE` appends **two kinds of line, and the distinction is the contract** (rewritten in v2.5 —
  `gpt-v5.md` §1 showed v2.4 claimed "one line per keystroke" and admitted coalescing two paragraphs later):
  - `frame_index,nowMs` — **one line per *refresh* frame**, emitted only when `tick()` renders because the
    `refreshMs` deadline was `due`. `frame_index` is `MarqueeProcess::cycles`, so successive lines *are* the
    refresh cadence; keystroke echo frames deliberately do **not** appear here, because mixing them in would
    corrupt the p50/p95 frame-interval series (§T1.4). Written by the marquee thread.
  - `eventMs,echoMs` — **one pair per rendered echo frame that consumed at least one queued keystroke**.
    `eventMs` is the stamp the input thread captured when `readEvent` returned; `echoMs` is that frame's own
    timestamp. `Parameters::measurePath` (infra, CLI-only) names the file; the marquee thread is its **only**
    writer, and both appends happen **outside** `mu_` (§3.8 rule 2).
  - **Coalescing is explicit, not accidental.** Keys that queue between two frames produce **one** frame and
    therefore **one** pair. `eventMs` is the **oldest** unconsumed keystroke, so the number is worst-case
    processing latency rather than a flattering last-key sample — v2.4 overwrote the stamp on every
    `postEvent`, which *understated* it. The pair is emitted once and cleared; v2.4's sketch never cleared it,
    so one keystroke would have produced an `eventMs,echoMs` line on **every** subsequent frame.
  - A keystroke that lands on a `due` frame emits both a frame line and an event pair from that one frame;
    the two series stay separate, and `frame_index` is still only `cycles`.
  It is the source of the PPT's p50/p95 frame-interval and **input-to-echo processing latency** numbers
  (§T5.3). It **cannot** measure physical keypress-to-pixel latency: `eventMs` is the moment `readEvent`
  returned, i.e. *after* the OS delivered the key. Perceived typing delay is therefore recorded as a separate,
  human-observed judgement during the sweep, never inferred from this file. With `--no-tty` no worker starts, so
  `--measure` is inert there (§T2.5 Step 2b).
- `--no-tty` forces **plain line mode**: no raw mode, no ANSI, no animation. Commands are read line-by-line
  from stdin, each response is printed as a plain line, and `exit`/EOF returns 0. This is the behaviour behind
  `isTty() == false`, and it is what lets CI smoke-test the real binary on all three OS images (T2.5).
  **No worker thread is started in plain line mode** — `--no-tty` stays single-threaded, which is why its
  output remains byte-identical to v2.3's (§3.8, T2.5 Step 2b).

### 3.7 Command contract — exact strings (tests assert these)

| Input | Response |
| --- | --- |
| `help` | the 6-line table below |
| `start_marquee` (stopped) | `Marquee started.` |
| `start_marquee` (running) | `Marquee is already running.` |
| `stop_marquee` (running) | `Marquee stopped.` |
| `stop_marquee` (stopped) | `Marquee is not running.` |
| `set_text HELLO WORLD` | `Marquee text set to "HELLO WORLD".` |
| `set_text` (no arg) | `Usage: set_text <text>` — state unchanged |
| `set_speed 250` | `Marquee speed set to 250 ms.` |
| `set_speed 0` | `Speed 0 ms is out of range [1, 10000]; clamped to 1 ms.` |
| `set_speed` / `set_speed abc` | `Usage: set_speed <milliseconds>` — state unchanged |
| `exit` | `Exiting CSOPESY. Goodbye!` then restore terminal, exit 0 |
| unknown `foo` | `Command not recognized: "foo". Type "help" for the list of commands.` |
| empty line | no message; prompt redrawn |

```text
Available commands:
  help           - displays the commands and its description
  start_marquee  - starts the marquee animation
  stop_marquee   - stops the marquee animation
  set_text       - accepts a text input and displays it as a marquee
  set_speed      - sets the marquee animation refresh in milliseconds
  exit           - terminates the console
```

**Recognition rules** (one rule, no ambiguity — this replaces v1's two contradictory rules):

1. Trim the raw line → `line`. Empty ⇒ no output, no state change.
2. `cmd` = the substring up to the first whitespace run; the rest (after that run) is `arg`.
3. Match `cmd` **case-sensitively** against the six commands (the handout spells them lowercase);
   `HELP` is reported as unrecognized.
4. Trim `arg`'s surrounding whitespace; **internal whitespace runs are preserved verbatim**.
   `set_text   HELLO   WORLD` ⇒ `text == "HELLO   WORLD"`.
5. `arg` empty after trimming ⇒ the command's `Usage:` line, state unchanged.
6. `set_speed`'s argument must match `[-+]?[0-9]+` **in full**. Anything else (`abc`, `5abc`, `1.5`, `+`) is
   **not a number** ⇒ the `Usage:` line — never a silent partial parse, which is what `std::stoi` would do to
   `5abc`. A well-formed value outside `[1, 10000]`, **including negatives such as `-5`**, is *not* a usage
   error: it is clamped and reported, because A5 grades the visible clamp.

Tests assert the **token** for state messages (`Marquee started`, `already running`, `out of range`,
`not recognized`) and the **whole line** for every `Usage:`, the `help` header, and the goodbye line. This
keeps drift detectable without freezing punctuation the handout never specified.

### 3.8 Process + scheduler contract

```cpp
// include/csopesy/process.hpp
#pragma once
#include <string>
namespace csopesy {
enum class ProcessState { Stopped, Running };   // Ready/Finished are unused: v1 kept them as decoration

struct MarqueeProcess {                 // PCB
  int pid = 1;
  std::string name = "marquee";
  ProcessState state = ProcessState::Stopped;
  long long cycles = 0;                 // rendered frames == "instructions executed"
  long long lastRenderMs = 0;
  bool hasRendered = false;             // false => the next tick draws immediately (fresh OR restarted)
  bool start();                         // false if already Running; clears hasRendered
  bool stop();                          // false if already Stopped
};
}

// include/csopesy/scheduler.hpp
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
```

**Threading model, in one table.** Everything follows from *who writes what*: a resource with one writer needs
no lock, and a resource with two writers does. `mu_` is the only mutex in the program.

| State | Writer(s) | Reader(s) | Protected by |
| --- | --- | --- | --- |
| `Parameters`, `MarqueeProcess`, `Interpreter::line_`/`message_` | input thread (interpreted commands) **and** marquee thread (`cycles`, `lastRenderMs`, `hasRendered`) | both | `mu_` — **except `Parameters::measurePath`**, which `main` writes once before any thread exists and the marquee thread then reads lock-free (§3.5) |
| `Interpreter::quit_` | input thread only (`feed`/`executeLine`, reached under `mu_` via `postEvent`) | input thread only — **the worker never reads it** (v2.6) | none needed: shutdown crosses to the worker as `stop_`, set by `requestStop()` (§3.10) |
| `Scheduler::stop_`, `dirty_`, `wakeTick_`, `pendingEventMs_`, `eventOwed_` | both | both | `mu_`; the *change* is delivered by `cv_` |
| `Scheduler::worker_` | input thread only (`start`/`join`/`~Scheduler`) | input thread only | none needed — **never** call `join()` from the worker |
| `FrameBuffer fb_`, `fbRows_`/`fbCols_`, `measure_`, every `Renderer` call | marquee thread only | marquee thread only | none needed |
| `Terminal::write`/`flush`/`size` | marquee thread only | marquee thread only | none needed |
| `Terminal::readEvent` | input thread only | input thread only | none needed |
| shutdown flag | signal handler (write) | input thread (read-and-clear) | `volatile std::sig_atomic_t` |

Taking a lock is therefore the *exception*, not the rule: the `FrameBuffer` is not shared, the terminal is not
shared, and the renderer is not shared — each has exactly one owning thread. `mu_` covers only the state that
genuinely crosses the boundary, which is why this design uses one mutex and one condition variable rather than
atomics sprinkled over every field.

**Three rules that make it deadlock-free and bound the critical section.** (1) `tick()` is **never** called
while `mu_` is held: `run()` releases its `unique_lock` before stepping, and
`postEvent`/`wake`/`requestStop` hold `mu_` only for field writes. (2) Every blocking I/O call happens
**outside** `mu_`: the frame is assembled into a `std::string` under the lock and written after unlocking, and
the `--measure` lines are likewise *formatted* under the lock and *appended* after it. (3) `Terminal::size()`
— an `ioctl`/`GetConsoleScreenBufferInfo` syscall — is read **before** the lock, because the ownership table
gives `size()` to the marquee thread alone and it therefore needs no protection; only the comparison against
`fbRows_`/`fbCols_` stays under the lock. There is no second mutex, so there is no lock order to get wrong.

**What the lock actually costs (v2.5 — the honest version of "typing stays responsive").** With rules 2 and 3
the critical section contains **no syscall at all**: just the interpreter feed, the scroll math, `drawFrame()`
and `renderDiff()`. So the wait another thread can incur is one frame's *render*, never console I/O. That wait
is real and is not denied here: a wide, dense frame (a 200-character drawing — A4's extreme) is CPU-bound work,
and `postEvent()` from the input thread can briefly queue behind it. Two consequences the PPT and
`docs/threading-model.md` must state plainly instead of overclaiming:

- the guarantee is *"the unbounded console write can never stall the input thread"*, **not** *"the input
  thread never waits"* (`gpt-v5.md` §2 is right that v2.4 said the first and implied it meant the second); and
- T1.4's sweep reports the measured worst-case `eventMs,echoMs` at the A4 extreme, which is precisely the
  number that covers this window.

This is deliberately **not** solved by the snapshot-then-render-outside-the-lock alternative: that needs a copy
of `Parameters`, the PCB and the whole interpreter view on every frame, and its consistency argument is harder
to defend than the measured wait it removes (`REVIEW_ADJUDICATION.md` §9 — rejected as scope, not as correctness).

**One input owner (the v1 double-poll fix, now enforced by the thread split).** `tick()` receives an event that
has already been read and already fed; it never calls `readEvent`, and the worker always steps with
`ev == nullptr` because the input thread delivers keystrokes through `postEvent`. The input thread is also the
sole reader of the shutdown flag **and** (v2.6) the sole reader and writer of `Interpreter::quit_` — the worker
never asks the interpreter whether the user left, because §3.10 already routes that decision through `stop_`.
(`Terminal&` is still held by the scheduler for `write`/`flush`/`size`.)

**Even the threading tests need no sleeps.** `wake()` exists for exactly one reason: it is the explicit
"re-evaluate now" signal, so a test can advance the injected `Clock` and force the worker to step **without
waiting on real time**. It is two lines, its notify is shared with `requestStop`, and it is what makes T1.3's
threaded assertions deterministic. **It is test-only by design**: every production keystroke already goes
through `postEvent()`, which sets `dirty_` and notifies, so `ConsoleApp` never calls `wake()`. `wakeTick_` is
therefore not speculative state — deleting it (as `gpt-v6.md` suggested) would force the threaded tests to wait
on real milliseconds or to fake a `dirty_` redraw, and both break §6.1's no-sleep rule (see §10.11).

```cpp
// src/app/scheduler.cpp (excerpt) — the marquee thread's pure step, its loop, and the input-thread API
TickResult Scheduler::tick(const KeyEvent* ev) {
  TickResult res;
  // Rule 3: size() is a syscall AND a marquee-thread-only resource, so it is read BEFORE the lock. It also
  // stays the first action of tick(), which is what makes the T1.1 double's size() counter a tick barrier:
  // "size() call n+1" still proves tick n returned (ticks are sequential in the worker).
  const Size sz = term_.size();
  std::string frame;                   // assembled UNDER the lock, written OUTSIDE it (rule 2)
  std::string measure;                 // formatted UNDER the lock, appended OUTSIDE it (rule 2)
  {
    std::lock_guard<std::mutex> lk(mu_);
    const long long now = now_();
    bool redraw = false;
    if (ev != nullptr) { interp_.feed(*ev); redraw = true; noteEventLocked(now); }
    // `dirty_` is how a keystroke's echo frame is requested: `run()` always steps with ev == nullptr, because
    // the INPUT thread owns readEvent and delivers through postEvent(). Posting and stepping are therefore
    // decoupled, and echo stays un-gated by refreshMs exactly as in v2.3.
    if (dirty_) { dirty_ = false; redraw = true; }

    // Follow the live terminal size — the ONE resize mechanism, on both platforms (§3.3). Unchanged from
    // v2.3, and now executed on the marquee thread. `resize()` alone is not a repaint: `redraw` must be set
    // too, or an event-less resize (the Windows case) would resize the buffer and still show the old layout.
    if (sz.rows != fbRows_ || sz.cols != fbCols_) {
      fb_.resize(sz.rows, sz.cols); fbRows_ = sz.rows; fbCols_ = sz.cols;
      redraw = true;
    }

    const bool running = proc_.state == ProcessState::Running;
    // `!hasRendered` makes a freshly started process draw its FIRST frame immediately (unchanged v2.1 rule).
    const bool due = running && (!proc_.hasRendered || now - proc_.lastRenderMs >= params_.refreshMs);
    if (due || redraw) {
      renderer_.drawFrame(fb_, params_, proc_, interp_.prompt(), interp_.buffer(), interp_.lastMessage());
      frame = fb_.renderDiff();        // ONE assembled frame
      // --measure, part 1 (§3.6): the frame line belongs to a REFRESH frame only (`due`). frame_index is
      // cycles, so the series IS the refresh cadence; echo frames must not be mixed into it.
      if (due) { measure += frameLine(proc_.cycles, now); }
      // --measure, part 2: at most one pair per echo frame, carrying the OLDEST unconsumed keystroke, emitted
      // once and cleared. Coalescing is the contract, not an accident: N queued keys => one frame, one pair.
      if (eventOwed_) { measure += eventLine(pendingEventMs_, now); eventOwed_ = false; }
      if (due) {
        proc_.lastRenderMs = now; proc_.hasRendered = true; proc_.cycles += 1; res.rendered = true;
      }
    }
    // v2.6: the worker does NOT read interp_.quitRequested(). `quit_` is now single-threaded (input thread
    // only), and shutdown reaches this thread solely through `stop_` — the input thread observes `quit_` and
    // calls requestStop() (§3.10). A worker read here re-introduces the one cross-thread read this plan had
    // to hedge about, and it removes nothing (REVIEW_ADJUDICATION.md §11).
    //
    // The shutdown FLAG is likewise not read here: it is read-and-clear, and only the INPUT thread reads it,
    // converting it into requestStop() (§3.10). Two readers of a read-and-clear flag would race for it.
  }
  // THE only terminal write in the program, on the marquee thread only, and outside mu_ so that a slow console
  // write can never stall the input thread (rule 2).
  if (!frame.empty()) { term_.write(frame); term_.flush(); }
  // The measure file is not the terminal, but the same rule applies: format under the lock (so the pair really
  // belongs to the frame that carried the echo), append after it, marquee thread only (rule 2).
  if (!measure.empty()) { appendMeasure(measure); }
  return res;
}

void Scheduler::noteEventLocked(long long now) {      // caller holds mu_
  if (!eventOwed_) { pendingEventMs_ = now; eventOwed_ = true; }   // keep the OLDEST: worst case, not last key
}

int Scheduler::pollTimeoutMsLocked() const {          // caller holds mu_
  long long ms = params_.pollingMs;
  if (proc_.state == ProcessState::Running) {
    const long long until = params_.refreshMs - (now_() - proc_.lastRenderMs);
    if (until > 0 && until < ms) { ms = until; }
  }
  if (ms < 1)    { ms = 1; }
  if (ms > 1000) { ms = 1000; }
  return static_cast<int>(ms);
}
int Scheduler::pollTimeoutMs() const { std::lock_guard<std::mutex> lk(mu_); return pollTimeoutMsLocked(); }

int Scheduler::run() {                                // the MARQUEE THREAD body
  for (;;) {
    {
      std::unique_lock<std::mutex> lk(mu_);
      if (stop_) { return 0; }
      const long long seen = wakeTick_;
      // A bounded TIMED WAIT, never a spin: it wakes on (a) the render deadline, (b) a postEvent/stop/wake
      // notify, (c) pollingMs elapsing. It never holds a core while idle.
      cv_.wait_for(lk, std::chrono::milliseconds(pollTimeoutMsLocked()),
                   [this, seen] { return stop_ || dirty_ || wakeTick_ != seen; });
      if (stop_) { return 0; }            // release mu_ BEFORE stepping — rule 1
    }
    (void)tick(nullptr);                  // pure step; the input thread already fed the event via postEvent
    // No `if (tick(...).quit)` branch any more (v2.6): `stop_` is the worker's ONLY exit condition, and every
    // exit path in §3.10 sets it from the input thread. The next iteration's `if (stop_)` returns.
  }
}

// --- input/command thread (§T2.5); every method below is called from the main thread ---
void Scheduler::postEvent(const KeyEvent& ev) {
  { std::lock_guard<std::mutex> lk(mu_); interp_.feed(ev); dirty_ = true; noteEventLocked(now_()); }
  cv_.notify_one();                       // harmless if the worker is already stepping
}
void Scheduler::wake()        { { std::lock_guard<std::mutex> lk(mu_); ++wakeTick_; } cv_.notify_one(); }
void Scheduler::requestStop() { { std::lock_guard<std::mutex> lk(mu_); stop_ = true; } cv_.notify_all(); }
void Scheduler::join()        { if (worker_.joinable()) { worker_.join(); } }

// join()/joinable()/start()/~Scheduler touch worker_ and are therefore INPUT-THREAD ONLY (ownership table).
// Calling join() from run() would be a self-join (std::system_error / terminate) — do not.
bool Scheduler::joinable() const { return worker_.joinable(); }
void Scheduler::start() {
  // v2.5: no `started_` flag. worker_ is the input thread's own field, so joinable() IS the started/stopped
  // state, and the spawn is exception-safe by construction: if std::thread's constructor throws, nothing was
  // mutated and the scheduler simply stays unstarted (v2.4 set started_ = true first, and could not undo it).
  if (worker_.joinable()) { return; }
  worker_ = std::thread([this] { run(); });
}
Scheduler::~Scheduler() { requestStop(); join(); }   // a worker can never outlive the objects it touches

SchedulerSnapshot Scheduler::snapshot() const {
  std::lock_guard<std::mutex> lk(mu_);
  return {proc_.state, proc_.cycles, proc_.lastRenderMs, now_(), proc_.hasRendered, params_.refreshMs,
          params_.pollingMs};
}

void Scheduler::appendMeasure(const std::string& s) {   // marquee thread only; params_.measurePath is read-only
  if (params_.measurePath.empty()) { return; }
  if (!measure_.is_open()) { measure_.open(params_.measurePath, std::ios::app); }
  if (measure_) { measure_ << s; measure_.flush(); }
}
```

**Every cross-thread field, and what would break without it (v2.5).** `gpt-v6.md` asked for each piece of
`Scheduler` state to be challenged before implementation instead of accreting. This is that challenge, and it
doubles as the "do not add to this list without a failing test" list.

| Field | The one job it does | Delete it and… |
| --- | --- | --- |
| `mu_` | makes both threads' reads/writes of `Parameters`/PCB/`Interpreter` well-defined | TSan reports a race on the first command |
| `cv_` | releases a worker blocked in `wait_for` on a *state* change, not only on the deadline | `requestStop()` could not interrupt an idle wait ⇒ shutdown waits out `pollingMs` |
| `stop_` | the shutdown edge; `run()`'s exit test | `exit` could not stop the worker at all |
| `dirty_` | "an echo frame is owed", set by `postEvent` | typing would not redraw until the next deadline ⇒ echo latency becomes `refreshMs` |
| `wakeTick_` | the test-only "re-evaluate without repainting" edge behind `wake()` | Step 1b would need real-time waits to force a step under the injected clock, breaking §6.1 DoD 2 (§10.11 **rejects** the proposal to delete it) |
| `pendingEventMs_` + `eventOwed_` | the `--measure` event pair: oldest unconsumed stamp, emitted once | the metric vanishes — or with one overwritten slot it reports a flattering last-key sample and repeats on every frame |
| `worker_` | the join handle; `joinable()` **is** the started/stopped state | no clean join — and this is exactly why v2.5 could delete `started_` |

Semantics to test — all of these are deterministic, with **no real-time sleep anywhere in the suite**: a
render happens exactly when `state == Running && (!hasRendered || now - lastRenderMs >= refreshMs)` — the
`!hasRendered` term is what makes `start_marquee` paint immediately rather than after one full refresh
interval; a **redraw** also happens for every consumed event, which is what keeps typing latency independent of
`refreshMs`; `cycles` increments once per *rendered* frame, not per redraw; `set_speed 500` moves the next
deadline without a recompile; a live size change **alone** repaints the frame at the new size, with no event
required (that is what makes the Windows path work, §3.3); and `pollTimeoutMs()` is never 0 (a 0 would
busy-spin a core) and never exceeds `pollingMs`. On top of those, the threaded model adds four invariants that
T1.3 asserts directly: exactly one thread ever calls `Terminal::write`, every `write()` carries exactly one
whole frame, the worker leaves `wait_for` promptly on `postEvent`/`wake`/`requestStop` (no busy-wait and no
`sleep_for`-only shutdown), and `join()` returns with no worker left alive. The `--measure` rewrite adds two
more, pinned by the new Step 1c tests: a `frame_index,nowMs` line appears **only** for a `due` frame, and an
`eventMs,echoMs` pair is emitted **once** per echo frame with the **oldest** unconsumed stamp.

**What `pollingMs` does and does not do** (this is the v2.1 correction, and it shapes the PPT): the `readEvent`
timeout is a **maximum idle wait**, not a sampling interval. `poll()`/`WaitForSingleObject` return the instant a
key arrives, and `pollTimeoutMs()` is *additionally* capped by the next render deadline, so `pollingMs` **cannot
add keystroke latency and cannot delay a frame**. What it genuinely bounds:

1. **Idle wakeups per second** — smaller `pollingMs` ⇒ more wakeups ⇒ more CPU while nothing is happening.
   Note what threading costs: `pollingMs` is the **only** timer in *either* thread, so it still does not drive
   the marquee (the render deadline caps the worker's wait), but there are now **two** idle waiters instead of
   v2.3's one. At the same `polling_ms` the idle-wakeup count is therefore roughly doubled; T1.4/T5.3 measure
   it and the PPT states it as the price of the architecture rather than hiding it.
2. **Latency of flag-only conditions** — on Windows, `SetConsoleCtrlHandler` runs on a separate thread and
   cannot interrupt the input thread's blocked `WaitForSingleObject`, so `pollingMs` bounds Ctrl+C-to-exit. On
   POSIX, signals interrupt `poll()` with `EINTR`, so flagged conditions are noticed promptly there regardless
   of `pollingMs`. In the threaded design the *input* thread is the one blocked on the console, and it is the
   only reader of the flag (§3.8, §3.10).
3. **Resize-onset latency** — both backends learn about a resize by re-reading `Terminal::size()` in `tick()`
   (on the marquee thread), so a resize is applied within one `pollTimeoutMs()`. That is ≤ 10 ms at the default
   `polling_ms`, but a deliberately large `polling_ms` makes an *idle* window resize visibly lag. During
   animation the render deadline caps the wait, so the band itself is never late.

Consequently the sweep reports what the knob actually controls (wakeups, control-flag latency) and the PPT
says plainly that typing delay in this design is driven by the refresh rate and per-frame render cost, not by
`polling_ms` — with data showing it (§T5.3, §T6.1). That is a stronger answer than a fabricated threshold.

**Why threading, and what it does *not* buy.** The professor's example program animates while the command
interface stays interactive, so the marquee's timing work must not share a control path with command entry; two
threads express that directly. The ownership table above shows the cost is one mutex and one condition variable
— no atomics over every field, no lock-free queue, no thread per marquee process, and no platform-specific
threading code (`std::thread` is C++17 on all three targets). **Multithreading does not remove screen tearing.**
The application-level guarantee is unchanged and is now *stronger* in one specific way: one assembled frame per
`write()`, issued by exactly one thread, so two frames cannot even be *assembled* concurrently, let alone
interleaved. It still does not make terminal repaint atomic, and no escape sequence is added to chase that. The
PPT says exactly this, in these words.

### 3.9 Renderer, glyphs, FrameBuffer, header layout

```cpp
// include/csopesy/glyphs.hpp
#pragma once
#include <string>
#include <string_view>
#include <vector>
namespace csopesy {
constexpr int kCellCols = 5;   // every glyph row is EXACTLY 5 columns wide
constexpr int kCellRows = 5;
constexpr int kArtRows  = kCellRows;   // band height in art mode; 1 in plain mode

struct Glyph { const char* rows[kCellRows]; };
const Glyph* glyphFor(char c);          // never nullptr (unknown -> box glyph)
std::vector<std::string> renderBlockText(std::string_view text);  // kCellRows rows, 1-col gap
}

// include/csopesy/frame_buffer.hpp
#pragma once
#include <string>
#include <string_view>
#include <vector>
namespace csopesy {
class FrameBuffer {
 public:
  void resize(int rows, int cols);
  void put(int row, int col, char c);            // 1-based
  void putRow(int row, int col, std::string_view s);
  std::string renderDiff();                      // ONE string; the caller issues ONE write()
  void invalidate();                             // force full repaint (after resize/clear)
};
}

// include/csopesy/renderer.hpp
#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "csopesy/parameters.hpp"
#include "csopesy/process.hpp"
#include "csopesy/frame_buffer.hpp"
namespace csopesy {
// Pure scroll math: frame `cycles` puts column scrollOffset(...) at band column 0.
int scrollOffset(long long cycles, int artWidth, int bandWidth);
std::string sliceRow(std::string_view artRow, int bandWidth, int offset);  // pads with spaces

class Renderer {
 public:
  void drawFrame(FrameBuffer&, const Parameters&, const MarqueeProcess&,
                 const std::string& prompt, const std::string& buffer, const std::string& message);
  static int artWidthFor(std::string_view text);         // == kCellCols*len + (len-1)
  std::vector<std::string> composeBand(const Parameters&, const MarqueeProcess&) const;
};
}
```

**Layout** (1-based rows, computed from `params.marqueeRow`; defaults shown):

```text
row 1        Welcome to CSOPESY!          <- capital W (§1.4)
row 2        (blank)
row 3..7     band: 5 art rows, or 1 plain row. Scrolled by scrollOffset(cycles, artWidth, cols)
row 8        (blank)
row 9        Group developer:
row 10..n    one name per row from Parameters::developers
             (blank)
             Version date: <versionDate>   <- no trailing space when blank
             (blank)
             Command> <visible slice of buffer>   <- ONE row; the slice scrolls horizontally (§3.11)
```

Invariants asserted in tests: `sliceRow` output length is **always exactly** `bandWidth`; `scrollOffset` wraps
at `artWidth + bandWidth` (art scrolls fully off before re-entering); a frame contains every band row exactly
once; header rows are repainted only when their text changes; `renderDiff()` output is a single string.

**Tearing claim (v2 wording, tightened in v2.4 — do not overstate):** emitting one assembled frame per
`write()` prevents *application-level* interleaving of two frames. The threaded design strengthens *that*
specific guarantee — the frame is now assembled and written by one owning thread, so two frames cannot even be
built concurrently — but it does **not** make terminal repaint atomic. No escape sequence is added to chase
that, and "we use threads" is not offered anywhere as a tearing fix. This is the wording the PPT must use.

### 3.10 Shutdown contract

```cpp
// include/csopesy/shutdown.hpp
#pragma once
namespace csopesy {
void installShutdownHandlers();   // POSIX: SIGINT/SIGTERM -> flag. Win32: SetConsoleCtrlHandler -> flag
bool shutdownRequested();         // read-and-clear
}
```

Rules: the handler **only** sets a `volatile std::sig_atomic_t` flag (async-signal-safe). No terminal
restoration, no allocation, no I/O in the handler, and no condition-variable notify from the handler (that is
not async-signal-safe here, and it is unnecessary: the input thread wakes on `poll` `EINTR` or at its next
`pollingMs` timeout).

**Shutdown is a sequence, and the order *is* the safety property (v2.4).** Every exit path — `exit`, `Ctrl+C`,
`SIGTERM`, EOF — goes through the same five steps, all of them in normal program flow:

1. **Intent.** `exit` sets `Interpreter::quit_` (via `executeLine`); a signal sets the `sig_atomic_t` flag.
   The handler does nothing else.
2. **Observe.** The **input thread** is the only reader of both: its loop tests
   `interp_.quitRequested() || csopesy::shutdownRequested()` after every `readEvent` (and immediately after an
   `EINTR`/timeout on POSIX). One reader, so the read-and-clear flag can never be raced for. **(v2.6: it is now
   the only reader of `quit_` full stop — the worker does not read it, so `quit_` needs no synchronization at
   all; without the reader this step is the entire quit mechanism.)**
3. **Signal the worker.** The input thread calls `Scheduler::requestStop()`: `{lock; stop_ = true;}` then
   `cv_.notify_all()`. That condition-variable wakeup is what releases a worker sitting in `wait_for` — not a
   `sleep_for` loop and not luck with the deadline. **Since v2.6 this is the worker's *only* exit signal**
   (`run()` no longer reads `quit_`), which is why this step must exist on every path, not just `exit`.
4. **Join.** The input thread calls `Scheduler::join()`, which blocks until `run()` returns. **The join happens
   before anything is destroyed and before the terminal is restored**, so no worker can touch a
   half-destroyed object or write into a restored terminal. `exit` therefore leaves **no detached thread**
   behind: there is no `detach()` anywhere in the program, and `~Scheduler()` performs
   `requestStop()` + `join()` as a backstop on any early return.
5. **Restore.** Only now does normal-scope cleanup run: the `TerminalGuard` destructor (with `atexit` as a
   backstop) calls the idempotent `restore()`. Nothing wrote to the terminal after the join except the plain
   goodbye line, printed by the input thread once it is the only thread again (§T2.5).

`SIGKILL`, power loss and kernel crashes cannot be handled — the runbook's recovery step is `stty sane` / a
fresh window. The T1.1 checklist verifies `exit`, `Ctrl+C`, and (where practical) `SIGTERM`; T1.3's tests
assert the join itself.

### 3.11 Interpreter / line editor contract

```cpp
// include/csopesy/interpreter.hpp
#pragma once
#include <string>
#include "csopesy/parameters.hpp"
#include "csopesy/process.hpp"
namespace csopesy {
// The horizontal-scroll window for the prompt row (§3.11). Unit-tested directly: result.size() <= availWidth,
// and when the buffer is longer than the window the TAIL is shown so the cursor stays visible.
std::string visibleSlice(std::string_view buffer, int availWidth);

class Interpreter {
 public:
  Interpreter(Parameters&, MarqueeProcess&);   // start/stop_marquee mutate the PCB
  bool feed(const KeyEvent&);                  // true when a full line was executed
  std::string executeLine(const std::string&); // the entry point used by unit tests
  bool quitRequested() const;
  std::string prompt()      const;             // "Command>"
  std::string buffer()      const;             // echoed typed text
  std::string lastMessage() const;             // the response line(s) currently displayed
 private:
  Parameters& params_; MarqueeProcess& proc_;
  std::string line_, message_; bool quit_ = false;
};
}
```

Line editor rules (raw mode gives no echo, so we own it): printable chars append; `Backspace` deletes the char
before the cursor (no-op at column 0); `Enter` submits; `Eof` submits/quiets; arrows are ignored (no crash, no
state change).

**Threading note (v2.4; `quit_` ownership fixed in v2.6).** The `Interpreter` is the one object with two threads
inside its call graph: the **input** thread calls `feed`/`executeLine` (mutating `line_`, `message_`, and through
them `Parameters` and `MarqueeProcess`), and the **marquee** thread reads `prompt`/`buffer`/`lastMessage` while
rendering. Those accesses are therefore made under `Scheduler::mu_` (§3.8). **`quit_` is the exception and is now
strictly single-threaded:** it is written by `feed`/`executeLine` (reached under `mu_` through `postEvent`) and
read only by the input thread itself in its observe step (§3.10 step 2); the worker never reads it, and the
returned `TickResult` no longer carries it. `Interpreter` itself takes **no** lock: the caller owns it, which is
what keeps `executeLine`'s unit tests lock-free and single-threaded. Do not add a mutex here — it would be a
second lock, and the §3.8 design deliberately has exactly one. The command line stays on **one fixed row**: when the buffer is wider than the space available
(`cols - len("Command> ") - 1`), the *displayed* slice scrolls horizontally so the cursor stays visible, rather
than wrapping onto more rows. Horizontal scrolling is chosen deliberately over multi-row wrapping, because a
wrapped prompt would have to displace the band/header rows in a fixed-layout frame — a whole class of
cursor-and-redraw bugs traded for a `substr` window.

---

## 4. Workload split — four workstreams, each with a reason

### 4.0 Why the split is drawn along OS capability

The risk is not "who writes the most code" — it is the ~14 `#ifdef`/API subtleties that only surface by
*running* on that platform (raw-mode flags, `\r\n` under raw mode, `Ctrl+C` no longer producing `SIGINT`,
`_getch` arrow prefixes, explicitly enabling VT, QuickEdit freezing the process, `TIOCGWINSZ`, resize events).
Assigning each platform surface to the person who owns that platform means every hour produces *verified*
work and nobody waits on a machine they do not have. Each workstream therefore gets **one platform primary**
plus **one pure-logic/documentation deliverable** so nobody idles behind an interface.

### 4.1 W1 — Lorens (Linux-only)

**Primary:** POSIX console backend `src/platform/terminal_posix.cpp`.
**Supporting:** process/scheduler core (`entities/process.cpp`, `app/scheduler.cpp`).

| | |
| --- | --- |
| **Why this person** | Linux-only ⇒ every hour must land on Linux-verifiable work. `termios`+`poll` failures (raw mode not restored, `\r\n`, `Ctrl+C`) are observable only on a real tty, and they can iterate without a VM. macOS shares the code path, so W4's Mac is a second opinion, never a blocker. |
| **Why the scheduler goes here** | The scheduler *is* defined by the backend's timing primitive: one marquee tick is a bounded timed wait behind a condition variable, and the render deadline is capped by the same `polling_ms` that the backend's `readEvent` timeout uses. The owner of the timeout semantics owns the deadline logic and can `strace`/`time` both threads to prove neither busy-spins. |
| **Deliverables** | `terminal_posix.cpp`; `process.cpp`/`scheduler.cpp` (including the two-thread model, the mutex/cv discipline and `docs/threading-model.md`); `docs/measurements/linux-*.md`; PPT sections *Process representation* + *Scheduler implementation* |
| **Budget** | ~10 h (backend 3 h, PCB 1 h, threaded scheduler + its tests 4 h, measurement+docs 2 h) |

### 4.2 W2 — Byron (Linux + Windows)

**Primary (infrastructure):** contract plumbing, CMake/presets, CI matrix, build scripts, integration.
**Secondary (logic):** parameterization (config parser, CLI, clamping), interpreter, line editor, entry point.

| | |
| --- | --- |
| **Why this person** | They are the **only** member who can compile *and* run both a POSIX and a Win32 build, so they are the only one who can prove the `Terminal` abstraction abstracts anything — the classic failure is a contract that quietly assumes one platform. Integrator and build owner by capability, not seniority. |
| **Why parameterization goes here** | "No recompile during the quiz" is an *artifact* rule, not a feature rule: the owner of the build owns the launch path and the parameter file. Interpreter and parameters are one owner because `set_text`/`set_speed` mutate `Parameters` and `help` prints the command table; splitting them creates a two-person edit on one struct. |
| **Why the line editor is bundled** | Raw mode disables echo, so the app echoes keystrokes itself. That echo path is exactly what `--measure` reports as **input-to-echo processing latency**, and it *is* the interpreter's input path — one owner keeps that number accountable. (Perceived typing delay is a separate human judgement, §T1.4.) |
| **Deliverables** | `CMakeLists.txt`, `CMakePresets.json`, `.github/workflows/ci.yml`, `CONTRACTS.md`, `scripts/build.*`, `config/csopesy.ini`, `config_io.cpp`, `parameters.cpp`, `interpreter.cpp`, `line_editor.cpp`, `README.txt`, `src/main.cpp`, `console_app.cpp`; PPT *Command recognition* + *Command interpreter implementation* |
| **Budget** | ~11 h (build+CI 3 h, params/config 2.5 h, interpreter+line editor 3.5 h, integration 1 h, README 1 h) |

### 4.3 W3 — Nathan (Windows-only)

**Primary:** Win32 console backend `src/platform/terminal_win32.cpp`.
**Supporting:** the Windows rehearsal kit and the graded-run rehearsal.

| | |
| --- | --- |
| **Why this person** | Win32 console APIs are their only executable surface, and they will **run the graded quiz on Windows** — the runbook owner must own the backend under it. Backend + runbook in one head removes the worst quiz-day failure: a runbook step that does not match the binary's actual behavior on that machine. Backend and runbook are one deliverable. |
| **Specifically on their plate** | `_kbhit()`/`_getch()` polling with 0/224 arrow prefixes; `WaitForSingleObject` on the console input handle for the timeout (`_kbhit` cannot block); VT **output** explicitly enabled; VT **input** deliberately **not** enabled (mixing it with `_getch` changes key delivery into escape sequences); `GetConsoleScreenBufferInfo` for size + resize; **clearing `ENABLE_QUICK_EDIT_MODE`** (a stray click otherwise freezes the animation mid-quiz); verifying under both `cmd.exe` and Windows Terminal |
| **Deliverables** | `terminal_win32.cpp`; `scripts/rehearse_windows.bat`; `docs/measurements/windows-*.md`; rehearsal recordings; PPT *Console UI implementation* review on Windows |
| **Budget** | ~8 h (backend 4 h, VT/QuickEdit verification 2 h, kit 1 h, rehearsal 1 h) |

### 4.4 W4 — Kim (macOS + Windows)

**Primary (logic):** glyph table + render engine (`glyphs.cpp`, `renderer.cpp`, `frame_buffer.cpp`).
**Secondary:** the measurement harness and the PPT/video production.

| | |
| --- | --- |
| **Why this person** | Two OS families but **neither Linux** ⇒ safest on platform-independent code that behaves identically on both machines: pure rasterization, scroll math, frame diffing. It is also the piece the video depends on visually. |
| **Why measurement goes here** | The PPT must give recommended refresh/polling values *and* the tearing/delay limits for your hardware. Only W4 can produce two baselines personally, and their renderer is what creates the tearing being measured. One head owns "what we measured", "why it tears" and "how the fix works". |
| **Why video production goes here** | The video is dominated by rendering quality and timing. The owner of the visual output owns the visual evidence; W3 drives the keyboard for the Windows takes, W4 directs timing and packages the MP4 into the PPTX. |
| **macOS-specific value** | A second POSIX backend verification (optional): `Terminal.app` vs `iTerm2` resize/VT behavior, poll-on-tty semantics. Done as an *optional extra* because macOS is not in the handout's shell reference. |
| **Deliverables** | `glyphs.cpp`, `renderer.cpp`, `frame_buffer.cpp`, `tests/unit/test_glyphs.cpp`, `test_scroll.cpp`, `test_renderer.cpp`; `scripts/measure.sh`, `scripts/measure.ps1`; `docs/measurements/*`; the PPT; video direction |
| **Budget** | ~11 h (glyphs 3 h, renderer/diff 3 h, measurement 3 h, PPT+video 2 h) |

### 4.5 RACI, writer rules, freeze protocol

| Artifact | W1 Lorens | W2 Byron | W3 Nathan | W4 Kim |
| --- | --- | --- | --- | --- |
| `include/csopesy/*.hpp` contracts | C | **A/R** (freeze owner) | C | C |
| `platform/terminal_posix.cpp` | **R** | C | — | C (macOS review) |
| `platform/terminal_win32.cpp` | — | C | **R** | — |
| `CMakeLists`, CI, `scripts/build.*`, `CONTRACTS.md` | C | **R** | C | C |
| `parameters.*`, `config_io.*`, `csopesy.ini`, CLI | C | **R** | C | C |
| `interpreter.*`, `line_editor.*`, `main.cpp`, `console_app.cpp` | C | **R** | C | C |
| `glyphs.*`, `renderer.*`, `frame_buffer.*` | C | C | C | **R** |
| `process.*`, `scheduler.*`, `shutdown.hpp` impl per platform | **R** | C | **R** (Win32) | C |
| `docs/threading-model.md` + the §3.8 threading contract | **R** | **A/R** (contract freeze owner) | C | C (test-double review) |
| Windows rehearsal kit | — | C | **R** | C |
| measurement tables | **R** (Linux) | C | **R** (Windows) | **R** (macOS + synthesis) |
| PPT | **R** (2 sections) | **R** (2 sections) | C | **A/R** (2 sections + assembly) |
| Video | C | C | **R** (drives the Windows run) | **A/R** (direction, packaging) |

R = responsible, A = accountable, C = consulted. **One writer per file at any time.** Contracts change only by:
propose in chat → W2 + the affected owner agree → bump the `CONTRACTS.md` marker → announce. No silent header
edits after Phase 0: a mid-phase header change invalidates other members' builds on machines they cannot fix.

---

## 5. Task backlog

Every task that *can* carry a test is TDD: failing test → run it → implement → run it → commit. The stated
exceptions are the **platform-surface tasks** — T1.1 (`termios`/`poll`/`SIGWINCH`), T3.1 (Win32 console modes,
QuickEdit, VT), and T0.5's CLion terminal gate — whose real test is the hand-run checklist in the task, on the
owner's hardware, because no unit test can observe raw-mode restoration, a stray-click freeze, or a live
resize. For those, do the checklist and record its result in the PR; do not invent a test-shaped ritual to
satisfy the wording (§6.1 item 1, §6.3). `cmake --preset` names come from `CMakePresets.json` (§T0.2).

### 5.0 Dated schedule — submission Wednesday 2026-09-23 (7 days)

End-of-day gates, not aspirations. If a gate is red at midnight, apply the cut line the next morning rather
than carrying debt forward. Only D6 has float.

| Day | Date | Goal | End-of-day gate | Leads |
| --- | --- | --- | --- | --- |
| D0 | **Wed Sep 16** (today) | Freeze contracts; CI green on 3 OSes; CLion runs verified | T0.1–T0.5 (+T0.5b if needed) done; all four build, run and `ctest` locally; §9 emailed | Byron drives, all verify |
| D1 | **Thu Sep 17** | Core code behind passing unit tests | T4.1–T4.3 (Kim), T2.1–T2.3 (Byron), T1.2–T1.3 (Lorens — **T1.3 now carries the two-thread worker and is the critical path**), T3.1 (Nathan) green | all |
| D2 | **Fri Sep 18** | End-to-end app on Windows **and** Linux | typing during animation works on real terminals; T4.4/T4.5/T2.4/T2.5/T1.1/T1.4 done; professor answers in hand (or §9 defaults locked) | all |
| D3 | **Sat Sep 19** | Acceptance A1–A9 passes on the Windows build | T5.1 done; A9 double-path proof recorded on the frozen binary | Byron + Nathan |
| D4 | **Sun Sep 20** | Measurements + cross-OS matrix | T5.3 sweeps committed for all four machines; T5.2 matrix; T3.3 Windows sweep | Kim synthesizes |
| D5 | **Mon Sep 21** | PPT complete (6 sections) + README.txt | T6.1, T2.6 | all |
| D6 | **Tue Sep 22** | Freeze and record | T6.3 tag + SHA-256; T6.2 two rehearsals then graded takes; MP4 embedded in the PPTX; size checks pass | Nathan runs, Kim directs |
| D7 | **Wed Sep 23** | Submit | deliverables uploaded, links verified, backup copy on a second drive/cloud | Byron submits |

**Cut line (apply in order; never cut a spec requirement):**

1. P2 extras — pty replay harness, macOS manual verification pass.
2. `FrameBuffer` row diffing → single-write full repaint (still one frame per write, just more bytes).
3. `marquee_row` and `--measure` — measure with the default layout and report intervals from observation.
4. `ascii_art` plain mode (`--plain`) — if time forces a choice, the art marquee is the graded default.
5. `--diag` — verify the terminal manually with `stty -a` (POSIX) / the console properties (Windows).
6. **Last resort only —** the threaded marquee worker → the v2.3 single-threaded cooperative loop (the pure
   `tick()` contract is preserved precisely so this is a deletion, not a rewrite). This forfeits the *reason*
   for the change (matching the professor's example) and must be an explicit group decision, never a silent
   one: it is only defensible if the threaded design cannot be made to pass A8 on all four machines by D3.

**Never cut:** the six commands, terminal restore on exit, the config-file + CLI path for `text`/`refresh_ms`
(partial-credit insurance), the video constraints, README.txt's entry-file statement.

---

### Phase 0 — contracts and a green pipeline (Byron leads; Lorens/Nathan/Kim review)

#### T0.1 — Repository skeleton, layer rules, assertion harness

**Files:** `CMakeLists.txt`, `.gitignore`, `scripts/check_layers.sh`, `tests/support/{check.hpp,check.cpp}`,
all `include/csopesy/*.hpp` from §3 as declarations only, `src/**` stub `.cpp` with `// TODO(task)` bodies.

- [ ] **Step 1 — the harness and its own test**

```cpp
// tests/support/check.hpp
#pragma once
#include <algorithm>   // std::count  (v1's tests used it without this include)
#include <cstdio>
#include <deque>
#include <string>
#include <vector>
namespace ck {
struct Case { const char* name; void (*fn)(); };
inline std::vector<Case>& registry() { static std::vector<Case> r; return r; }
inline int failures = 0;
struct Reg { Reg(const char* n, void (*f)()) { registry().push_back({n, f}); } };
}
#define TEST(name) static void name(); static ck::Reg reg_##name(#name, name); static void name()
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); ++ck::failures; } } while (0)
#define CHECK_EQ(a, b) do { auto _a = (a); auto _b = (b); if (!(_a == _b)) { \
  std::printf("FAIL %s:%d  %s == %s\n", __FILE__, __LINE__, #a, #b); ++ck::failures; } } while (0)
#define CHECK_STR(a, b) do { std::string _a = (a); std::string _b = (b); if (_a != _b) { \
  std::printf("FAIL %s:%d\n  actual:   [%s]\n  expected: [%s]\n", __FILE__, __LINE__, _a.c_str(), _b.c_str()); \
  ++ck::failures; } } while (0)
```

```cpp
// tests/support/check.cpp
#include "check.hpp"
int main() {
  for (const auto& c : ck::registry()) { c.fn(); }
  if (ck::failures == 0) { std::printf("OK  %zu tests\n", ck::registry().size()); return 0; }
  std::printf("FAILED  %d assertion(s)\n", ck::failures);
  return 1;
}
```

Note on `CHECK_STR`: expected literals must reproduce trailing padding exactly, because every glyph cell is
fixed-width. The test file includes `<algorithm>`, `<deque>`, `<string>` here so no individual test has to.

- [ ] **Step 2 — the layer guard** (§3.1): `scripts/check_layers.sh` fails the build if a lower layer includes a
  higher one — no `#include "platform/..."` outside `platform/`, no `features/` header inside `entities/`, and
  no `features/` header inside a *different* `features/` slice. It is a ~15-line `grep`, and it exists because
  the `entities/scheduler.cpp` violation survived three review rounds of prose (§3.1, §10.8). Wiring it into
  CI is what makes §3.1's "enforced" true rather than aspirational.
- [ ] **Step 3 — run it, expect success with zero tests**
`cmake --preset debug && cmake --build build/debug && ./build/debug/csopesy_tests` → `OK  0 tests`
- [ ] **Step 4 — commit** `chore: repo skeleton, frozen contracts, zero-dep test harness, layer guard`

#### T0.2 — CMake with platform source selection + presets

**Files:** `CMakeLists.txt`, `CMakePresets.json`.

> **CMake floor is 3.21, not 3.20.** `CMakePresets.json` uses `"version": 3`, and per CMake's own
> `cmake-presets(7)` every feature at schema version 3 (`condition`, `installDir`, omitting `generator`/
> `binaryDir`) was *added in CMake 3.21*; the `Visual Studio 17 2022` generator was also added in 3.21. So
> `cmake_minimum_required`, the stated floor and the preset schema must all say **3.21**.

```cmake
cmake_minimum_required(VERSION 3.21)     # presets schema v3 + VS 17 2022 generator both require 3.21
project(csopesy_marquee CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Warning flags. Written as a plain if/else ON PURPOSE. The tempting one-liner
#   add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/W4>$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>)
# is split by CMake's argument parser at the space inside the generator expression: CMake *configures* cleanly
# and then fails at compile time with `c++: error: unrecognized command-line option '-Wextra>'` (confirmed on
# CMake 4.3 + GCC 16). Do not "simplify" this back into one line.
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra)
endif()

# std::thread needs -pthread on POSIX (GCC/Clang); Threads::Threads is CMake's portable answer and a no-op on
# MSVC. Without it the POSIX build fails to LINK with `undefined reference to pthread_create` — the compile
# succeeds, so this is only caught at link time on a machine you may not be holding.
find_package(Threads REQUIRED)

set(CORE_SOURCES
  src/app/console_app.cpp src/app/scheduler.cpp
  src/features/marquee/renderer.cpp src/features/marquee/glyphs.cpp
  src/features/commands/interpreter.cpp src/features/commands/line_editor.cpp
  src/entities/parameters.cpp src/entities/config_io.cpp
  src/entities/process.cpp
  src/shared/terminal/frame_buffer.cpp)

if(WIN32)
  list(APPEND CORE_SOURCES src/platform/terminal_win32.cpp)
else()
  list(APPEND CORE_SOURCES src/platform/terminal_posix.cpp)   # Linux + macOS
endif()

add_library(csopesy_core STATIC ${CORE_SOURCES})
target_include_directories(csopesy_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(csopesy_core PUBLIC Threads::Threads)   # PUBLIC: csopesy and csopesy_tests both link it

add_executable(csopesy src/main.cpp)
target_link_libraries(csopesy PRIVATE csopesy_core)

enable_testing()
add_executable(csopesy_tests
  tests/support/check.cpp tests/unit/test_glyphs.cpp tests/unit/test_scroll.cpp
  tests/unit/test_parameters.cpp tests/unit/test_config.cpp tests/unit/test_interpreter.cpp
  tests/unit/test_scheduler.cpp tests/unit/test_renderer.cpp)
target_include_directories(csopesy_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/support)
target_link_libraries(csopesy_tests PRIVATE csopesy_core)
add_test(NAME unit COMMAND csopesy_tests)
# The scheduler tests now drive a real worker thread. A missing join would hang them forever; a bounded timeout
# turns that hang into a red test instead of a stuck CI job. 60 s is ~100x the suite's real runtime.
set_tests_properties(unit PROPERTIES TIMEOUT 60)
# The real-binary end-to-end smoke test is registered in T2.5, alongside the plain line mode that makes it pass
# (a stub `main` cannot satisfy it here, and a test that stays red for two phases trains people to ignore CI).
```

```json
{
  "version": 3,
  "configurePresets": [
    { "name": "debug",      "generator": "Ninja", "binaryDir": "${sourceDir}/build/debug",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" } },
    { "name": "release",    "generator": "Ninja", "binaryDir": "${sourceDir}/build/release",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Release" } },
    { "name": "windows-vs", "generator": "Visual Studio 17 2022", "architecture": "x64",
      "binaryDir": "${sourceDir}/build/vs" }
  ]
}
```

Note: `FakeTerminal` is **not** in `CORE_SOURCES` — it is a header-only test double in `tests/support/`.

- [ ] **Step 1 — configure + build + test on every OS you have.**
Linux/macOS: `cmake --preset debug && cmake --build build/debug && ctest --test-dir build/debug --output-on-failure`
Windows: `cmake --preset windows-vs && cmake --build build/vs --config Debug && ctest --test-dir build/vs -C Debug --output-on-failure`
Expected: build succeeds; `100% tests passed, 0 tests failed out of 1` (the `unit` harness only — the
real-binary `smoke` test is registered in T2.5, once a plain line mode exists for it to test).
Also confirm the warning flags **really reach the compiler**: `cmake --build build/debug --verbose 2>&1 |
grep -E '(-Wall|-Wextra)'` on Linux/macOS, or `/W4` in the MSBuild log / generated `.vcxproj` on Windows. A
silently split option expression still configures cleanly and only fails at the compile step, on a machine you
may not be holding — that is the exact failure mode v2.3 fixes above, and this check is what catches it.
Also confirm the **threading link** is real: if `find_package(Threads)`/`Threads::Threads` were dropped, the
POSIX build fails at link time with `undefined reference to 'pthread_create'`, never at configure time, and
`std::thread` is in `CORE_SOURCES` from T1.3 onwards.
- [ ] **Step 2 — commit** `build: cmake with per-platform terminal source + presets`

#### T0.3 — CI matrix on all three OS families

**Files:** `.github/workflows/ci.yml`.

```yaml
name: ci
on: [push, pull_request]
jobs:
  build-and-test:
    strategy:
      fail-fast: false
      matrix: { os: [ubuntu-latest, windows-latest, macos-latest] }
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4          # no lukka/get-cmake: hosted runners already ship CMake >= 3.21
      - name: Layer guard
        run: bash scripts/check_layers.sh   # explicit `bash`: the Windows default shell is pwsh
      - name: Configure
        run: cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
      - name: Build
        run: cmake --build build --config Debug
      - name: Test
        run: ctest --test-dir build -C Debug --output-on-failure   # `unit` now; `smoke` joins it at T2.5
```

- [ ] **Step 1 — push a branch and confirm all three jobs are green** — the only proof that the Win32/POSIX
  split still compiles on platforms nobody here can hold.
- [ ] **Step 2 — commit** `ci: build+ctest on ubuntu, windows, macos`

#### T0.4 — Freeze marker

**Files:** `CONTRACTS.md`. Record `contracts v2.6 — 2026-09-16`, the header list it covers, and the §4.5 change
protocol. The headers that actually changed are `scheduler.hpp` (the two-thread contract, its v2.5
critical-section rules and the v2.6 `quit_`/`TickResult` narrowing, §3.8), `parameters.hpp` (`measurePath`,
§3.5) and `interpreter.hpp` (the `quit_` ownership note, §3.11), so those are re-frozen first.

- [ ] **Step 1** write it. **Step 2** commit `docs: freeze contracts v2.6`

#### T0.5 — CLion toolchains, presets, and the Run/Debug config (all four; Byron owns the record)

**Files:** `CMakePresets.json` (extend), `.idea/runConfigurations/csopesy-dev.xml`,
`.idea/runConfigurations/csopesy-quiz.xml`, `.gitignore` (add `CMakeUserPresets.json`, `build/`, `frozen/`,
`.idea/*` except `runConfigurations/`), `docs/clion-run-config.md`, `docs/clion-toolchain.md`.

> **Sequencing note.** `csopesy-quiz` points at `frozen/csopesy.exe`, which does not exist until T2.6/T6.3. So
> **T0.5 verifies the terminal behaviour only**, and the *frozen-binary, builds-nothing* property of
> `csopesy-quiz` is re-verified at T6.3 Step 0 once the artifact exists. Do not ask anyone to press Run on
> `csopesy-quiz` before then.

- [ ] **Step 1 — enable presets in CLion** (*Settings → Build, Execution, Deployment → CMake* → use CMake
  presets). Presets load read-only; local edits go to `CMakeUserPresets.json`.
- [ ] **Step 2 — pin the toolchain per member and record it**: Lorens: system GCC/clang + Ninja · Byron:
  GCC/Ninja on Linux, **MSVC** on Windows · Nathan: **MSVC** (documented pairing for *Run in external console*)
  · Kim: AppleClang + Ninja, MSVC on Windows. Write the compiler versions into `docs/clion-toolchain.md`.
- [ ] **Step 3 — commit both run configurations**:
  - `csopesy-dev`: CMake Application, target `csopesy`, working directory `$ProjectFileDir$`, program
    arguments `--config=config/csopesy.ini`.
  - `csopesy-quiz`: **Custom Build Application**, `Executable` =
    `$ProjectFileDir$/frozen/csopesy.exe`, the same working directory and program arguments, and a custom
    build target created with **all fields left empty** (per JetBrains' *Debug arbitrary executables*).
  - In both, verify **Before-launch** contains **no `Build` entry**. If the quiz configuration is instead a
    CMake Application, delete its default `Build` step — *"Build is the default pre-launch step for CMake
    applications"*, and a rebuild at Run time would silently invalidate the frozen artifact.
- [ ] **Step 4 — enable the terminal switch per OS**: Linux/macOS → *Emulate terminal in the output console*;
  Windows → *Emulate terminal in the output console* **with MSVC + LLDB** (documented supported) or
  *Run in external console (Windows)*. Either is acceptable — what matters is Step 5.
- [ ] **Step 5 — verification gate (run on each OS before Phase 1 starts).** Using **`csopesy-dev`**, press
  Run/Debug and confirm: (a) ANSI output renders as layout, not literal escape codes; (b) `start_marquee`
  animates **while** the prompt accepts typing; (c) `--diag` reports `isTty=true` (POSIX) / VT enabled
  (Windows); (d) resizing the window mid-animation repaints cleanly (both backends do this by re-reading
  `Terminal::size()` every tick — §3.8 — not by a platform resize event). Then, **without running it**,
  confirm **`csopesy-quiz` has no `Build` in Before-launch**. If a terminal check fails, try the other switch,
  then the T0.5b fallback.
- [ ] **Step 6 — record the outcome** in `docs/clion-run-config.md`: OS × CLion version × toolchain × option
  chosen × observed result × who verified, **plus the `csopesy-quiz` Before-launch list**. This file is the
  evidence that the graded run configuration is real.
- [ ] **Step 7 — commit** `chore(clion): presets, pinned toolchains, verified Run/Debug config`

#### T0.5b — Fallback: real terminal, launched by Run/Debug (only if T0.5 Step 5 fails)

**Files:** `scripts/run_external.sh`, `scripts/run_external.bat`.

- [ ] **Step 1** the script launches the **already-built** binary in a real terminal — Linux:
  `gnome-terminal -- ./build/release/csopesy --config=config/csopesy.ini` (or `tmux new-window`); macOS:
  `open -a Terminal` with a command file; Windows: `start "" cmd /k build\release\csopesy.exe --config=config\csopesy.ini`
  or `wt.exe`.
- [ ] **Step 2** point the CLion run config at the script instead of the binary. The video then shows the
  Run/Debug press plus the real console window — still handout-compliant, because the IDE initializes the
  program. **This is the only role the script has; it is not an evidence shortcut** (§7).
- [ ] **Step 3 — commit** `chore(clion): external-terminal launcher fallback for the graded run config`

---

### Phase 1A — W4 (Kim): glyphs, scroll math, renderer (pure logic, no platform)

#### T4.1 — Glyph table + block-text rasterizer

**Files:** `include/csopesy/glyphs.hpp`, `src/features/marquee/glyphs.cpp`, `tests/unit/test_glyphs.cpp`.

> **v2 fixes here (both v1 assertions were wrong — see `REVIEW_ADJUDICATION.md` M1):** `"C S 0"` is
> **5 cells + 4 gaps = 29** columns, not 17; and `rows[0]` for `"AB"` is `" ###  #### "` — **11** chars
> *including* the trailing cell pad, not the 10-char literal v1 wrote. A glyph-width invariant test is added
> so all ~45 hand-typed cells are checked instead of two of them.

- [ ] **Step 1 — write the failing test**

```cpp
// tests/unit/test_glyphs.cpp
#include "check.hpp"
#include <cstring>
#include "csopesy/glyphs.hpp"
using namespace csopesy;

TEST(glyph_A_is_five_rows) {
  const Glyph* g = glyphFor('A');
  CHECK(g != nullptr);
  CHECK_STR(g ? g->rows[0] : "", " ### ");
  CHECK_STR(g ? g->rows[2] : "", "#####");
}

TEST(lowercase_maps_to_uppercase) { CHECK(glyphFor('a') == glyphFor('A')); }

TEST(unknown_char_falls_back_to_box_never_null) {
  const Glyph* g = glyphFor('\x01');
  CHECK(g != nullptr);                 // never nullptr -> the renderer can never crash
  CHECK_STR(g->rows[0], "+---+");
}

// The test that makes a hand-typed font safe: ragged cells are the #1 way the scroll math breaks.
TEST(every_glyph_cell_is_exactly_five_columns) {
  for (int c = 32; c < 127; ++c) {
    const Glyph* g = glyphFor(static_cast<char>(c));
    CHECK(g != nullptr);
    for (int r = 0; r < kCellRows; ++r) { CHECK_EQ(std::strlen(g->rows[r]), size_t{kCellCols}); }
  }
}

TEST(block_text_has_five_rows_and_one_column_gaps) {
  const auto rows = renderBlockText("AB");
  CHECK_EQ(rows.size(), size_t{5});
  CHECK_EQ(rows[0].size(), size_t{11});        // 5 + gap + 5
  CHECK_STR(rows[0], " ###  #### ");           // NOTE: trailing pad is part of the cell
}

TEST(block_text_width_accounts_for_every_character) {
  CHECK_EQ(renderBlockText("CS0")[0].size(),   size_t{17});   // 3 cells + 2 gaps
  CHECK_EQ(renderBlockText("C S 0")[0].size(), size_t{29});   // 5 cells + 4 gaps  (v1 said 17)
  CHECK_EQ(renderBlockText("")[0].size(),      size_t{kCellCols});
}
```

- [ ] **Step 2 — run it, expect FAIL** — `glyphFor`/`renderBlockText` unimplemented (link error or stub).
- [ ] **Step 3 — implement**

```cpp
// src/features/marquee/glyphs.cpp
#include "csopesy/glyphs.hpp"
#include <cctype>
#include <unordered_map>
namespace csopesy {
namespace {
const Glyph kA{{{" ### "}, {"#   #"}, {"#####"}, {"#   #"}, {"#   #"}}};
const Glyph kB{{{"#### "}, {"#   #"}, {"#### "}, {"#   #"}, {"#### "}}};
// ... C..Z, 0..9, space, and the punctuation the quiz can realistically type
const Glyph kSpace{{{"     "}, {"     "}, {"     "}, {"     "}, {"     "}}};
const Glyph kBox{{{"+---+"}, {"|   |"}, {"|   |"}, {"|   |"}, {"+---+"}}};

const std::unordered_map<char, const Glyph*>& table() {
  static const std::unordered_map<char, const Glyph*> t = {
    {'A', &kA}, {'B', &kB}, /* ... */ {' ', &kSpace}
  };
  return t;
}
}

const Glyph* glyphFor(char c) {
  const char up = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  const auto it = table().find(up);
  return it == table().end() ? &kBox : it->second;
}

std::vector<std::string> renderBlockText(std::string_view text) {
  if (text.empty()) { text = " "; }     // guarantees a non-zero art width
  std::vector<std::string> rows(kCellRows);
  for (size_t i = 0; i < text.size(); ++i) {
    const Glyph* g = glyphFor(text[i]);
    for (int r = 0; r < kCellRows; ++r) {
      if (i != 0) { rows[static_cast<size_t>(r)] += ' '; }
      rows[static_cast<size_t>(r)] += g->rows[r];
    }
  }
  return rows;
}
}
```

- [ ] **Step 4 — run it, expect PASS** → `OK  5 tests`
- [ ] **Step 5 — commit** `feat(marquee): block glyph table (fixed 5x5 cells) + rasterizer with width invariant`

#### T4.2 — Scroll math

**Files:** `src/features/marquee/renderer.cpp` (partial), `tests/unit/test_scroll.cpp`.
*(v1's tests here were already correct; kept verbatim except the removed `stepCols` argument.)*

- [ ] **Step 1 — write the failing test**

```cpp
// tests/unit/test_scroll.cpp
#include "check.hpp"
#include "csopesy/renderer.hpp"
using namespace csopesy;

TEST(offset_starts_at_zero)                     { CHECK_EQ(scrollOffset(0,  30, 20), 0); }
TEST(offset_advances_one_column_per_frame)      { CHECK_EQ(scrollOffset(5,  30, 20), 5); }
TEST(offset_wraps_after_art_plus_band) {           // art must fully exit before re-entry
  CHECK_EQ(scrollOffset(50, 30, 20), 0);           // 30 + 20 == 50
  CHECK_EQ(scrollOffset(55, 30, 20), 5);
}
TEST(sliceRow_pads_right_inside_art)            { CHECK_STR(sliceRow("ABCDE", 4, 1), "BCDE"); }
TEST(sliceRow_pads_when_offset_exceeds_art) {
  CHECK_STR(sliceRow("ABCDE", 4, 7), "    ");      // fully scrolled off -> blank band
  CHECK_STR(sliceRow("ABCDE", 4, 3), "DE  ");      // partially off -> right-padded
}
TEST(sliceRow_always_returns_exact_band_width) {
  for (int off = 0; off < 25; ++off) { CHECK_EQ(sliceRow("ABCDE", 7, off).size(), size_t{7}); }
}
```

- [ ] **Step 2 — expect FAIL.** **Step 3 — implement:**

```cpp
int scrollOffset(long long cycles, int artWidth, int bandWidth) {
  const int period = artWidth + bandWidth;             // blank band between passes
  if (period <= 0) { return 0; }
  const long long raw = cycles;                        // one column per rendered frame
  return static_cast<int>(((raw % period) + period) % period);
}

std::string sliceRow(std::string_view artRow, int bandWidth, int offset) {
  std::string out;
  out.reserve(static_cast<size_t>(bandWidth));
  for (int i = 0; i < bandWidth; ++i) {
    const int idx = offset + i;
    out += (idx >= 0 && idx < static_cast<int>(artRow.size())) ? artRow[static_cast<size_t>(idx)] : ' ';
  }
  return out;
}
```

- [ ] **Step 4 — expect PASS. Step 5 — commit** `feat(marquee): wrapping scroll offset + exact-width row slicing`

#### T4.3 — FrameBuffer with one assembled frame per write

**Files:** `include/csopesy/frame_buffer.hpp`, `src/shared/terminal/frame_buffer.cpp`,
`tests/unit/test_renderer.cpp`.

- [ ] **Step 1 — write the failing test**

```cpp
// tests/unit/test_renderer.cpp
#include "check.hpp"
#include <algorithm>          // std::count (already pulled in by support/check.hpp)
#include "csopesy/frame_buffer.hpp"
using namespace csopesy;

TEST(first_frame_contains_every_row_once) {
  FrameBuffer fb; fb.resize(3, 5);
  for (int r = 1; r <= 3; ++r) { fb.putRow(r, 1, "     "); }
  fb.putRow(2, 1, "HELLO");
  const std::string frame = fb.renderDiff();
  CHECK(frame.find("HELLO") != std::string::npos);
  CHECK_EQ(std::count(frame.begin(), frame.end(), 'H'), 1);
}

TEST(unchanged_rows_are_not_repainted) {
  FrameBuffer fb; fb.resize(2, 5);
  fb.putRow(1, 1, "AAAAA"); fb.putRow(2, 1, "BBBBB");
  (void)fb.renderDiff();                                // flush the full frame once
  fb.putRow(2, 1, "CCCCC");
  const std::string delta = fb.renderDiff();
  CHECK(delta.find("AAAAA") == std::string::npos);       // header untouched
  CHECK(delta.find("CCCCC") != std::string::npos);
}

TEST(resize_invalidates_everything) {
  FrameBuffer fb; fb.resize(2, 5); fb.putRow(1, 1, "AAAAA"); (void)fb.renderDiff();
  fb.resize(2, 6);                                       // terminal got wider
  const std::string frame = fb.renderDiff();
  CHECK(frame.find("AAAAA") != std::string::npos);        // full repaint required
}
```

- [ ] **Step 2 — expect FAIL.**
- [ ] **Step 3 — implement** `FrameBuffer` as `std::vector<std::string>` rows + a dirty-row bitset +
  `resize()` setting `forceFullRepaint_`. `renderDiff()` builds **one** `std::string` of
  `\x1b[<r>;<c>H` + row text per dirty row, clears the flags, and parks the cursor at the end of the prompt
  buffer. Callers issue exactly one `write()` per returned string.
- [ ] **Step 4 — expect PASS. Step 5 — commit** `feat(ui): diffing frame buffer, one assembled frame per write`

#### T4.4 — Marquee band composition (art or plain)

**Files:** `src/features/marquee/renderer.cpp`, `tests/unit/test_renderer.cpp` (extend).

- [ ] **Step 1 — failing test**: `Renderer::composeBand(params, proc)` returns exactly `kArtRows` rows when
  `params.asciiArt` (one row otherwise), each exactly `params`-derived terminal width, and
  `artWidthFor("CSOPESY") == 7*5 + 6 == 41`.
- [ ] **Step 2–4:** implement `Renderer::drawFrame(...)` (band + header + prompt + message); run until PASS.
- [ ] **Step 5 — commit** `feat(marquee): band composition for art and plain modes`

#### T4.5 — Header region (handout fidelity — §1.4)

**Files:** `src/features/marquee/renderer.cpp`.

- [ ] **Step 1 — failing test**: the header draws, verbatim:

```text
Welcome to CSOPESY!
```

Note the capital `W` — v1 asserted `welcome` (see `REVIEW_ADJUDICATION.md` M2). After that line come a blank
line, `Group developer:` and one row per `Parameters::developers` entry, a blank line, then
`Version date: <versionDate>` — and with an empty `versionDate` the line is exactly `Version date:` with no
trailing space.

- [ ] **Step 2–5:** implement, PASS, commit `feat(ui): handout-faithful header region`. The prompt line is
  laid out on **one fixed row**; a buffer wider than the row scrolls horizontally (§3.11) and never wraps onto
  the band rows.

---

### Phase 1B — W1 (Lorens): POSIX backend + process/scheduler

#### T1.1 — PosixTerminal: raw mode, sizing, restore, signals

**Files:** `src/platform/terminal_posix.cpp` (also defines `installShutdownHandlers`).

- [ ] **Step 1 — write the test double first** (the backend itself needs a tty, so it is verified by hand):

```cpp
// tests/support/fake_terminal.hpp
#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "csopesy/terminal.hpp"
namespace csopesy {
// Public fields are read directly by the single-threaded pure-step tests (no worker exists there, so nothing
// can race). The threaded tests in T1.3 use push()/outCopy()/waitFor*() instead, and assert shared scheduler
// state only through `Scheduler::snapshot()` or after `join()`.
//
// v2.4: this is the ONE Terminal implementation that had to change for threading (see §3.3) — it is
// in-process state rather than two OS handles, so the two-thread read/write split has to be made explicitly
// safe here. `write()` and `size()` are called by the marquee thread while the test thread is the input thread.
class FakeTerminal : public Terminal {
 public:
  std::atomic<Size> sz{Size{24, 80}};
  std::atomic<long long> clock{0};
  std::deque<KeyEvent> events;         // test thread only; use push() when a worker may be running
  std::string out;                     // appended under m_ by the marquee thread; read it with outCopy()
  bool rawEntered = false, restored = false;

  void enterRawMode() override { rawEntered = true; }
  void restore() override { restored = true; }
  // size() is called exactly once per Scheduler::tick(), and it is the first thing tick() does — so a count of
  // size() calls is a precise "the worker reached tick n" barrier. No sleeps are needed anywhere.
  Size size() const override {
    std::lock_guard<std::mutex> lk(m_);
    ++sizeCalls_;
    sizeCv_.notify_all();
    return sz.load();
  }
  // No input source of its own: a test double cannot block on a console, so an empty queue is an immediate
  // "timeout" (false). The threaded tests ARE the input thread and call this synchronously (`readEvent(ev, 0)`
  // is a poll under any implementation) — which is why the whole suite is free of std::this_thread::sleep_for.
  bool readEvent(KeyEvent& e, int) override {
    std::lock_guard<std::mutex> lk(m_);
    if (events.empty()) { return false; }
    e = events.front(); events.pop_front(); return true;
  }
  void write(std::string_view b) override {
    std::lock_guard<std::mutex> lk(m_);
    out.append(b);
    writes_.emplace_back(b);
    writers_.push_back(std::this_thread::get_id());
    ++frames_;
    cv_.notify_all();
  }
  void flush() override {}
  bool isTty() const override { return false; }
  long long nowMs() const override { return clock.load(); }

  // --- explicit synchronization for the threaded tests; no sleeps, no flakiness ---
  void push(KeyEvent e) { { std::lock_guard<std::mutex> lk(m_); events.push_back(e); } }
  // Returns once tick #n has STARTED, i.e. ticks 1..n-1 have completed. tick() calls size() as its first
  // action (and, since v2.5, the action *before* mu_ — §3.8 rule 3), so waiting for n+1 is what proves tick n
  // finished; the following snapshot() then
  // also blocks until tick n+1 releases mu_, giving a deterministic barrier.
  bool waitForTickStarted(size_t n, int ms) {
    std::unique_lock<std::mutex> lk(m_);
    return sizeCv_.wait_for(lk, std::chrono::milliseconds(ms), [&] { return sizeCalls_ >= n; });
  }
  bool waitForFrames(size_t n, int ms) {
    std::unique_lock<std::mutex> lk(m_);
    return cv_.wait_for(lk, std::chrono::milliseconds(ms), [&] { return frames_ >= n; });
  }
  // Lock order note: `m_` is a LEAF lock. The worker acquires mu_ and then m_ (tick -> size -> write), and
  // nothing ever acquires m_ and then mu_, so mu_ -> m_ is a total order with no cycle. In production this is
  // moot: the real backends' size()/write() take no lock at all (§3.3).
  std::string outCopy() const          { std::lock_guard<std::mutex> lk(m_); return out; }
  size_t frameCount() const            { std::lock_guard<std::mutex> lk(m_); return frames_; }
  std::vector<std::string> writesCopy() const      { std::lock_guard<std::mutex> lk(m_); return writes_; }
  std::vector<std::thread::id> writersCopy() const { std::lock_guard<std::mutex> lk(m_); return writers_; }
 private:
  mutable std::mutex m_;
  std::condition_variable cv_, sizeCv_;
  std::vector<std::string> writes_;
  std::vector<std::thread::id> writers_;
  size_t frames_ = 0, sizeCalls_ = 0;
};
}
```

The `waitFor*` timeouts guard against a *hang* (a missing join, a lost notify); they are never used as a
clock. That distinction is what keeps the suite deterministic instead of flaky (§6.1).

- [ ] **Step 2 — manual checklist for `terminal_posix.cpp`** (results recorded in the PR):
  - `enterRawMode()` clears `ICANON|ECHO|ISIG` (`ISIG` off ⇒ we handle `0x03` ourselves), sets `VMIN=0,VTIME=0`.
  - Newlines are emitted as `\r\n` (raw mode disables `ONLCR`).
  - `size()` uses `ioctl(STDOUT_FILENO, TIOCGWINSZ)` and falls back to `24x80`.
  - **No resize event is required** (v2.2): `Scheduler::tick` re-reads `size()` every tick (now on the marquee
    thread) and repaints on a change (§3.8), so `readEvent` needs no resize path — and do **not** add
    `SIGWINCH` handling for correctness. (Optional: a `SIGWINCH` handler that only sets a `volatile sig_atomic_t` ends the wait
    immediately instead of at the next tick; `pollTimeoutMs()` is ≤ 10 ms by default. Why the old
    `SIGWINCH`/`Resize` design was deleted rather than patched: `REVIEW_ADJUDICATION.md` §6.3 and §7.3.)
  - `readEvent(out,t)` is `poll(&pfd,1,t)`. On **timeout** → `false`. On **`EINTR`** → return `false` (a
    shutdown signal still exits promptly: the *input thread* polls `shutdownRequested()` after every
    `readEvent`, and it is the only reader of that flag — §3.8, §3.10). Maps
    `0x0D`→`Enter`, `0x7F`/`0x08`→`Backspace`,
    `0x1B`+`[`+letter→arrows (5 ms follow-up deadline so a bare `Esc` is not lost), `0x04`→`Eof` at line start,
    `Tab`→`Tab`.
  - **Call site:** `enterRawMode()` is invoked by the RAII guard constructed in `main` (T2.5), skipped when
    `noTty` / `!isTty()`; `restore()` runs from the guard's destructor (plus `atexit` as a backstop).
  - `Ctrl+C` (`0x03`) → `Eof`/quit request.
  - **Threading (v2.4, refined in v2.5):** `readEvent` is called from the input thread only; `write`/`flush`
    are called from the marquee thread only, and `size()` is read by that thread **before** it takes `mu_`
    (§3.3, §3.8 rules 2–3). POSIX already reads fd 0 and writes fd 1, so **no code change**
    is needed for that split — and do **not** add a mutex around terminal I/O to "make it safe": the ownership
    table in §3.8 is what makes it safe, and a lock here would only add contention to a slow console write.
  - **Signal safety:** `installShutdownHandlers()` installs a handler that only does
    `g_flag = 1`. `restore()` is called from normal scope (RAII/`atexit`) — **never from the handler**.
    `SIGKILL` cannot be caught; document that.
- [ ] **Step 3 — implement and verify by hand**: `./build/debug/csopesy`, type during animation, then `exit`,
  `Ctrl+C`, and `kill -TERM`; after each, `stty -a | head` must show normal echo/canonical mode again. Also
  confirm the threaded shutdown: after `exit` the process must terminate promptly (the join completed) and no
  worker thread may linger — `ps -T -p <pid>` / `top -H` shows a single thread once the program has exited,
  and the program must not hang waiting on a join that never returns.
- [ ] **Step 4 — commit** `feat(posix): raw-mode backend, live sizing, flag-only signal handling`

#### T1.2 — MarqueeProcess PCB

**Files:** `include/csopesy/process.hpp`, `src/entities/process.cpp`.

- [ ] **Step 1 — failing test** — `start()` from `Stopped` → `Running` and returns `true`; from `Running` →
  `false`; `stop()` from `Running` → `Stopped`/`true`; from `Stopped` → `false`; `cycles` starts at 0;
  `hasRendered` starts `false`, becomes `true` after the first render, and `start()` clears it again.
  (No `Ready`/`Finished`/`quantumMs`/`asciiArt` — see §3.8.)
- [ ] **Step 2–4:** implement, PASS. **Step 5 — commit** `feat(entity): marquee PCB with Stopped/Running states`

#### T1.3 — Threaded scheduler: marquee worker + preserved pure step (the PPT's "scheduler implementation")

**Files:** `include/csopesy/scheduler.hpp`, `src/app/scheduler.cpp`, `tests/unit/test_scheduler.cpp`,
`tests/support/fake_terminal.hpp`, `include/csopesy/parameters.hpp` (`measurePath`, T2.2),
`docs/threading-model.md`.

> **v2 rewrite.** v1's `runOnce()` both read input *and* was called by a loop that read input (double poll),
> and its `typed_command_is_executed_during_animation` test could never pass (one event per tick). Here
> `tick()` is pure and the test drives the loop itself.
>
> **v2.4 rewrite — this is the architecture task.** The loop moved into a worker thread (`run()`), the input
> side moved into `ConsoleApp::run()` (T2.5), and the two are joined by `mu_`/`cv_` exactly as §3.8 specifies.
> The **pure-step tests in Step 1 stay exactly as written and keep passing** — that is the whole reason
> `tick()` was left pure, so the threading change *adds* tests instead of invalidating them. Step 1b adds the
> concurrency tests, Steps 2–4 build the worker, Step 6 writes the one-page explanation for the PPT.
>
> **v2.5 refinement.** Two gaps the threading reviews found are closed *here*, in the tests: Step 1b gains the
> one test with a **second real thread** (the v2.4 block only ever drove input from the test thread, which
> `gpt-v5.md` §3 correctly called "not a concurrency test"), and Step 1c pins the corrected `--measure`
> contract. Neither adds a sleep, and Step 1's pure-step block stays byte-for-byte unchanged.

- [ ] **Step 1 — write the failing test**

> The `tick(&ev)` calls below are the **only** production-shaped use of the test-only overload (§3.8): they
> exist to keep v2.3's pure-step suite byte-for-byte. No production path ever passes a non-null event.

```cpp
// tests/unit/test_scheduler.cpp
#include "check.hpp"
#include "fake_terminal.hpp"
#include "csopesy/scheduler.hpp"
#include "csopesy/interpreter.hpp"
#include "csopesy/renderer.hpp"
using namespace csopesy;

namespace {
struct Harness {                       // declaration order == wiring order
  FakeTerminal term;
  Parameters params;
  MarqueeProcess proc;
  Renderer renderer;
  Interpreter interp{params, proc};    // v1 default-constructed this and could not compile
  Scheduler sched{term, params, renderer, interp, proc, [this] { return term.clock; }};
};
}

TEST(no_render_while_stopped) {
  Harness h; h.term.clock = 1000;
  (void)h.sched.tick(nullptr);
  CHECK_EQ(h.proc.cycles, 0);
}

TEST(render_respects_refresh_interval) {
  Harness h; h.proc.state = ProcessState::Running; h.params.refreshMs = 100;
  h.term.clock = 0;   (void)h.sched.tick(nullptr); CHECK_EQ(h.proc.cycles, 1);  // first frame immediately
  h.term.clock = 50;  (void)h.sched.tick(nullptr); CHECK_EQ(h.proc.cycles, 1);  // too early
  h.term.clock = 100; (void)h.sched.tick(nullptr); CHECK_EQ(h.proc.cycles, 2);
  h.term.clock = 250; (void)h.sched.tick(nullptr); CHECK_EQ(h.proc.cycles, 3);
}

TEST(set_speed_moves_the_deadline_without_recompiling) {
  Harness h; h.proc.state = ProcessState::Running; h.params.refreshMs = 1000;
  h.term.clock = 0; (void)h.sched.tick(nullptr);
  h.params.setRefresh(50);                                   // == `set_speed 50`
  h.term.clock = 60; (void)h.sched.tick(nullptr);
  CHECK_EQ(h.proc.cycles, 2);
}

TEST(start_and_restart_render_immediately) {   // the `hasRendered` rule (§3.8)
  Harness h; h.params.refreshMs = 500;
  CHECK(h.proc.start());
  h.term.clock = 0; (void)h.sched.tick(nullptr);      CHECK_EQ(h.proc.cycles, 1);
  CHECK(h.proc.stop());
  h.term.clock = 10000; (void)h.sched.tick(nullptr);  CHECK_EQ(h.proc.cycles, 1);  // stopped: no render
  CHECK(h.proc.start());
  (void)h.sched.tick(nullptr);                        CHECK_EQ(h.proc.cycles, 2);  // immediate again
}

TEST(size_change_forces_a_repaint_without_any_event) {   // the FrameBuffer-resize gap (v2 never resized it)
  Harness h; h.proc.state = ProcessState::Running; h.params.refreshMs = 100000;
  h.term.clock = 0; (void)h.sched.tick(nullptr);
  const size_t before = h.term.out.size();
  h.term.sz = Size{30, 100};                   // the terminal got bigger; NO key, NO event (the Windows case)
  h.term.clock = 5;
  (void)h.sched.tick(nullptr);
  CHECK(h.term.out.size() > before);           // repainted at the new size, without waiting for refreshMs
}

TEST(poll_timeout_is_capped_by_the_render_deadline) {   // pollingMs cannot delay a frame (§3.8)
  Harness h; h.proc.state = ProcessState::Running; h.params.refreshMs = 30; h.params.pollingMs = 1000;
  h.term.clock = 0; (void)h.sched.tick(nullptr);          // renders; lastRenderMs = 0
  h.term.clock = 10;
  CHECK(h.sched.pollTimeoutMs() <= 20);                   // ~msUntilNextRender, NOT 1000
}

TEST(poll_timeout_is_never_zero_and_never_exceeds_polling) {
  Harness h; h.params.pollingMs = 10; h.params.refreshMs = 100; h.proc.state = ProcessState::Running;
  h.term.clock = 0; (void)h.sched.tick(nullptr);
  for (int t = 0; t <= 100; t += 7) {
    h.term.clock = t;
    CHECK(h.sched.pollTimeoutMs() >= 1);
    CHECK(h.sched.pollTimeoutMs() <= h.params.pollingMs);
  }
}

TEST(tick_never_reads_input_itself) {          // the double-poll regression guard
  Harness h; h.proc.state = ProcessState::Running;
  h.term.events.push_back(KeyEvent{KeyType::Char, 'x'});
  (void)h.sched.tick(nullptr);
  CHECK_EQ(h.term.events.size(), size_t{1});   // tick consumed nothing
}

TEST(echo_redraw_is_not_gated_by_refresh) {    // typing stays responsive at refresh_ms = 10000
  Harness h; h.proc.state = ProcessState::Running; h.params.refreshMs = 10000;
  h.term.clock = 0; (void)h.sched.tick(nullptr);
  const size_t afterFirstFrame = h.term.out.size();
  h.term.clock = 5;
  const KeyEvent ev{KeyType::Char, 'x'};
  (void)h.sched.tick(&ev);
  CHECK(h.term.out.size() > afterFirstFrame);  // a frame was emitted for the keystroke
}

TEST(typed_command_is_executed_during_animation) {
  Harness h; h.proc.state = ProcessState::Running;
  for (char c : std::string("stop_marquee\n")) { h.term.events.push_back(KeyEvent{KeyType::Char, c}); }
  h.term.events.back() = KeyEvent{KeyType::Enter, 0};
  for (int i = 0; i < 40 && h.proc.state != ProcessState::Stopped; ++i) {
    h.term.clock += 10;
    KeyEvent ev;                                   // the LOOP owns input; tick() is pure
    const bool have = h.term.readEvent(ev, 0);
    (void)h.sched.tick(have ? &ev : nullptr);
  }
  CHECK(h.proc.state == ProcessState::Stopped);
}
```

- [ ] **Step 1b — write the failing concurrency tests** (same file, appended). These are the tests the
  threading change *adds*; the Step 1 block above is untouched. They cover requirements 1–9 and 11 of
  `threading.md`.

  The two helpers the whole section is built on: the test thread **plays the input thread**
  (`readEvent` → `postEvent`), and every wait is a `condition_variable` wait on a counter the test double
  already has. **Not one `std::this_thread::sleep_for` appears in this section** — that is the flakiness
  requirement, and it is met by construction rather than by tuning a sleep.

```cpp
// tests/unit/test_scheduler.cpp — appended section: the THREADED model (v2.4; Step 1c added in v2.5)
//
// Waits are synchronization barriers, not timers:
//   waitForFrames(n)      == "n frames have been written"
//   waitForTickStarted(n) == "the worker has reached tick n" (tick() calls size() exactly once, first);
//                            therefore ticks 1..n-1 have completed
// Assertions about live shared state go through Scheduler::snapshot() (which locks mu_), or are made after
// join(). The `kHangGuardMs` timeouts exist only to turn a missing join / lost notify into a failed CHECK
// instead of a hung job — they are never part of the synchronization.
namespace {
constexpr int kHangGuardMs = 5000;

void drainInput(FakeTerminal& t, Scheduler& s) {      // this thread IS the input thread
  KeyEvent ev;
  while (t.readEvent(ev, 0)) { s.postEvent(ev); }     // readEvent(ev, 0) == poll, in any implementation
}
void typeLine(FakeTerminal& t, Scheduler& s, const std::string& line) {
  for (char c : line) { t.push(KeyEvent{KeyType::Char, c}); }
  t.push(KeyEvent{KeyType::Enter, 0});
  drainInput(t, s);
}
}

TEST(threaded_worker_renders_immediately_then_only_on_the_refresh_deadline) {   // req. 5, 10
  Harness h; h.params.refreshMs = 100; h.params.pollingMs = 1000;
  CHECK(h.proc.start());
  h.sched.start();
  CHECK(h.term.waitForTickStarted(1, kHangGuardMs));
  CHECK_EQ(h.sched.snapshot().cycles, 1);              // !hasRendered => the first frame is immediate
  h.term.clock = 50;  h.sched.wake();                  // explicit re-evaluate; no real-time wait
  CHECK(h.term.waitForTickStarted(2, kHangGuardMs));   // tick 1 finished, tick 2 running
  CHECK_EQ(h.sched.snapshot().cycles, 1);              // too early: no render
  h.term.clock = 100; h.sched.wake();
  CHECK(h.term.waitForTickStarted(3, kHangGuardMs));
  CHECK_EQ(h.sched.snapshot().cycles, 2);              // past the deadline: rendered
  h.sched.requestStop(); h.sched.join();
}

TEST(threaded_marquee_keeps_animating_across_refresh_deadlines) {               // req. 1
  Harness h; h.params.refreshMs = 100; h.params.pollingMs = 1000;
  CHECK(h.proc.start());
  h.sched.start();
  CHECK(h.term.waitForTickStarted(1, kHangGuardMs));
  size_t n = 1;
  for (int i = 1; i <= 3; ++i) {                       // three successive deadlines
    h.term.clock = i * 100;
    h.sched.wake();
    CHECK(h.term.waitForTickStarted(++n, kHangGuardMs));
  }
  CHECK_EQ(h.sched.snapshot().cycles, 4);              // 1 immediate + 3 deadline frames
  CHECK_EQ(h.term.frameCount(), size_t{4});
  h.sched.requestStop(); h.sched.join();
}

TEST(threaded_command_is_accepted_and_executed_while_animating) {              // req. 1, 2
  Harness h; h.params.refreshMs = 1; h.params.pollingMs = 1000;
  CHECK(h.proc.start());
  h.sched.start();
  CHECK(h.term.waitForTickStarted(1, kHangGuardMs));
  typeLine(h.term, "stop_marquee", h.sched);           // typed while the band is animating
  h.sched.wake();
  CHECK(h.term.waitForTickStarted(2, kHangGuardMs));
  CHECK(h.sched.snapshot().state == ProcessState::Stopped);   // the command was interpreted
  CHECK(h.term.frameCount() >= size_t{2});             // and frames kept coming while it was typed
  h.sched.requestStop(); h.sched.join();
}

TEST(threaded_start_marquee_restarts_and_renders_immediately) {                // req. 3
  Harness h; h.params.refreshMs = 500; h.params.pollingMs = 1000;
  h.sched.start();
  CHECK(h.term.waitForTickStarted(1, kHangGuardMs));
  typeLine(h.term, "start_marquee", h.sched); h.sched.wake();
  CHECK(h.term.waitForTickStarted(2, kHangGuardMs));
  CHECK_EQ(h.sched.snapshot().cycles, 1);              // start() cleared hasRendered => immediate frame
  typeLine(h.term, "stop_marquee", h.sched); h.sched.wake();
  CHECK(h.term.waitForTickStarted(3, kHangGuardMs));
  CHECK_EQ(h.sched.snapshot().cycles, 1);              // stopped: no further render
  h.term.clock = 10000; h.sched.wake();
  CHECK(h.term.waitForTickStarted(4, kHangGuardMs));
  CHECK_EQ(h.sched.snapshot().cycles, 1);              // still stopped, however far the clock moves
  typeLine(h.term, "start_marquee", h.sched); h.sched.wake();
  CHECK(h.term.waitForTickStarted(5, kHangGuardMs));
  CHECK_EQ(h.sched.snapshot().cycles, 2);              // restart renders immediately again
  h.sched.requestStop(); h.sched.join();
}

TEST(threaded_set_text_changes_the_rendered_band) {                            // req. 4
  Harness h; h.params.refreshMs = 100; h.params.pollingMs = 1000;
  CHECK(h.proc.start());
  h.sched.start();
  CHECK(h.term.waitForTickStarted(1, kHangGuardMs));
  typeLine(h.term, "set_text HELLO", h.sched); h.sched.wake();
  CHECK(h.term.waitForTickStarted(2, kHangGuardMs));
  h.sched.requestStop(); h.sched.join();               // joined => touching shared state here is race-free
  CHECK_STR(h.params.text, "HELLO");
  h.proc.cycles = 0;                                   // pure-function check: band offset 0
  const auto band = h.renderer.composeBand(h.params, h.proc);
  const auto art  = renderBlockText("HELLO");
  for (int r = 0; r < kArtRows; ++r) {
    CHECK_EQ(band[r].size(), size_t{80});              // band width invariant holds after the change
    CHECK(band[r].rfind(art[r], 0) == 0);              // the new text is at the head of the band
  }
}

TEST(threaded_set_speed_moves_the_next_deadline) {                             // req. 5
  Harness h; h.params.refreshMs = 1000; h.params.pollingMs = 1000;
  CHECK(h.proc.start());
  h.sched.start();
  CHECK(h.term.waitForTickStarted(1, kHangGuardMs));   // renders at t=0, refreshMs = 1000
  typeLine(h.term, "set_speed 50", h.sched); h.sched.wake();
  CHECK(h.term.waitForTickStarted(2, kHangGuardMs));
  h.term.clock = 60; h.sched.wake();                   // 60 < 1000 but >= 50
  CHECK(h.term.waitForTickStarted(3, kHangGuardMs));
  CHECK_EQ(h.sched.snapshot().cycles, 2);              // the NEW deadline applied, no recompile
  h.sched.requestStop(); h.sched.join();
}

TEST(threaded_exit_stops_every_thread_and_joins) {                             // req. 6, 7
  Harness h; h.params.refreshMs = 1; h.params.pollingMs = 1000;
  CHECK(h.proc.start());
  h.sched.start();
  CHECK(h.term.waitForTickStarted(1, kHangGuardMs));
  typeLine(h.term, "exit", h.sched); h.sched.wake();
  CHECK(h.term.waitForTickStarted(2, kHangGuardMs));
  CHECK(h.interp.quitRequested());                     // what ConsoleApp observes (§T2.5)
  h.sched.requestStop();                               // step 3 of the §3.10 sequence
  h.sched.join();                                      // step 4
  CHECK(!h.sched.joinable());                          // no detached worker was left behind
  h.sched.requestStop(); h.sched.join();               // idempotent: safe to repeat
  CHECK(!h.sched.joinable());
}

TEST(destroying_a_live_scheduler_joins_instead_of_detaching) {                 // req. 7
  FakeTerminal term; Parameters params; MarqueeProcess proc; Renderer renderer;
  Interpreter interp{params, proc};
  params.refreshMs = 1;
  {
    Scheduler sched{term, params, renderer, interp, proc, [&] { return term.clock.load(); }};
    sched.start();
    CHECK(term.waitForFrames(1, kHangGuardMs));        // deliberately NOT stopped before the scope ends
  }                                                    // ~Scheduler must requestStop() + join() and return
  CHECK(true);                                         // reaching this line IS the assertion
}

TEST(only_the_marquee_thread_writes_frames) {                                  // req. 8, 9
  Harness h; h.params.refreshMs = 1; h.params.pollingMs = 1000;
  CHECK(h.proc.start());
  h.sched.start();
  CHECK(h.term.waitForFrames(2, kHangGuardMs));
  const auto writers = h.term.writersCopy();
  CHECK(!writers.empty());
  for (const auto& w : writers) { CHECK(w != std::this_thread::get_id()); }   // never the input thread
  for (const auto& w : writers) { CHECK(w == writers.front()); }             // exactly one writer, ever
  h.sched.requestStop(); h.sched.join();
}

TEST(every_write_is_one_whole_frame) {                                         // req. 8
  Harness h; h.params.refreshMs = 1; h.params.pollingMs = 1000;
  CHECK(h.proc.start());
  h.sched.start();
  CHECK(h.term.waitForFrames(3, kHangGuardMs));
  const auto writes = h.term.writesCopy();
  CHECK_EQ(writes.size(), h.term.frameCount());        // one write per frame: no partial frames
  std::string joined;
  for (const auto& w : writes) {
    CHECK(!w.empty());
    CHECK_EQ(w.substr(0, 3), writes.front().substr(0, 3));                          // every write opens a frame
    CHECK_EQ(w.substr(w.size() - 3), writes.front().substr(writes.front().size() - 3));
    joined += w;
  }
  h.sched.requestStop(); h.sched.join();
  CHECK_STR(joined, h.term.outCopy());                 // nothing was ever written outside a frame write
}

TEST(threaded_stress_typing_while_animating_at_one_millisecond) {              // req. 1, 9, 11
  Harness h; h.params.refreshMs = 1; h.params.pollingMs = 1;
  CHECK(h.proc.start());
  h.sched.start();
  CHECK(h.term.waitForFrames(1, kHangGuardMs));
  for (int batch = 0; batch < 20; ++batch) {           // 400 keystrokes, drained in batches
    for (int i = 0; i < 20; ++i) { h.term.push(KeyEvent{KeyType::Char, 'x'}); }
    drainInput(h.term, h.sched);
    h.sched.wake();
  }
  h.term.push(KeyEvent{KeyType::Backspace, 0});
  drainInput(h.term, h.sched);
  h.sched.wake();
  CHECK(h.term.waitForTickStarted(2, kHangGuardMs));
  h.sched.requestStop(); h.sched.join();
  CHECK(h.sched.snapshot().cycles > 0);                // it animated throughout; it never crashed
  CHECK_EQ(h.term.frameCount(), h.term.writesCopy().size());
}

// The ONE test in which a second REAL thread (not the test thread) drives input while the worker animates.
// gpt-v5.md §3 was right that everything above proves serializability, not real overlap; this is the fix.
// It still contains no sleep: the stop condition is the barrier waitForFrames(), and every assertion is
// interleaving-independent — it holds for *every* legal schedule, so it cannot flake.
TEST(threaded_input_thread_and_animation_overlap_in_real_time) {               // req. 1 — real concurrency
  Harness h; h.params.refreshMs = 1; h.params.pollingMs = 1;
  CHECK(h.proc.start());
  h.sched.start();
  std::atomic<bool> stopInput{false};
  std::atomic<int>  posted{0};
  std::thread inputThread([&] {                        // a REAL input thread, playing ConsoleApp::run()
    KeyEvent ev;
    while (!stopInput.load() && posted.load() < 400) {
      h.term.push(KeyEvent{KeyType::Char, 'x'});       // the "keyboard"
      if (h.term.readEvent(ev, 0)) { h.sched.postEvent(ev); posted.fetch_add(1); }
    }
  });
  CHECK(h.term.waitForFrames(5, kHangGuardMs));        // the barrier: animation advanced while input ran
  stopInput = true;
  inputThread.join();
  h.sched.requestStop(); h.sched.join();
  CHECK(posted.load() > 0);                            // input really was processed, not merely queued
  CHECK(h.term.frameCount() >= size_t{5});             // and frames kept arriving the whole time
  CHECK_EQ(h.term.frameCount(), h.term.writesCopy().size());   // still one writer, one frame per write
}
```

> **Why no test asserts "no frame happened in the next N ms".** A negative assertion cannot be made without a
> real-time window, and a real-time window is exactly what makes threading tests flaky. Every negative in the
> list above is instead expressed **positively**: "after tick *n* completed, `cycles` is still *k*" — using the
> static injected clock, which makes a background tick provably a no-op. The worker's own deadline behaviour
> is covered by the pure-step tests in Step 1, which have no threads in them at all.

- [ ] **Step 1c — write the failing `--measure` contract tests** (same file, appended; **no threads needed** —
  these are pure-step tests, so they are exactly as deterministic as Step 1). They pin the v2.5 contract that
  `gpt-v5.md` §1 showed v2.4 described inconsistently. `scheduler.cpp` gets two file-local helpers,
  `frameLine(index, now)` and `eventLine(eventMs, echoMs)`, each returning one `","`-joined line ending `\n`.
  The test file needs `<cstdio>`, `<fstream>` and `<iterator>` on top of the existing includes.

```cpp
// tests/unit/test_scheduler.cpp — appended: the --measure contract (§3.6, v2.5)
namespace {
std::string slurp(const std::string& p) {
  std::ifstream in(p); return std::string((std::istreambuf_iterator<char>(in)), {});
}
size_t countLines(const std::string& s) { size_t n = 0; for (char c : s) { if (c == '\n') { ++n; } } return n; }
}

TEST(measure_frame_line_only_for_due_frames) {
  Harness h; h.params.refreshMs = 100; h.params.measurePath = "measure_test.csv";
  std::remove(h.params.measurePath.c_str());
  h.proc.state = ProcessState::Running; h.term.clock = 0;
  (void)h.sched.tick(nullptr);                         // due (hasRendered false) => one frame line
  h.term.clock = 5;  (void)h.sched.tick(nullptr);      // neither due nor redraw => nothing at all
  h.term.clock = 100; (void)h.sched.tick(nullptr);     // due again => second frame line
  const std::string log = slurp(h.params.measurePath);
  CHECK_EQ(countLines(log), size_t{2});                // frame_index is cycles: 0 then 1
  CHECK(log.find("0,0\n") != std::string::npos);
  CHECK(log.find("1,100\n") != std::string::npos);
  CHECK(log.find(",5\n") == std::string::npos);       // the non-rendering tick wrote nothing
}

TEST(measure_pair_uses_the_oldest_unconsumed_keystroke_and_is_emitted_once) {
  Harness h; h.params.refreshMs = 100; h.params.measurePath = "measure_test.csv";
  std::remove(h.params.measurePath.c_str());
  h.proc.state = ProcessState::Running; h.term.clock = 0; (void)h.sched.tick(nullptr);
  h.term.clock = 10; h.sched.postEvent(KeyEvent{KeyType::Char, 'a'});
  h.term.clock = 20; h.sched.postEvent(KeyEvent{KeyType::Char, 'b'});   // coalesces onto ONE echo frame
  h.term.clock = 30; (void)h.sched.tick(nullptr);      // one echo frame for two keystrokes
  h.term.clock = 200; (void)h.sched.tick(nullptr);     // a due frame, with nothing owed
  const std::string log = slurp(h.params.measurePath);
  CHECK(log.find("10,30") != std::string::npos);       // OLDEST stamp (worst case), not the last key (20)
  CHECK(log.find("20,30") == std::string::npos);
  CHECK_EQ(countLines(log), size_t{3});                // frame(0) + refresh(200) + exactly ONE pair
  // The 3rd line is the pair, and it is not repeated: v2.4's never-cleared slot would have emitted it again.
  CHECK_EQ(log.find("10,30"), log.rfind("10,30"));
}
```

- [ ] **Step 2 — expect FAIL.** All **three** blocks fail to compile: `start`/`join`/`snapshot`/
  `waitForTickStarted` do not exist yet, and neither do `noteEventLocked`/`eventOwed_`/`Parameters::measurePath`;
  `FakeTerminal` is still the single-threaded double.
- [ ] **Step 3 — implement** `scheduler.hpp` + `scheduler.cpp` exactly per §3.8: the worker (`start`/`run`),
  the input-thread API (`postEvent`/`wake`/`requestStop`/`join`/`joinable`), the lock discipline (including
  rule 3: `size()` before the lock, `frame`/`measure` writes after it), `noteEventLocked`/`eventOwed_`,
  `appendMeasure` + the lazy `measure_` stream, `frameLine`/`eventLine`, and `snapshot()`. Then the
  `FakeTerminal` upgrade from T1.1 and `Parameters::measurePath` from T2.2.
- [ ] **Step 4 — expect PASS, and expect the Step 1 tests to be *unchanged and green*.**
  `ctest --test-dir build/debug --output-on-failure` must pass with one `unit` job, and the file must contain
  no `sleep_for`/`sleep_until`: `grep -n 'sleep_' tests/unit/test_scheduler.cpp` prints nothing.
- [ ] **Step 5 — if the toolchain has sanitizers, run the harness under TSan and ASan by hand** on Linux:
  `g++ -std=c++17 -fsanitize=thread -g -Iinclude -Itests/support -pthread src/**/*.cpp tests/support/check.cpp
  tests/unit/test_scheduler.cpp -o /tmp/tsan_sched && /tmp/tsan_sched`, and repeat with `-fsanitize=address`.
  A clean TSan run of the Step 1b section is the evidence for "no obvious data races in the designed ownership
  model". **Do not add a sanitizer preset or wire this into CI** — it is a local, optional check, and
  restructuring the build for it is out of scope.
- [ ] **Step 6 — write `docs/threading-model.md`** (≤1 page, plain language, no code beyond a 10-line sketch).
  It is the PPT's source of truth and must answer, in this order: what each thread does; what data is shared
  (the §3.8 ownership table, copied); how synchronization works (one mutex, one condition variable, the three
  lock rules — including that `size()` is read before the lock and the frame + measure writes happen after it);
  how shutdown works (the §3.10 five-step sequence, including *why* the join precedes `restore()`);
  how refresh timing works (`refresh_ms` is the deadline, `polling_ms` is the only timer and is a *maximum idle
  wait*, so it cannot delay a frame); and why typing stays responsive (echo is event-driven and redraws are not
  gated on `refresh_ms`). It must also state the **three** honest limits, in these words: two idle waiters
  roughly double idle wakeups at the same `polling_ms`; threading does **not** make terminal repaint atomic;
  and command processing can briefly wait behind one frame's *render* — the unbounded console write is what
  rule 2 removes from that path, not every wait (§3.8).
- [ ] **Step 7 — commit** `feat(app): two-thread scheduler — marquee worker owns the terminal, input thread owns commands`

#### T1.4 — Linux measurement run (W1's own hardware)

- [ ] **Step 1:** `scripts/measure.sh` sweeps `refresh_ms ∈ {16,33,50,100,200,500,1000}` with
  `polling_ms = 10`, then `polling_ms ∈ {1,5,10,20,50,100}` with `refresh_ms = 100`, writing
  `--measure=docs/measurements/linux-<machine>.csv`. Report four columns, and be explicit about what each one
  actually measures (§3.8):
  1. **frame interval** (p50/p95/max) — from the `frame_index,nowMs` lines; this is where refresh-rate
     fluidity lives.
  2. **input-to-echo processing latency** (p50/p95/max) — from `eventMs,echoMs`; *not* physical keypress
     latency, and *not* one row per keystroke: keys that queue between two frames coalesce onto one pair whose
     `eventMs` is the **oldest** of them, so the tail is the worst case rather than an average (§3.6).
  3. **idle wakeups per second** at each `polling_ms`, with `start_marquee` stopped (the knob's CPU cost).
  4. **perceived typing delay** — a human judgement recorded per cell as `none` / `slight` / `noticeable`,
     typed while the marquee animates.
- [ ] **Step 2:** visual protocol — record 60 fps and inspect band rows for partially drawn glyphs, noting the
  refresh value where a torn row first appears. **"Not observed within the tested range" is a valid and
  expected result** and must be recorded as such; the handout asks us to identify the limits *where* tearing or
  delay is present, not to manufacture one.
- [ ] **Step 3 — commit** `docs(measure): linux refresh/polling sweep`

> `tests/replay/` (the pty transcript harness) is **P2** (§10), not part of this phase. Unit + acceptance +
> the real-terminal smoke test are the required evidence.

---

### Phase 1C — W3 (Nathan): Win32 backend + rehearsal kit

#### T3.1 — Win32Terminal

**Files:** `src/platform/terminal_win32.cpp` (also defines `installShutdownHandlers` via
`SetConsoleCtrlHandler`).

- [ ] **Step 1 — manual checklist with the exact API calls:**
  - **Output VT on:** `GetConsoleMode(OUT)`; `SetConsoleMode(OUT, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING |
    DISABLE_NEWLINE_AUTO_RETURN)`. If it fails, print a clear startup error — there is **no** ANSI-free
    fallback path in v2 (§M5).
  - **VT input deliberately OFF.** Do not set `ENABLE_VIRTUAL_TERMINAL_INPUT`: it changes what the console
    delivers and would turn keys into escape sequences that `_getch` surfaces as raw bytes. Arrow keys come
    from the `0`/`224` prefix instead.
  - **Clear `ENABLE_QUICK_EDIT_MODE`** (and keep `ENABLE_EXTENDED_FLAGS`), otherwise a stray click suspends
    the animation mid-quiz.
  - **Blocking read with a timeout (the part v1 left unspecified):**

    ```cpp
    bool Win32Terminal::readEvent(KeyEvent& out, int timeoutMs) {
      const HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
      const DWORD w = WaitForSingleObject(in, timeoutMs < 0 ? INFINITE : static_cast<DWORD>(timeoutMs));
      if (w != WAIT_OBJECT_0) { return false; }          // timeout: nothing happened

      // No size check here (v2.2). Screen-buffer size records are only readable via ReadConsoleInput, and
      // ENABLE_WINDOW_INPUT is off by default and cannot reach the CRT's _kbhit/_getch path this backend uses
      // (Microsoft documents those events as "always filtered by ReadFile and ReadConsole"). `Scheduler::tick`
      // re-reads `Terminal::size()` every tick, so the resize still reaches the screen — no event required.
      if (!_kbhit()) { return false; }                   // woke for a mouse/focus record only
      const int c = _getch();
      // 0 / 224 lead byte -> second code: 72/80/75/77 => arrows; 0x03 => quit; 0x04 => Eof; \r => Enter; \b => Backspace
      ...
    }
    ```

    `_kbhit()` alone cannot wait, and polling it in a loop would burn a core *and* corrupt the measurement
    numbers.
  - **Size:** `GetConsoleScreenBufferInfo` → `srWindow`; `querySize()` returns it (falling back to 24×80) and is
    this backend's **only** resize mechanism. `Scheduler::tick` (on the marquee thread) calls `size()` every
    iteration and repaints when it changes (§3.8), so a resized window is redrawn within `pollTimeoutMs()` —
    visually immediate at the
    default `polling_ms`. Do **not** try to turn a resize into an input event: that needs `ENABLE_WINDOW_INPUT`
    plus `ReadConsoleInput`, a different input path from this backend's timed `_kbhit`/`_getch`, and Microsoft
    documents those records as filtered out of the `ReadFile`/`ReadConsole` path used here.
  - **Ctrl+C latency:** `SetConsoleCtrlHandler` runs on a separate thread and cannot interrupt the input
    thread's blocked `WaitForSingleObject`, so the input loop notices the quit flag only at its next return —
    within `pollingMs`. This is the one user-visible effect of `pollingMs` on Windows (§3.8), and T3.3 measures
    it. **Do not try to fix this by notifying the condition variable from the control handler** — that is not
    async-signal-safe, and it is unnecessary: the flag is what the input thread already polls (§3.10).
  - **`nowMs()`:** `QueryPerformanceCounter` / `QueryPerformanceFrequency` (never `clock()`).
  - **`restore()`:** original modes saved in `enterRawMode()` and restored in `restore()` + `atexit`, never
    from the control handler (which only sets the flag).
- [ ] **Step 2 — implement.**
- [ ] **Step 3 — hand-verify in both `cmd.exe` and Windows Terminal:** animation while typing; arrows and
  backspace while editing `set_text`; window resize mid-animation; a QuickEdit click does not freeze;
  `exit` leaves the console usable.
- [ ] **Step 4 — commit** `feat(win32): VT-output console backend, QuickEdit disabled, timed blocking read`

#### T3.2 — Windows rehearsal kit (rehearsal only — never graded evidence)

**Files:** `scripts/rehearse_windows.bat`, `scripts/build.bat`, `docs/quiz-windows-runbook.md`.

```bat
@echo off
REM REHEARSAL / RECOVERY ONLY. The submitted video must show Run/Debug in the IDE (§7).
setlocal
cd /d "%~dp0\.."
if not exist build\release\csopesy.exe echo MISSING build\release\csopesy.exe & exit /b 1
build\release\csopesy.exe --config=config\csopesy.ini
```

- [ ] **Step 1:** the runbook covers: IDE = CLion → press **Run/Debug** on the **`csopesy-quiz`**
  configuration (frozen binary, no build step) using the T0.5-verified terminal option; font size ≥ 16; window
  sized so the band is fully visible; recording starts **before** the Run press; 720p; one take per test case.
  The `.bat` appears only under "recovery", never as a way to produce evidence.
- [ ] **Step 2 — commit** `chore(quiz): windows rehearsal launcher + runbook`

#### T3.4 — Graded run-configuration verification on Windows (the video path)

- [ ] Nathan: CLion run config `csopesy (quiz)` on **MSVC**, with *Emulate terminal in the output console*
  (documented supported for MSVC + LLDB) or *Run in external console (Windows)*; confirm `_kbhit` input works
  **while** animating, QuickEdit is disabled, and a resize mid-animation does not tear.
- [ ] Record `$env:WT_SESSION`, the chosen console host and the compiler version in
  `docs/measurements/windows-<machine>.md`.
- [ ] Confirm the run config executes the **same** binary copied into `frozen/` (record its SHA-256 once in
  `docs/frozen-artifact.md`). No launcher prints hashes.
- [ ] **Commit** `docs(quiz): verified windows graded run configuration`

#### T3.3 — Windows measurement run

- [ ] **Step 1:** `powershell -File scripts/measure.ps1` → the same sweep and the same four reported columns as
  T1.4 (frame interval, input-to-echo processing latency, idle wakeups, perceived typing delay), plus the
  Windows-specific Ctrl+C-to-exit latency vs `polling_ms` (§T3.1), writing
  `docs/measurements/windows-<machine>.md`.
- [ ] **Step 2:** visual tearing protocol (60 fps capture, inspect band rows); "not observed within the tested
  range" is a valid result.
- [ ] **Step 3 — commit** `docs(measure): windows refresh/polling sweep`

---

### Phase 1D — W2 (Byron): parameters, config, interpreter, entry point, README

#### T2.1 — Parameters with clamping

**Files:** `src/entities/parameters.cpp`, `tests/unit/test_parameters.cpp`.

- [ ] **Step 1 — failing test** — `setRefresh(0)` → applied `1`, `clamped==true`, `requested==0`;
  `setRefresh(10000)` → not clamped; `setRefresh(10001)` → applied `10000`; `setPolling(2000)` → applied
  `1000`; `setText("")` → `false`, text unchanged; `setText("  HELLO   WORLD  ")` → `true`, text
  `"HELLO   WORLD"` (**internal run preserved — the single §3.7 rule**).
- [ ] **Step 2–5:** implement, PASS, commit `feat(entity): parameter clamping with explicit clamp reports`

#### T2.2 — Config parser (a typo must never cost the quiz)

**Files:** `include/csopesy/config_io.hpp`, `src/entities/config_io.cpp`, `tests/unit/test_config.cpp`.

- [ ] **Step 1 — failing test**

```cpp
TEST(missing_file_is_not_fatal) {
  Parameters p; const auto r = loadConfig("/nonexistent.ini", p);
  CHECK(!r.ok);                       // reported...
  CHECK_STR(p.text, "CSOPESY");       // ...but defaults are intact
}
TEST(unknown_key_warns_and_continues) {
  writeFile("/tmp/t1.ini", "text = HELLO\nbogus_key = 5\nrefresh_ms = 250\n");
  Parameters p; const auto r = loadConfig("/tmp/t1.ini", p);
  CHECK(r.ok); CHECK_EQ(r.warnings.size(), size_t{1});
  CHECK_STR(p.text, "HELLO"); CHECK_EQ(p.refreshMs, 250);
}
TEST(malformed_number_keeps_default) {
  writeFile("/tmp/t2.ini", "refresh_ms = abc\n");
  Parameters p; const auto r = loadConfig("/tmp/t2.ini", p);
  CHECK_EQ(p.refreshMs, Parameters::defaults().refreshMs);
  CHECK_EQ(r.warnings.size(), size_t{1});
}
TEST(lists_booleans_and_blank_values) {
  writeFile("/tmp/t3.ini", "ascii_art = false\ndevelopers = A; B\nversion_date =\n");
  Parameters p; (void)loadConfig("/tmp/t3.ini", p);
  CHECK(!p.asciiArt);
  CHECK_EQ(p.developers.size(), size_t{2}); CHECK_STR(p.developers[1], "B");
  CHECK_STR(p.versionDate, "");              // blank is the handout's default (§1.4)
}
TEST(cli_flag_overrides_config) {
  Parameters p; (void)loadConfigFromText("refresh_ms = 100\n", p);
  (void)applyCliArgs({"--refresh=250", "--text=HI THERE", "--plain"}, p);
  CHECK_EQ(p.refreshMs, 250); CHECK_STR(p.text, "HI THERE"); CHECK(!p.asciiArt);
}
```

- [ ] **Step 2–4:** implement `loadConfig`, `loadConfigFromText`, `applyCliArgs` for exactly the flag set in
  §3.6; PASS.
- [ ] **Step 5 — commit** `feat(entity): ini config + CLI plumbing with warnings`

#### T2.3 — Line editor (own echo, because raw mode has none)

**Files:** `include/csopesy/line_editor.hpp`, `src/features/commands/line_editor.cpp`,
`tests/unit/test_interpreter.cpp`.

- [ ] **Step 1 — failing test** — `h i Enter` yields line `hi` with echoed buffer `hi`; `Backspace` at col 0 is
  a no-op; `Backspace` mid-line removes the char before the cursor; a line wider than the visible area keeps
  the prompt on **one** row and scrolls horizontally — a `visibleSlice(buffer, availWidth)` helper returns at
  most `availWidth` characters and shows the tail so the cursor stays visible (no multi-row wrapping); arrows
  are ignored (no crash, no state change); `Eof` at position 0 marks the line complete.
- [ ] **Step 2–5:** implement, PASS, commit `feat(commands): line editor with immediate echo and horizontal scrolling`

#### T2.4 — Interpreter + response table

**Files:** `include/csopesy/interpreter.hpp`, `src/features/commands/interpreter.cpp`,
`tests/unit/test_interpreter.cpp`.

> **v2 change:** v1 froze full sentences including decorative suffixes. v2 asserts the **token** for state
> messages and the exact line for `Usage:`/goodbye/`help` header (see `REVIEW_ADJUDICATION.md` #7).

- [ ] **Step 1 — failing test**

```cpp
TEST(response_table) {
  struct Row { const char* in; const char* token; };
  const Row rows[] = {
    {"help",                "Available commands:"},
    {"start_marquee",       "Marquee started"},
    {"start_marquee",       "already running"},        // second call, same interpreter
    {"stop_marquee",        "Marquee stopped"},
    {"stop_marquee",        "is not running"},
    {"set_text HELLO WORLD","Marquee text set to \"HELLO WORLD\""},
    {"set_speed 250",       "Marquee speed set to 250 ms"},
    {"set_speed 0",         "out of range [1, 10000]; clamped to 1 ms"},
    {"foo",                 "Command not recognized: \"foo\""},
    {"HELP",                "Command not recognized"},  // case-sensitive, per §3.7
  };
  Harness h;
  for (const auto& r : rows) { CHECK(h.interp.executeLine(r.in).find(r.token) != std::string::npos); }
}
TEST(usage_lines_are_exact) {
  Harness h;
  CHECK_STR(h.interp.executeLine("set_text"),        "Usage: set_text <text>");
  CHECK_STR(h.interp.executeLine("set_speed"),       "Usage: set_speed <milliseconds>");
  CHECK_STR(h.interp.executeLine("set_speed abc"),   "Usage: set_speed <milliseconds>");
  CHECK_STR(h.interp.executeLine("set_speed 5abc"),  "Usage: set_speed <milliseconds>");  // no partial parse
  CHECK_STR(h.interp.executeLine("set_speed 1.5"),   "Usage: set_speed <milliseconds>");
}
TEST(negative_speed_is_clamped_not_a_usage_error) {   // §3.7 rule 6
  Harness h;
  CHECK(h.interp.executeLine("set_speed -5").find("out of range") != std::string::npos);
  CHECK_EQ(h.params.refreshMs, Parameters::kRefreshMin);
}
TEST(help_lists_every_required_command) {
  Harness h; const std::string help = h.interp.executeLine("help");
  for (const char* c : {"help", "start_marquee", "stop_marquee", "set_text", "set_speed", "exit"}) {
    CHECK(help.find(c) != std::string::npos);
  }
}
TEST(argument_whitespace_rule) {
  Harness h;
  (void)h.interp.executeLine("   set_text   HELLO   WORLD  ");
  CHECK_STR(h.params.text, "HELLO   WORLD");       // internal run kept, surroundings trimmed
}
TEST(exit_sets_quit_flag) { Harness h; (void)h.interp.executeLine("exit"); CHECK(h.interp.quitRequested()); }
TEST(empty_and_long_input_do_not_crash) {
  Harness h;
  CHECK_STR(h.interp.executeLine("   "), "");
  (void)h.interp.executeLine("set_text " + std::string(5000, 'X'));
  CHECK(true);
}
```

- [ ] **Step 2–4:** implement a dispatch table (`std::unordered_map<std::string, CommandFn>`) implementing
  §3.7 steps 1–5 exactly; PASS.
- [ ] **Step 5 — commit** `feat(commands): interpreter, dispatch table, response table`

#### T2.5 — `main.cpp` + `ConsoleApp` (the graded entry path)

**Files:** `src/main.cpp`, `include/csopesy/console_app.hpp`, `src/app/console_app.cpp`,
`tests/smoke/{basic.txt,run_smoke.cmake}`, `CMakeLists.txt` (the smoke test of Step 4).

- [ ] **Step 1:** `main` = parse argv → load config → apply CLI → `--diag` short-circuit → construct
  `Terminal::create()` → `installShutdownHandlers()` → RAII `TerminalGuard`, whose constructor calls
  `enterRawMode()` (skipped when `noTty` / `!isTty()`) and whose destructor calls `restore()` (`atexit` as
  backstop) → `ConsoleApp{...}.run()`; returns 0.
- [ ] **Step 2:** `ConsoleApp::run()` draws the header, then **starts the marquee thread** (`sched_.start()`)
  and **becomes the input/command thread**: `readEvent(ev, params_.pollingMs)` → `sched_.postEvent(ev)` → test
  `interp_.quitRequested() || shutdownRequested()`. On exit it runs the §3.10 sequence — `sched_.requestStop()`,
  then `sched_.join()` **before** the `TerminalGuard` restores the terminal — and only then prints
  `Exiting CSOPESY. Goodbye!`. **Since v2.6 this test is what actually stops the worker** (`run()` no longer
  reads `quit_`), so the `quitRequested()` check must run after *every* `postEvent` — including on the `EINTR`
  and timeout paths — or `exit` would leave the worker animating until the next spurious wakeup.
  ConsoleApp never renders and never calls `sched_.tick()`: while the worker lives,
  the marquee thread is the only writer. Printing the goodbye line *after* the join is legal for exactly the
  reason plain line mode may write at all — the join has returned the program to one thread. `Scheduler` is
  owned by `ConsoleApp` (member or local in `run()`), and either way `~Scheduler` joins as a backstop, so no
  early return can leave a worker running.
- [ ] **Step 2b — plain line mode** (§3.6): when `noTty` / `!isTty()`, skip raw mode and all rendering; read
  lines from stdin, pass each to `Interpreter::executeLine`, print the response and `Command>` as plain text,
  and return 0 on `exit`/EOF. This is what makes the CI smoke test below possible. **No worker thread is
  started in this mode** — `--no-tty` stays single-threaded (it cannot animate), so its output is
  byte-identical to v2.3's and the smoke test's assertions are unchanged.
- [ ] **Step 3:** verify by hand on your OS: `./build/debug/csopesy --config=config/csopesy.ini` shows the
  handout header; `start_marquee` animates while typing stays responsive; `set_speed 1` and `set_speed 10000`
  both behave; `Ctrl+C` restores the terminal; and a window resize repaints the layout.
- [ ] **Step 4 — the CI-runnable end-to-end smoke test** (added in v2.1, made portable in v2.2; this is the one
  check that exercises the real binary on all three OS images without a tty — register it here, where the plain
  line mode of Step 2b finally exists for it to assert). It is a **CTest test, not a shell pipe**: `ctest` runs
  a CMake script that feeds stdin from a file, so there is no `printf`, no `|`, and no guessed binary path. That
  matters because GitHub Actions' default shell on Windows is **pwsh** (where v2.1's
  `printf … | ./build/debug/csopesy` line could not run at all), and the binary path differs between
  multi-config and single-config generators. Nothing here guesses a path: `$<TARGET_FILE:csopesy>` is the only
  thing that knows where a given generator actually put the binary.

  **Files:** `tests/smoke/{basic.txt,run_smoke.cmake}`, plus this block appended to `CMakeLists.txt`:

```cmake
# Portable end-to-end smoke test: no shell, no `printf`, no guessed binary path, no *nix-only syntax.
# $<TARGET_FILE:csopesy> resolves to the real artifact on every generator (VS puts it in build/vs/Debug).
add_test(NAME smoke COMMAND ${CMAKE_COMMAND}
         -DEXE=$<TARGET_FILE:csopesy>
         -DINFILE=${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke/basic.txt
         -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke/run_smoke.cmake)
set_tests_properties(smoke PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
```

  The harness is driven by CMake (`execute_process` supports `INPUT_FILE`), so it behaves identically under
  PowerShell, bash and cmd:

```cmake
# tests/smoke/run_smoke.cmake
# Called as: cmake -DEXE=<binary> -DINFILE=<input> -P run_smoke.cmake
# FILE is a CMake built-in when running with -P; do not shadow it.
execute_process(COMMAND "${EXE}" --no-tty
                INPUT_FILE "${INFILE}"
                OUTPUT_VARIABLE out ERROR_VARIABLE err
                RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "smoke: exit code ${rc}\n${err}")
endif()
foreach(needle "Available commands:" "out of range [1, 10000]; clamped to 1 ms" "Exiting CSOPESY. Goodbye!")
  string(FIND "${out}" "${needle}" pos)
  if(pos EQUAL -1)
    message(FATAL_ERROR "smoke: missing [${needle}] in output:\n${out}")
  endif()
endforeach()
message(STATUS "smoke: OK")
```

```text
# tests/smoke/basic.txt
help
set_speed 0
exit
```

  `ctest` now reports **two** tests. `--no-tty` selects plain line mode (Step 2b), and asserting the
  `out of range [1, 10000]; clamped to 1 ms` line also pins the `set_speed 0` clamp rule end-to-end, which no
  unit test does. A red `smoke` on a machine where `unit` is green means the *binary* or the shell-free input
  path broke — exactly the signal CI exists to give.

- [ ] **Step 5 — commit** `feat(app): entry point, delegated console loop, plain line mode, safe shutdown`

#### T2.6 — `README.txt` (literal handout item) + frozen artifact

**Files:** `README.txt`, `.gitignore` (`build/`, `frozen/`).

- [ ] **Step 1:** README.txt contains each member's name; the entry file **`src/main.cpp`** (function `main`)
  and class `csopesy::ConsoleApp`; per-OS build steps (`cmake --preset …`); the run command; the parameter
  table (`text`, `refresh_ms`, `polling_ms`, `ascii_art`, `marquee_row`, `developers`, `version_date`); and the
  note that config file **and** CLI flags set each knob, with the six commands mutating `text`/`refreshMs`/
  process state at runtime. **v2.4:** add a ≤5-line description of the two threads and point at
  `docs/threading-model.md`. The grader opens the README first, and "two threads: one reads your keystrokes,
  one animates the band and owns the screen" is a sentence that must not be missing from it.
- [ ] **Step 2:** build the release binary and copy it to `frozen/` **(not committed — `frozen/` is
  gitignored; it goes out in the submission package)**, and record its SHA-256 in `docs/frozen-artifact.md`.
- [ ] **Step 3 — commit** `docs: README.txt with entry file, run steps, parameter table`

---

### Phase 2 — Integration and cross-OS verification (W2 leads)

#### T5.1 — Integration on Windows (the primary target)

- [ ] W2 and W3 run the full acceptance list (§6.2) on the Windows build; every failure becomes a task, not a
  "known issue".
- [ ] Verify the parameter-path redundancy: for `text`, `refresh_ms` and `ascii_art`, prove the effect via
  config file **and** CLI flag (and via the runtime command where one exists) with the same frozen binary.

#### T5.2 — Cross-OS matrix

- [ ] CI green on ubuntu + windows + macos (`T0.3`).
- [ ] W1 runs unit tests on native Linux; W4 runs unit tests + a macOS backend smoke test (`Terminal.app`,
  `iTerm2`) — **optional, P2**; W2 runs both Windows and Linux locally.
- [ ] Record results in `docs/measurements/matrix.md` (OS × feature × result × who verified).

#### T5.3 — Tearing and typing-delay sweeps (all four machines)

- [ ] Each member runs `scripts/measure.sh` / `measure.ps1` on their own hardware and commits their table.
- [ ] **Method:** vary `refresh_ms` with `polling_ms` fixed at 10 first; then vary `polling_ms` with
  `refresh_ms` fixed at 100. Only then probe the few combinations near the thresholds that showed tearing or
  delay — do **not** run the full 7×4 cross product on every machine before the independent sweeps.
- [ ] W4 synthesizes: recommended values per machine, the group's recommended default (expected
  `refresh_ms` 80–120, `polling_ms` 8–16 — confirm with data, do not assume), the refresh below which tearing
  appears, and the `polling_ms` above which **idle CPU** or **Ctrl+C-to-exit latency** becomes noticeable.
- [ ] **State the negative result.** If no tested `refresh_ms`/`polling_ms` produces tearing or a noticeable
  typing delay, report exactly that, with the table that supports it, and explain why: input is event-driven,
  so the read timeout is a maximum idle wait and cannot delay a keystroke or a frame (§3.8). Recording "not
  observed within the tested range" is a correct outcome; inventing a threshold to satisfy the slide is not.

---

### Phase 3 — Documentation, PPT, freeze

#### T6.1 — PPT sections (owners per §4.5)

- [ ] *Command recognition* (W2), *Console UI implementation* (W4 + W3 review), *Command interpreter
  implementation* (W2), *Process representation* (W1), *Scheduler implementation* (W1), *Refresh vs polling
  balance* (W4, with W3's Windows and W1/W4's POSIX tables).
- [ ] The scheduler slide uses the §3.8 wording: two threads (input/command + marquee worker), one mutex, one
  condition variable, one terminal writer. It states *why* threading is used (the marquee's timing must not
  share a control path with command entry), shows the §3.8 ownership table, and states the three honest limits —
  two idle waiters roughly double idle wakeups at the same `polling_ms`; threading does **not** make terminal
  repaint atomic; and command processing can briefly wait behind one frame's *render*, because what rule 2
  removes from that path is the unbounded console write, not every wait. Source of truth:
  `docs/threading-model.md` (T1.3 Step 6).
- [ ] The rendering slide uses the §3.9 wording: *one assembled frame per write prevents application-level
  interleaving* — never "tearing is impossible".
- [ ] The *Refresh vs polling balance* section must use §3.8's actual semantics, not a generic textbook one:
  `polling_ms` is a **maximum idle wait**, so it trades idle CPU wakeups (and, on Windows, Ctrl+C latency)
  against nothing else, while fluidity is governed by `refresh_ms` and per-frame render cost. Show the
  measured tables, and show "not observed" where that is what the data says.

#### T6.2 — Video production (W4 directs, W3 runs on Windows)

- [ ] One continuous take per §6.2 case; **every take starts with the Run/Debug press in CLion on
  `csopesy-quiz`** (the frozen-binary configuration, which builds nothing); no source access after that; IDE
  font ≥ 16; 720p; ≤ 1 GB; MP4 embedded in the PPTX (re-check total deck size).
- [ ] Two full rehearsals before the graded recording; keep the rehearsal video as a fallback if a graded take
  fails.

#### T6.3 — Freeze

- [ ] **Step 0 — re-verify `csopesy-quiz` now that the artifact exists.** Press Run on `csopesy-quiz` and
  confirm: no `Build` entry runs in Before-launch, the process starts the binary in `frozen/`, and the binary's
  SHA-256 is **unchanged** after the press (a rebuild/relink would change it, and would invalidate the frozen
  claim). Record this in `docs/clion-run-config.md`.
- [ ] Tag `quiz-frozen` on the commit whose binary is used; record the SHA-256 in `docs/frozen-artifact.md`
  and copy the binary into the submission package.
- [ ] After the tag: **no code edits**. Parameter files and command inputs only.
- [ ] Dry-run the whole quiz flow (§7) using only the frozen artifact, **copying one `config/quiz_case_<n>.ini`
  over `config/csopesy.ini` per case** (§3.6, §7 T-0:02) — the frozen Run loads that one literal path, so the
  copy is the case selector. Explicitly verify the stale-config hazard: after switching cases, `--diag` must
  report the **new** case's effective values, never the previous case's.

---

## 6. Verification and definition of done

### 6.1 Definition of done (per workstream)

1. The task's test existed, failed before the implementation, and passes after it — **or**, for the
   platform-surface tasks named in §5 (T1.1, T3.1, T0.5), the task's hand-run checklist was executed on the
   owner's hardware and its result recorded in the PR (§6.3). Demanding a red unit test for `tcsetattr` or
   `SetConsoleMode` would produce theatre, not evidence.
2. **Concurrency tests are deterministic, not merely green once.** T1.3 Step 1b contains **no**
   `std::this_thread::sleep_for`/`sleep_until` (Step 4 greps for it): a real-time sleep is a flake waiting to
   happen, and it is exactly what the plan forbids.
3. CI is green on all three OS families (or the file is explicitly excluded from the platform it cannot build on).
4. No header contract changed without the §4.5 protocol.
5. The change is demonstrated on the owner's own hardware (screenshot/transcript in the PR).
6. The commit message names the requirement (R1–R9) it advances.
7. **The concurrency claim is backed by concurrency evidence, not by green unit tests.** T1.3 Step 1b proves
   *race-freedom and state serializability* under a driven input thread; on its own it does **not** demonstrate
   real overlap. The `threaded_input_thread_and_animation_overlap_in_real_time` test (Step 1b) and the A8
   transcript are the evidence for "the marquee animates while commands are still accepted", and the PR must
   say which one it relied on (adopted from `gpt-v5.md` §3).

### 6.2 Mandatory acceptance tests (varying inputs — full-credit criterion)

| # | Case | Varying inputs | Expected |
| --- | --- | --- | --- |
| A1 | `help` | at a clean prompt; while animating | the 6-line table, identical both times |
| A2 | `start_marquee` | when stopped; when already running; immediately after `stop_marquee` | `Marquee started` / `already running` / `stopped` then `started` |
| A3 | `stop_marquee` | when running; when never started | `Marquee stopped.` / `Marquee is not running.` |
| A4 | `set_text` | single word; multiple words; punctuation; digits; 200+ chars; lowercase; extra internal spaces; while stopped | the echoed text equals the argument after §3.7's single whitespace rule; art re-rasterizes; no crash |
| A5 | `set_speed` | `1`, `16`, `100`, `1000`, `10000`, `0`, `-5`, `10001`, missing arg, `abc`, `5abc`, `1.5`, huge number | a well-formed number out of range (incl. `-5`) is clamped with an explicit message; a non-number is a `Usage:` error; never a crash; animation visibly changes rate |
| A6 | `exit` | while animating; with text in the buffer | goodbye line, terminal restored, exit code 0, shell prompt returns. *(v1's "twice" is impossible and was removed.)* **(v2.4: the worker has been joined by this point — the process must exit promptly, never hang on the join, and `ps -T`/Task Manager must show no leftover thread.)** |
| A7 | Unknown/empty input | `foo`, `HELP`, `start marquee`, whitespace only, `set_speed 100 extra` | `not recognized` / `Usage:` messages, no crash, prompt intact |
| A8 | Coexistence (**stress**) — *the real-concurrency acceptance case* | type a full command **one key at a time** during animation; resize mid-animation; type during a `1 ms` refresh | typing stays responsive; **several marquee updates are observed between the first keystroke and `Enter`** — record the transcript/recording and state the observed count, because this is the one case the deterministic suite cannot substitute for (`gpt-v5.md` §3); band never half-drawn and **no frame ever interleaves or garbles another** (the marquee thread is the only writer, §3.8); **the layout follows the new size after a resize** |
| A9 | Parameter-only paths | the same effect via `config/csopesy.ini` and via `--refresh=`/`--text=`/`--plain`, on the frozen binary | identical observable behavior (the §1.2 partial-credit hedge) |
| A10 | Extremes | `refresh_ms=1`, `refresh_ms=10000`, `polling_ms=1`, `polling_ms=1000` | no input loss, no crash, terminal stays responsive. *(Idle CPU and Ctrl+C latency are **observations** recorded in the PPT, not pass/fail criteria — and `polling_ms` cannot add typing latency at all, §3.8.)* |

### 6.3 What "proof" means here

- Unit tests: `ctest` output pasted into the PR — the task's test must have **failed before** the change.
- Threading: the T1.3 Step 1b `ctest` run, plus the optional local TSan/ASan transcript if the toolchain has
  it (T1.3 Step 5). A clean TSan run of that section is what "no obvious data races" is allowed to mean here —
  it is evidence, not a guarantee, and the §3.8 ownership table is the actual argument.
- **Concurrency — a claim distinct from race-freedom:** the
  `threaded_input_thread_and_animation_overlap_in_real_time` test (two real threads, barrier-synchronized, no
  sleeps) plus the A8 transcript. A green deterministic suite is **not** offered as proof that two activities
  overlapped in time; `gpt-v5.md` §3 was right that it does not show that, and the plan no longer implies it.
- Platform behavior: a terminal transcript/screenshot from the owner's machine. For T1.1, T3.1 and T0.5 this
  transcript **is** the task's test (there is no unit-test substitute — §5), which is why those tasks ship a
  checklist instead of a failing test.
- Measurements: the `--measure` CSV + the derived table, committed under `docs/measurements/`.
- Quiz readiness: a full dry run of §7 against the frozen SHA-256.

**Simulation is not verification.** The pre-commit simulation recorded in this plan (§10) checks that the
*specified* algorithm is self-consistent — arithmetic, thresholds and deadlines. It proves nothing about C++,
compilation, or a real terminal. A task is done when its own test fails first and then passes under `ctest`
(or, for the platform-surface tasks, when its checklist passes on the owner's hardware), and the behaviour is
observed on the owner's hardware.

---

## 7. Quiz-day runbook (time-pressure)

```text
T-0:00  Open a terminal at the repo root and confirm the frozen binary is the one built for the tag:
        Windows: certutil -hashfile frozen\csopesy.exe SHA256
        POSIX:   sha256sum frozen/csopesy
        (matches docs/frozen-artifact.md)
T-0:02  Select the case's parameters by copying its file over the ONE path the frozen Run loads (§3.6):
        Windows: copy /Y config\quiz_case_<n>.ini config\csopesy.ini
        POSIX:   cp -f config/quiz_case_<n>.ini config/csopesy.ini
        `csopesy-quiz` always passes --config=config/csopesy.ini (§T0.5), so this copy IS the case selector.
        Skipping it silently runs the PREVIOUS case's parameters — a missing file is never fatal (§3.6), so
        there is no error to warn you. Then sanity-check that exact file with the frozen binary:
        Windows: frozen\csopesy.exe --config=config\csopesy.ini --diag
        POSIX:   ./frozen/csopesy --config=config/csopesy.ini --diag
        (one line: isTty / size / VT / effective refresh+polling — compare it against the case's expected
        values, not just against "it ran").
T-0:03  Start the recorder (720p, uncut) BEFORE launching.
T-0:05  PRESS RUN/DEBUG IN THE IDE on the **`csopesy-quiz`** configuration — the handout requires the video
        to show this. `csopesy-quiz` is a Custom Build Application whose Executable is frozen\csopesy.exe and
        whose Before-launch has NO Build step, so the press cannot recompile anything (verified at T6.3
        Step 0). If the terminal switch from T0.5 failed on this machine, the config points at
        scripts/run_external.* instead — still a Run/Debug press, still the frozen binary.
T-0:10  Execute the case's commands exactly as written. Do not improvise; do not open a second window; do
        not touch source.
T-0:40  `exit`, stop the recorder, note the take number + case id. Never delete a take.
T-1:00  Repeat for the next case. Keep the terminal geometry identical across takes so the PPT's tables match
        the video.
```

**There is no launcher-based evidence path.** `scripts/rehearse_windows.bat` is for practice and for recovering
from a wedged terminal; it never appears in a submitted take.

Fallback ladder if a case misbehaves mid-quiz: (1) retry the same command; (2) reach the same state through
the parameter path (`--refresh=` / `--text=` / the config file) and keep recording — that is the documented
partial-credit route; (3) if the terminal is wedged, `Ctrl+C` (POSIX) / close the window, restart **without**
recompiling, and continue recording.

---

## 8. Risks and mitigations

| # | Risk | Impact | Mitigation | Owner |
| --- | --- | --- | --- | --- |
| 1 | Raw mode not restored after a crash ⇒ terminal unusable | quiz-blocking | flag-only signal handler + normal-scope `restore()` + `atexit` backstop; tested with `exit`, `Ctrl+C`, `SIGTERM`; runbook step to `stty sane` | W1 |
| 2 | Windows QuickEdit freezes the app on a stray click | quiz-blocking | clear `ENABLE_QUICK_EDIT_MODE` in `enterRawMode`; verify by clicking during animation | W3 |
| 3 | A console without VT support renders escape codes literally | no points for UI | enable VT explicitly and **fail loudly at startup** if it cannot be enabled; prefer Windows Terminal; `--diag` reports it (v1's separate ANSI-free mode was cut as a second render path) | W3 |
| 4 | macOS-only POSIX divergence discovered late | lost macOS baseline only | macOS stays non-blocking: Linux first, macOS as a P2 second-opinion pass | W4 |
| 5 | Compile break on a platform nobody is sitting at | lost hours | CI matrix from day 1 (T0.3); contracts frozen (T0.4) | W2 |
| 6 | Header churn after Phase 0 | three members blocked | one-writer-per-file (§4.5); contract marker; changes announced | W2 |
| 7 | Busy-spin at low `polling_ms` burns a core, causes jitter | measurement noise | both threads wait on bounded timed primitives, never on a spin: the marquee thread on `cv_.wait_for` capped by `pollTimeoutMs()` ≥ 1 ms, the input thread on `poll`/`WaitForSingleObject` — never `sleep(0)`, never `_kbhit` in a loop | W1/W3 |
| 8 | Glyph coverage gap for text typed during the quiz | art looks broken | box-glyph fallback for every unknown char **plus** the width-invariant test over all 95 printable chars | W4 |
| 9 | "Process representation / scheduler" expected to be *threads* | PPT graded down | **inverted in v2.4:** the threaded design is now the implementation — two threads, one mutex, one cv, one terminal writer (§3.8); the PPT presents the ownership table and `docs/threading-model.md`, and §9 Q3 confirms the shape rather than asking permission | W1 |
| 10 | "Parameters only" interpreted as "edit files" or "type commands" | wrong quiz prep | config **and** CLI for every knob, commands for the four the handout names; §9 Q1 asks | W2 |
| 11 | Video rejected for format reasons (cut, >720p, >1 GB, MP4 not embedded, no Run/Debug press) | submission invalidated | §T6.2 pre-flight checklist; runbook §7 forbids launcher-based takes | W4 |
| 12 | The graded binary drifts from source (someone rebuilds, or CLion's default Build pre-launch step relinks) | invalidates "no recompile" | `csopesy-quiz` is a Custom Build Application with an **empty** build target and no Build step, pointing at `frozen/`; `quiz-frozen` tag + SHA-256 in `docs/frozen-artifact.md`; T6.3 Step 0 proves the hash is unchanged after a Run press | W3/W2 |
| 13 | CLion's Run window may not give a real console on some toolchain/debugger combinations | quiz-blocking | T0.5's per-OS verification gate (*Emulate terminal in the output console* — documented supported with MSVC + LLDB; *Run in external console* on Windows) + T0.5b fallback + the record in `docs/clion-run-config.md` | Byron owns the record, all verify |
| 14 | CLion builds with a different toolchain than the frozen binary (bundled MinGW vs MSVC) | invalidates the frozen-build claim | T0.5 Step 2 pins and records toolchains; `csopesy-quiz` runs the artifact from `frozen/` **without** a build step, so no toolchain is involved at Run time | Byron, Nathan |
| 19 | **A window resize is silently dropped** ⇒ the layout is stranded at the old width | cosmetic at best, looks broken on video at worst | `Scheduler::tick` re-reads `Terminal::size()` every iteration, resizes the `FrameBuffer` **and sets the repaint flag** (§3.8); no platform resize event is involved on either backend; `size_change_forces_a_repaint_without_any_event` + A8 cover it | W1/W3 |
| 20 | The `--measure` numbers are mistaken for physical typing latency in the PPT | a false claim in a graded report | metric renamed to **input-to-echo processing latency**; perceived typing delay is a separate human-observed column (§3.6, T1.4) | W4 |
| 15 | The professor never answers §9 ⇒ we guess wrong | wasted prep / lost PPT marks | emailed 2026-09-16, checkpoint 2026-09-18; every plan default already satisfies both readings | Byron chases |
| 16 | 7-day schedule slips | missed submission | §5.0 end-of-day gates; ordered cut line; a spec requirement is never cut | Byron |
| 17 | Enabling VT **input** alongside `_getch` turns keys into escape sequences | broken input on Windows | VT input is deliberately **not** enabled; arrows come from the `0`/`224` prefix (T3.1) | W3 |
| 18 | `_kbhit()` used as a wait ⇒ 100 % core and corrupted measurements | measurement noise, quiz lag | `WaitForSingleObject(consoleInput, timeoutMs)` then `_kbhit`/`_getch` (T3.1, M4) | W3 |
| 21 | Deadlock or lock inversion between the two threads | hangs the graded run | exactly **one** mutex and **one** condition variable in the whole program; `tick()` is never called while `mu_` is held (§3.8 rule 1); the terminal write happens outside `mu_` (rule 2); every lock is an RAII guard in a bounded scope; no nested locks exist, so there is no lock order to get wrong | W1/W2 |
| 22 | A worker thread outlives the objects it references (use-after-free) | crash at exit or mid-quiz | the §3.10 order — `requestStop()` → `join()` → *then* `restore()`; `~Scheduler` joins as a backstop; **no `detach()` anywhere**; `destroying_a_live_scheduler_joins_instead_of_detaching`; the `unit` ctest `TIMEOUT 60` turns a missed join into a red test instead of a stuck job (T0.2) | W1 |
| 23 | POSIX link fails on `pthread` because `Threads::Threads` is missing | build dead on 3 of 4 machines | `find_package(Threads REQUIRED)` + a `PUBLIC` link on `csopesy_core` (T0.2). It fails at *link* time only, so T0.2 Step 1 says explicitly what to look for; CI proves it on the two non-MSVC images | W2 |
| 24 | Threading silently costs CPU: two idle waiters instead of one | worse PPT numbers, fan noise during the quiz | `polling_ms` is the only timer in either thread, so raising it still reduces idle wakeups; T1.4/T5.3 report wakeups for both threads and the PPT states the doubling as the price of the architecture | W1/W4 |
| 25 | The threaded rewrite drifts from the assignment's observable behaviour | marks lost on behaviour that already worked | the pure-step tests, A1–A10 and the `smoke` test are **unchanged** and must stay green; `threading.md`'s rule — change the architecture, not the contract — is the review criterion; every new test is added in a new section of the file | W1/W2 |
| 26 | `postEvent()` still waits behind one frame's **render** (the critical section is CPU work) | perceived typing delay at the A4 extreme; an overclaim in the PPT | §3.8 states the narrow true guarantee ("the unbounded console write can never stall the input thread") instead of "the input thread never waits"; rules 2–3 keep `size()` and every write/append outside `mu_`; T1.4 reports the worst-case `eventMs,echoMs` at 200+ characters, which is the measured size of this window | W1/W4 |
| 27 | The `--measure` file is read as **per-keystroke** latency | a false claim in a graded report | §3.6 makes coalescing explicit (keys between two frames share one pair), pins `eventMs` to the **oldest** unconsumed keystroke, and risk 20 already renamed the metric to *input-to-echo processing latency*; Step 1c tests assert one-pair-per-echo-frame | W4 |
| 28 | A second reader of `Interpreter::quit_` reappears (someone restores `res.quit`/`TickResult::quit`) | a cross-thread read that needs a correctness argument the project should not have to make; a subtly wrong shutdown path | v2.6 removed the worker's read and the field; §3.8's ownership table gives `quit_` its own row, and §3.10/§3.11 state that shutdown crosses as `stop_` only. Restoring it also breaks the `TickResult` contract in §3.8 and T0.4's frozen marker | W1 |

---

## 9. Questions for the professor — emailed 2026-09-16, answer needed by 2026-09-18

| # | Question | If unanswered by Sep 18 |
| --- | --- | --- |
| 1 | Does "modify the parameters" mean editing a **parameter file** on the frozen build, or typing parameters **as commands**? | **Both** — `config/csopesy.ini`, CLI flags and the commands all feed one `Parameters` struct; the PPT states it |
| 2 | Is the marquee expected to be **ASCII-art graphics**, or is plain text scroll sufficient? | **ASCII-art by default**; plain scroll reachable via `ascii_art = false` / `--plain` (A9 covers both) |
| 3 | For *process representation* and *scheduler implementation*: we now run **two** threads — one for input/commands, one for the marquee/scheduler — with a condition-variable handoff and a single terminal writer (§3.8). Is that the expected shape? | **Yes, two threads** is the implemented default (`threading.md` is the instruction); the PPT presents the §3.8 ownership table, the three lock rules, the join order and `docs/threading-model.md`. **No thread-per-process work is planned at any scope** — `gpt-v6.md`'s warning against floating it is adopted |
| 4 | Is the quiz run on your machine or ours, and does the platform matter? | **Ours, on Windows** (3 of 4 members; Nathan drives), while CI keeps Linux and macOS compiling |
| 5 | Where exactly should the marquee band sit — the two console images in the handout place it differently? | **Band directly above `Group developer:`** (`marquee_row = 3`), adjustable from the config file |

---

## 10. Plan self-review

### 10.1 Spec coverage check

| Spec item | Covered by |
| --- | --- |
| OS emulator with command input + display output | §3.3–3.11, T2.5 |
| **Concurrency**: the marquee animates while commands are still accepted | §3.8 (two threads, ownership table, three lock rules), T1.3 Step 1b/1c, A8 |
| Main menu console (welcome, developers, version date, prompt) | §1.4, T4.5, T2.5 |
| Text / ASCII-graphics marquee | T4.1, T4.4 |
| Commands change marquee behavior | T2.4, T1.2, T1.3 Step 1b |
| `help`, `start_marquee`, `stop_marquee`, `set_text`, `set_speed`, `exit` | §3.7 + T2.4 |
| Parameters modifiable without recompiling | §3.5, §3.6, T2.2, T2.6, A9 |
| Varying inputs earn full points | §6.2 A1–A10 |
| Workaround path for partial credit | §1.2, §3.5, runbook fallback ladder |
| SOURCE + README.txt + entry file | T2.6 |
| PPT: command recognition, console UI, interpreter, process, scheduler, refresh-vs-polling | T6.1 + §4.5 RACI |
| Recommended values + tearing/delay limits for the current hardware | T1.4, T3.3, T5.3 |
| Video: uncut, 480p–720p, ≤1 GB, Run/Debug visible, no code changes, MP4 in PPTX | §1.3, T3.2, T6.2, §7 |
| Submission **Wed 2026-09-23** | §5.0 gates + cut line |

### 10.2 Placeholder scan

No "TBD"/"implement later": every task names its files, its test, its implementation shape, its run command
and its expected result. Deliberately deferred and labelled: the full glyph table (T4.1 shows the structure,
two glyphs, the fallback rule and the width invariant — the remaining ~45 cells are mechanical), the exact
measurement numbers (they are the *output* of T5.3), and the P2 extras (§10.5).

### 10.3 Name/type consistency check

`Terminal`, `KeyEvent`/`KeyType`, `Size`, `Parameters`, `ClampReport`, `MarqueeProcess`/`ProcessState`,
`Scheduler`/`TickResult`/`Clock`, `Renderer`, `FrameBuffer`, `Glyph`, `Interpreter`, `FakeTerminal`,
`scrollOffset`, `sliceRow`, `glyphFor`, `renderBlockText`, `artWidthFor`, `loadConfig`, `loadConfigFromText`,
`applyCliArgs`, `setRefresh`, `setPolling`, `setText`, `installShutdownHandlers`, `shutdownRequested`,
`visibleSlice` (line editor) and `querySize` (the Windows backend's live size read, §3.8) —
each introduced once in §3, used with the same signature in every task. Removed names (`stepCols`, `scheduler`,
`devCommands`, `statsPath`, `bandRows`, `quantumMs`, `Ready`, `Finished`, `ascii_art` on the PCB,
`runOnce`, `responses.cpp`, `ansi.cpp`) appear nowhere in the current revision (v2.6). Fields added since v1:
`MarqueeProcess::hasRendered` and `Scheduler::fbRows_`/`fbCols_` (§3.8). `KeyType::Resize` was **removed** in
v2.2 — no backend produces one, because §3.8 detects a resize from `size()` — so `KeyType` is now
`{None, Char, Enter, Backspace, Tab, Escape, ↑↓←→, Eof}`. `Scheduler` lives in `src/app/` (T1.3), not
`src/entities/`, and `Win32Terminal` keeps no cached size.

**Added in v2.4 (the threading conversion, §10.10):** `Scheduler::start`, `requestStop`, `join`, `joinable`,
`postEvent`, `wake`, `snapshot`, `pollTimeoutMsLocked`, `SchedulerSnapshot`, and the fields `worker_`, `mu_`,
`cv_`, `started_`, `stop_`, `dirty_`, `wakeTick_`, `pendingEventMs_`; `FakeTerminal::push`, `outCopy`,
`frameCount`, `writesCopy`, `writersCopy`, `waitForFrames`, `waitForTickStarted`. Two re-readings rather than
renames: `Scheduler::run()` is now the **marquee thread body** (not "the only loop"), and `Scheduler::tick()`
keeps its v2.3 signature *and* semantics but takes `mu_` internally and is only called by the worker (with
`ev == nullptr`) or by a test.

**Changed in v2.5 (§10.11):** `Scheduler::started_` is **removed** (`worker_.joinable()` already is the state);
`Scheduler::eventOwed_`, `noteEventLocked`, `appendMeasure`, the marquee-thread-private `std::ofstream
measure_`, and `Parameters::measurePath` are **added**; the file-local helpers `frameLine(index, now)` and
`eventLine(eventMs, echoMs)` format the two `--measure` line kinds. `Scheduler::tick(const KeyEvent*)` is now
labelled **test-only** in the header contract, with `tick(nullptr)` the only production call.

**Changed in v2.6 (§10.12):** `TickResult::quit` is **removed** — `TickResult` is now `{ bool rendered; }`, and
`&Scheduler::run` no longer reads it; the worker no longer calls `Interpreter::quitRequested()`, so `quit_` is
input-thread-only. No new names.

### 10.4 Deliberate deviations from this project's stated process rules

- **Feature-Sliced Design** targets frontend projects; this is a C++ terminal program. Only its invariants
  (layer order, downward-only imports, explicit contracts) are applied (§3.1). No `pages/`/`widgets/` layers.
  The review's proposed flattening was rejected — `AGENTS.md` mandates these layers and the file count is
  identical either way.
- **`npx @tanstack/intent list`** was run from the workspace root: no intent-enabled packages found, so no
  local skill overlays this plan.
- **CLion**: researched, not assumed — JetBrains documents CMake presets (read-only, disabled by default) and
  states that output-console terminal emulation *"depends on the OS, debugger, and toolchain"*, with MSVC+LLDB
  supported on Windows and GDB not supported. The plan therefore keeps a **verify-don't-assume gate** plus the
  T0.5b fallback instead of claiming the IDE console is always a real tty.
- The plan lives at the repo root because the group asked for it there; `docs/` is for evidence.

### 10.5 Explicitly out of scope (P2, only after the §T6.3 freeze)

1. **pty replay harness** (`tests/replay/`, `script`-based transcript diffs). Technically interesting, brittle,
   and macOS `script` syntax differs; unit + acceptance + smoke tests are the required evidence.
2. **macOS manual verification pass** (`Terminal.app` vs `iTerm2`). macOS keeps compiling in CI regardless.
3. `--self-test` running A1–A10 automatically; a live `ps` view inside the band showing both threads and the
   shared-state snapshot.

### 10.6 Revision log (v1 → v2)

Full adjudication with evidence: **`REVIEW_ADJUDICATION.md`**. Summary of what changed and why:

| Change | Reason |
| --- | --- |
| `Scheduler::run()` owns input; `tick(const KeyEvent*)` is pure; echo no longer gated on `refreshMs` | v1 double-polled (two keystrokes per cycle possible) and its T1.3 test could never pass |
| `MarqueeProcess::hasRendered` added; `due` is `!hasRendered \|\| now - lastRenderMs >= refreshMs` | v1's condition could not render at `now == lastRenderMs == 0`, so its "first frame immediately" assertion was unreachable and `start_marquee` would show a blank band for one whole interval |
| T4.1 tests corrected; glyph width-invariant test added | v1's `17` was arithmetically wrong (true: 29) and its `" ###  ####"` literal was 10 chars vs 11 actual |
| One whitespace rule for `set_text` | v1's trim rule contradicted acceptance test A4 |
| `Welcome to CSOPESY!` capitalised; `version_date` defaults blank; band layout parameterised | Transcribed from the handout's own mock (§1.4) |
| Signal handler sets a flag only | Calling `tcsetattr`/`SetConsoleMode` from a handler is not async-signal-safe |
| Runbook: IDE-only evidence; `.bat` demoted to rehearsal | v1's runbook contradicted the handout's video rule |
| `--dev` (`ps`/`params`/`stats`) → single `--diag`; CSV telemetry → `--measure=FILE` | Removed the T4.6 dangling reference and the telemetry framework |
| `scheduler=threaded`, `step_cols`, `Ready`/`Finished`, `quantumMs`, PCB `asciiArt`, `--self-test`, live `ps`, committed binary, `lukka/get-cmake` removed | Over-engineering; nothing in the handout needs them. *(v2.4: threading returns as the **architecture** — §3.8, §10.10 — but still not as a config knob, and `Ready`/`Finished`/`quantumMs` stay deleted.)* |
| Windows: VT **input** off, `WaitForSingleObject` for the read timeout | Enabling VT input breaks `_getch` key delivery; `_kbhit` cannot block |
| `marquee_row`, `--diag`, `--measure`, the 4-machine matrix, the FSD layers **kept** | Each has a specific job; removing them all (as the review proposed) would leave no automated end-to-end evidence and no PPT numbers |
| Response strings simplified; tests assert tokens + exact `Usage:`/goodbye lines | The handout never specifies wording, so byte-exact assertions would freeze guesses |

### 10.7 Revision log (v2 → v2.1) — the second review round

Adjudicated in `REVIEW_ADJUDICATION.md` §6. Five changes come from the reviewer; six defects were found in
v2's own text while applying them (two of the rows below were simplified again in v2.2 — see §10.8).

| Change | Reason |
| --- | --- |
| Split `csopesy-dev` (CMake target) from `csopesy-quiz` (Custom Build Application → `frozen/csopesy.exe`, **empty** build target, no Build step); T6.3 Step 0 re-verifies the hash after a Run press | v2 said the run config "targets `csopesy`" *and* must "execute the same binary copied into `frozen/`" — those are not the same file. JetBrains documents *"Build is the default pre-launch step for CMake applications"*, so a CMake-target config can relink at Run time, which is exactly what the frozen-build rule forbids |
| `polling_ms` documented as a **maximum idle wait**; new "what it does and does not do" paragraph; `poll_timeout_is_capped_by_the_render_deadline` test | `poll()`/`WaitForSingleObject` return the instant a key arrives, and the timeout is additionally capped by the render deadline, so `polling_ms` cannot delay a keystroke or a frame. It bounds idle wakeups and (on Windows) Ctrl+C-to-exit |
| Metric renamed to **input-to-echo processing latency**; perceived typing delay becomes a separate human-observed column | `eventMs` is when `readEvent` returned, so `--measure` never sees the physical keypress; calling it "key→echo latency" would be a false claim in a graded report |
| Input line scrolls **horizontally** (`visibleSlice`) instead of wrapping to multiple rows | A wrapped prompt cannot coexist with a fixed-row frame layout without displacing the band/header rows; horizontal scrolling is a `substr` window with no cursor-row bookkeeping |
| "Not observed within the tested range" is an allowed measurement result | The handout asks us to identify the limits *where* tearing or delay is present; it does not guarantee both exist. Inventing a threshold to fill a slide would be a fabricated result |
| Windows: compare `srWindow` **before** the `!_kbhit()` shortcut and return a real `Resize` event | v2's snippet returned `false` on `!_kbhit()`, silently stranding the layout at the old width — nothing else in the loop ever resized the buffer. **Superseded in v2.2** (§10.8) |
| POSIX: on `EINTR`, check the resize flag first, then return `false` only if it is unset | v2 said "return `false` on timeout or `EINTR`" *and* "`SIGWINCH` ⇒ the next `readEvent` returns `Resize`". `SIGWINCH` is what makes `poll` return `EINTR`, so the two statements cannot both hold — resize was swallowed. **Superseded in v2.2** (§10.8) |
| `Scheduler::tick` resizes the `FrameBuffer` every tick; a resize test added (v2.2: it must also set `redraw`) | Nothing in v2's loop ever called `FrameBuffer::resize()`, so T4.3's resize test covered a path that was never wired up |
| `enterRawMode()` given a stated call site: the RAII guard constructed in `main`, skipped when `noTty`/`!isTty()` | v2 described `enterRawMode()` and its guard in two places but never said who calls it, so the contract was unassignable |
| `set_speed`'s argument must match `[-+]?[0-9]+` in full; `-5` clamps rather than erroring | `std::stoi("5abc")` silently returns 5 (a latent bug), and v2 never said whether `-5` is a `Usage:` error or a clamp while A5 implies the clamp |
| `--no-tty` / `isTty() == false` defined as **plain line mode**, plus a CI end-to-end smoke test of the real binary | v2 claimed "no ANSI" for non-tty while `FrameBuffer::renderDiff` always emits cursor addressing — the mode was undefined. The smoke test is the only CI check that exercises the actual binary on all three OS images |

### 10.8 Revision log (v2.1 → v2.2) — the third review round

Adjudicated in `REVIEW_ADJUDICATION.md` §7. Two changes are corrections; the rest are simplifications that
*delete* machinery earlier rounds had added.

| Change | Reason |
| --- | --- |
| CMake floor **3.20 → 3.21**, in the stated floor, in `cmake_minimum_required` and in the CI comment | The plan's own `CMakePresets.json` uses `"version": 3`, and CMake documents every schema-v3 feature (`condition`, `installDir`, omitting `generator`) as *added in 3.21*; the `Visual Studio 17 2022` generator is 3.21 too. A stated floor below what the project's own file requires fails on the one machine that has 3.20 |
| `Scheduler` moves `src/entities/` → `src/app/` | `scheduler.hpp` includes `renderer.hpp` and holds `Renderer&`/`Interpreter&`, so `entities/` would import *upward* into `features/` (FSD rule 4-1), and a `features/` home would cross-import two `features/` slices (rule 4-3). `app/` is the composition root and the only legal home; no interface layer was added to keep the old path (§3.1) |
| Resize is **detected, not delivered**: `KeyType::Resize` removed; both backends just expose live `size()`; `tick()` sets `redraw` when it changes | Windows console resize records require `ENABLE_WINDOW_INPUT` + `ReadConsoleInput` and are *filtered* from the `ReadFile`/`ReadConsole` path a CRT `_kbhit`/`_getch` backend uses, so v2.1's `srWindow`-compare could never fire; POSIX needed a flag to defeat its own `EINTR` rule. Deleting the event removes both platform mechanisms, one enum value and one self-contradiction. It also exposed a real bug: v2.1 resized the buffer on a size change and then **did not repaint it**, so an event-less resize showed the old layout until the next key or frame |
| CI smoke test: `printf … \| ./build/debug/csopesy` → a CTest job using `execute_process(INPUT_FILE)`, registered at T2.5 where the plain line mode it asserts finally exists | GitHub Actions' default shell on Windows is `pwsh`, so `printf` is not a command there; and the binary path differs per generator (VS `build/vs/Debug/…` vs Ninja `build/debug/…`). The check was meant to prove portability across three OSes while itself being Linux-only shell with a generator-specific path. Registering it in T0.2 (as first drafted) would also have left CI red for two whole phases, because the stub `main` cannot print a help table |
| `scripts/check_layers.sh` added (T0.1, T0.3) | §3.1 claimed the layer rules were "enforced" with no mechanism — which is precisely why the `entities/scheduler.cpp` violation survived three review rounds of prose |
| Documentation fixes: "five further defects" → **six** (adding `enterRawMode()`), §10.7's "four are defects" → **six**, T2.3's commit message "wrapping" → **horizontal scrolling**, T0.4's freeze marker `contracts v2.0` → **v2.2** | The plan is the team's source of truth; an internal count that contradicts its own adjudication table makes every other count suspect, and a commit message that names a rejected design misleads the reviewer reading the history |

### 10.9 Revision log (v2.2 → v2.3) — the fourth review round

Adjudicated in `REVIEW_ADJUDICATION.md` §8. Two corrections that would otherwise have cost real time, two
cleanups, and one process rule made honest.

| Change | Reason |
| --- | --- |
| `add_compile_options()` rewritten as `if(MSVC) /W4 else() -Wall -Wextra endif()` | The combined generator-expression one-liner is **split by CMake's argument parser at the space inside it**, so `-Wextra>` is passed to the compiler as a literal flag: CMake configures happily, then the build dies with `c++: error: unrecognized command-line option '-Wextra>'`. Reproduced on CMake 4.3 + GCC 16 while adjudicating this round — it would have blocked T0.1 at the first build, on every non-MSVC machine at once. T0.2 now carries a comment forbidding "simplification", and T0.2 Step 1 asserts the flags really reach the compile line, because this failure is silent at configure time |
| Quiz-case selection made explicit in §3.6, §7 (T-0:02) and T6.3 | The runbook told the operator to sanity-check `config/quiz_case_<n>.ini` while the graded Run loads the literal `--config=config/csopesy.ini`, and nothing said how one becomes the other. Because a missing file is non-fatal **by design**, a stale `config/csopesy.ini` would have run the *previous* case's parameters with no error at all — a silent wrong answer during a timed quiz. The fix names the copy step as the case selector and adds a stale-config check to the T6.3 dry run |
| "Every task is TDD" and DoD item 1 get a stated carve-out for T1.1/T3.1/T0.5 | §5, §6.1 and §6.3 demanded a failing unit test for *every* task while T1.1/T3.1 ship hand-run checklists — an internal contradiction that invites either theatrical tests or ignored rules. Those tasks now say plainly that the recorded terminal transcript **is** their test |
| T1.1's resize bullet cut back to the instruction; the §10 navigation row corrected | The v2.1 `SIGWINCH`/`Resize` post-mortem belongs in the review record (`REVIEW_ADJUDICATION.md` §6.3/§7.3), not in the coding instructions; and §0 still described §10 as "v1→v2" three revisions later |

### 10.10 Revision log (v2.3 → v2.4) — the architecture conversion to threads

Requested by `threading.md`, after the professor's own example program showed an animated display that stays
interactive while the user types. **The observable contract was not changed.** The pure `Scheduler::tick()`
step was deliberately preserved so the existing deterministic tests survive verbatim: this design *added*
tests instead of rewriting them, which is the strongest available evidence that the architecture changed and the
behaviour did not.

| Section | Was (v2.3) | Now (v2.4) |
| --- | --- | --- |
| Header *Architecture* + §10 status | "single-threaded cooperative loop" | two threads, one mutex, one condition variable, one terminal writer |
| §3.1 | `app/` hosts the composition root | + a note that the worker thread, mutex and cv are `app/` implementation detail and change no layer rule |
| §3.3 `Terminal` | no concurrency statement | + a concurrency clause: `readEvent` from the input thread, `write`/`flush`/`size` from the marquee thread; **no backend changes**; `FakeTerminal` does change |
| §3.5 `Parameters` | not described as shared | + the shared-state rule (one writer; cross-thread readers under `mu_`) |
| §3.6 `--measure`, `--no-tty` | both write sites on one thread | + the `eventMs,echoMs` pair is completed on the marquee thread; `--no-tty` starts **no** worker |
| **§3.8** | `run()` owns `readEvent`; one loop; "Cooperative is the only scheduler" | the two-thread model, the ownership table, the two lock rules, `start`/`run`/`postEvent`/`wake`/`requestStop`/`join`, the cv-based bounded wait, and the "what threading does not buy" paragraph. `tick()`'s signature and semantics are unchanged |
| §3.9 | tearing wording | + "one owning thread" strengthens the *application-level* claim only; still explicitly **not** an atomic-repaint claim |
| §3.10 | "`Scheduler::run()` polls the flag each iteration" | the five-step shutdown sequence, with the join **before** `restore()`, and the input thread as the flag's only reader |
| §4.1, §4.5 | scheduler owner, 8 h budget | threading deliverable `docs/threading-model.md`, ~10 h; a RACI row added |
| §5.0 D1 + cut line | T1.3 in the day's list; 5 cut items | T1.3 marked the critical path; cut item 6 added as an **explicit group decision** (revert to single-threaded), flagged as forfeiting the reason for the change |
| T0.1 / T1.1 | `FakeTerminal` was a single-threaded double | thread-aware double with exact synchronization counters, and **no sleeps** |
| T0.2 | `std::thread` unlinked | `find_package(Threads REQUIRED)` + `Threads::Threads`, and `unit` gets `TIMEOUT 60` so a missed join is red, not a hang |
| T0.4 | freeze marker `contracts v2.3` | `contracts v2.4` |
| T1.1 | `tick()` polls the shutdown flag; no I/O thread rule | the input thread polls it and is its only reader; the terminal read/write split is stated, with a warning not to "fix" it with a lock |
| **T1.3** | "Deterministic cooperative scheduler" | "Threaded scheduler: marquee worker + preserved pure step"; Step 1 **unchanged** + Step 1b concurrency tests + Step 5 optional TSan/ASan + Step 6 `docs/threading-model.md` |
| T2.5 | `ConsoleApp::run()` "delegates the loop to `Scheduler::run()`" | ConsoleApp **is** the input thread: `start()` → read/post loop → `requestStop()` → `join()` → goodbye → restore; plain line mode starts no worker |
| T2.6 | README had no threading statement | + a ≤5-line threading paragraph and a pointer to `docs/threading-model.md` |
| T3.1 | Ctrl+C latency explained via "the next tick" | explained via the input thread's poll; explicitly forbids notifying the cv from the control handler |
| T6.1 | scheduler slide "states that threads were a deliberate non-goal" | scheduler slide presents the threaded model + ownership table + the two honest limits |
| §6.1, §6.2 | DoD said nothing about real-time sleeps | the threading tests must contain no `sleep_for`/`sleep_until` (DoD 2); A6 pins "exits promptly, no hang on join"; A8 pins "no frame interleaved or garbled" |
| §8, §9 | risk 9 = "threads expected ⇒ graded down", mitigated by *not* using threads; Q3 asked permission | risk 9 inverted (threads are implemented); risks 21–25 added (deadlock, worker outliving objects, `pthread` link, doubled idle wakeups, behaviour drift); Q3 now confirms the shape and asks only whether one marquee thread is enough |
| §10.1, §10.3, §10.5, §10.6 | no concurrency row; no v2.4 names; a threaded scheduler listed as out-of-scope P2; `scheduler=threaded` recorded as removed | concurrency row added; new names listed; the P2 item removed (threading *is* the architecture); the v1 removal row annotated |

**This is the one place the plan changed without a failing test as its trigger**, which §10.9 said would not
happen. The justification is external and checkable: `threading.md` is a direct instruction from the owner, and
the change is additive by construction — the existing tests, the six commands, the config/CLI precedence,
`--no-tty`, `--diag`, `--measure`, the layer guard, the FSD layout, the frozen-binary quiz workflow and the
video requirement are all untouched, and §6.1's DoD still requires T1.3's tests to fail first and then pass.

**Status (the v2.4 sub-claim corrected in v2.5).** v2.4 is frozen for *paper* review — it supersedes v2.3 on the
scheduler architecture only — but the phrase "implementation-ready" was doing more work than the evidence
supports, exactly as `gpt-v5.md` argued in closing: **not one line of the threaded scheduler above has been
compiled on any machine.** Races, use-after-free, missed wakeups and deadlock freedom are therefore *claims
argued from the §3.8 ownership table and exercised on paper*, not observations, until T0.1's green `ctest` and
T1.3's Step 1/1b/1c runs exist. v2.3's rule stands for everything future: this file changes on *evidence* — a
green `ctest` on the owner's hardware, a compile on all three CI images, a terminal transcript, or a failing
acceptance case. The next action is still T0.1, and T0.1→T0.2 must precede T1.3 because the thread-aware
`FakeTerminal` and `Threads::Threads` are prerequisites for it.

### 10.11 Revision log (v2.4 → v2.5) — the threading reviews (rounds 5–6)

`gpt-v5.md` reviewed `threading.md` + v2.4 as a design review (deliberately without the source, which does not
exist yet); `gpt-v6.md` reviewed the same against the handout and asked for a leaner design. Adjudicated in
`REVIEW_ADJUDICATION.md` §9–§10. **No architectural change is involved**: the two-thread / one-mutex / one-cv /
one-terminal-writer model is untouched, and every accepted point is a contract or critical-section correction.
The plan did **not** shrink to v6's "minimum safe" sketch, and the reasons are in the rejection rows below.

| Point (review) | Verdict | What changed |
| --- | --- | --- |
| v5 §1: `--measure` claims "one line per keystroke" but admits coalescing | **A** (and sharpened) | §3.6 rewritten: one pair per **echo frame**, **oldest** stamp, emitted once, and a frame line only for a `due` frame. Two defects v5 did not name are fixed with it (Y3, Y4 below) |
| v5 §2: the mutex is held across rendering | **A′** | §3.8 gains rule 3 (`size()` read *before* the lock) and a "what the lock actually costs" paragraph; the snapshot-then-render-outside alternative is **rejected** as scope, not as correctness |
| v5 §3: the concurrency tests do not show concurrency | **A** | Step 1b gains `threaded_input_thread_and_animation_overlap_in_real_time` (a real second thread, barrier-stopped, no sleeps); A8 becomes the named real-concurrency acceptance case that must record the observed frame count; DoD 7 and §6.3 state what the deterministic suite does *not* prove |
| v5 §4: `start()` is not exception-safe | **A′** | Solved by **deleting** `started_` rather than reordering it: `worker_.joinable()` already is the state, so the spawn is exception-safe by construction |
| v5 §5: `joinable()` is safe only under strict ownership | **A** | Stated in the header comment, the ownership table and the snippet: `worker_` is input-thread-only, and `join()` from `run()` is forbidden |
| v5 §6: the `tick(ev)` compatibility overload invites drift | **A** | Labelled TEST-ONLY in §3.8, T1.3 Step 1 and §10.3; both paths share one locked body |
| v5 §7: keep the cv/timed-wait model, no `sleep_for` | **✓** | Unchanged |
| v5 closing: "frozen for paper review" ≠ implementation-verified | **A** | §10.10's status reworded: races/lifetime/deadlock freedom are unverified claims until T0.1 + T1.3 land |
| v6 "remove": delete `wakeTick_`, drop `SchedulerSnapshot`, trim the concurrency matrix, de-scope FSD/CI/`--diag` | **R** | `wakeTick_` is what lets Step 1b force a step with no real-time wait (removing it breaks DoD 2); `SchedulerSnapshot` is the only lock-safe live read for tests/`--diag`, and objecting to the *debugger* is a double standard; each concurrency test maps to a `threading.md` requirement (1–9, 11); FSD/CI/`--diag` were adjudicated in rounds 1 and 3 and are not threading scope — v6 says as much itself |
| v6 "simplify `--measure` to `timestamp,frame_number` and measure by hand" | **R** (partly A) | The implementation is ~5 lines and auto-generates two of T1.4's four columns; perceived typing delay *is* manual and stays so. Cutting the metric deletes T1.4 columns 1–2 |
| v6 "don't build measurement/test machinery" and "don't float thread-per-process" | **✓ / A** | The record is a table plus a script, not a subsystem; §9 Q3 no longer floats a thread-per-process P2 |
| v6 "the two threads are 🟡, not required by the PDF" | **✓** | Recorded, not acted on: `threading.md` is the owner's direct instruction, and §1.2/risk 9 already carry the "defensible implementation story" framing |

**Four defects the reviews missed, fixed in this round** (the same class as the ones they found — two in the
exact `--measure` area v5 was reviewing):

| # | Defect in v2.4 | Evidence | Fix in v2.5 |
| --- | --- | --- | --- |
| Y1 | **The `--measure` file had no plumbing.** The snippet did `measure += …` against a stream that does not exist, and no task said how `--measure=FILE` reaches the writer | §3.5's `Parameters` had no path field, §3.6 named the flag, §3.8's class had no stream | `Parameters::measurePath` (infra, CLI-only, written before any thread exists) + the marquee-thread-only `std::ofstream measure_`, opened lazily |
| Y2 | **`tick()` held `mu_` across `Terminal::size()`** — an `ioctl`/`GetConsoleScreenBufferInfo` syscall — even though the ownership table assigns `size()` to the marquee thread exclusively | §3.8's ownership table vs its own snippet | Rule 3: read `size()` before the lock; only the `fbRows_`/`fbCols_` comparison stays inside |
| Y3 | **The event stamp was lossy and sticky.** `postEvent` overwrote `pendingEventMs_`, so N queued keys reported the **newest** (understating worst-case latency), and nothing ever cleared it, so the pair would repeat on **every** later frame | §3.8's own `postEvent` and the un-cleared field | `noteEventLocked` keeps the **oldest** stamp; `eventOwed_` makes the pair one-shot. A `0` sentinel is *not* safe, because the injected clock legitimately starts at `0` (Step 1 uses `clock = 0`) — hence a bool |
| Y4 | **The frame line's write site was ambiguous** — its comment sat inside `if (due \|\| redraw)` while the contract says "one line per rendered frame", so echo frames would have entered the refresh-cadence series | §3.6's metric intent vs §3.8's comment placement | The frame line is emitted **only** under `if (due)`; the echo frame emits the event pair and no frame line |

#### Verification performed before committing v2.5

The only algorithmic change is the `--measure` rule. It was exercised against a reference implementation in
simulation (the same standard as §7.5/§8.5): **11 checks, all passing**, reproducing both new Step 1c tests and
the v2.4 counterfactual:

```text
frame_line_only_for_due_frames: 2 lines total                     PASS
  ...and they are F0,0 and F1,100                                 PASS
  ...the non-rendering tick wrote nothing                         PASS
pair_oldest_once: exactly 3 lines                                 PASS
  ...pair carries the OLDEST stamp (10)                           PASS
  ...not the last key (20)                                        PASS
  ...and the pair is emitted exactly once                         PASS
counterfactual v2.4: oldest stamp is LOST (understates latency)   PASS
counterfactual v2.4: reports the last key instead                 PASS
counterfactual v2.4: the pair REPEATS on a later frame            PASS
counterfactual v2.4: 4 lines where v2.5 writes 3                   PASS
```

The counterfactual rows are the point: restoring v2.4's overwrite-and-never-clear makes the new tests fail, so
they are not vacuous and **Y3 was a real defect, not a theory.**

**This is still simulation, not verification** (§6.3). It shows the specified `--measure` rule is
self-consistent. It does **not** prove the C++ compiles, that the two threads behave, that TSan is clean, that
`size()`-before-the-lock removes the readiness problem in practice, or that a real terminal renders anything.
The `size()` hoist (Y2) and the lock-cost paragraph are *arguments*, not measurements; T1.4's sweep is where
their cost is finally observed. That is exactly the distinction `gpt-v5.md`'s closing warning was about, and it
is now stated in the plan rather than glossed over.

### 10.12 Revision log (v2.5 → v2.6) — the seventh review run (round 7)

`gpt-v7.md` is the first review that read `REVIEW_ADJUDICATION.md` itself. It reviewed the revised v2.5 plan,
found **one** issue to fix before implementation, explicitly endorsed keeping the machinery rounds 5–6 had
argued for, and closed by advising us to stop reviewing the paper and start compiling. Result: **v2.6** — one
accepted change, one **false positive corrected**, and its closing advice adopted as the status rule below.

| Point (review) | Verdict | What changed |
| --- | --- | --- |
| v7: "`quit_` still has two unsynchronized readers — a real concurrency defect that must be fixed before implementation" | **A′ — direction accepted, reason rejected** | The worker's `tick()` no longer reads `interp_.quitRequested()`, so `quit_` is now input-thread-only. But there was **no data race** to repair (below); the change is taken because it deletes a hedge, not because it fixes a race |
| v7: "production `run()` should not depend on `TickResult::quit`" | **A** | `run()` exits solely on `stop_`; `TickResult::quit` is removed (no test ever read it — see the static check) |
| v7: "keep `wakeTick_`, `SchedulerSnapshot`, the single `--measure` mechanism, the platform CI, the FSD structure" | **✓** | Nothing reverted; §10.11's rejections stand and are now record-backed |
| v7: "the mutex scope, measurement design, `started_` removal and concurrency evidence are all now sound" | **✓** | No change |
| v7 closing: "don't keep reviewing the paper until it is perfect; once shared-state ownership is consistent, move to C++ and let the compiler/tests/TSan find the next problems" | **A** | The status below: this is the **last paper revision**; the next edit to this file is an evidence edit |

**Why the filed defect was a false positive.** A data race needs two accesses to the same object, at least one a
write, on **different** threads, with no ordering between them. `quit_` had exactly three accesses, and no pair
was unordered:

| Access | Thread | Lock held? |
| --- | --- | --- |
| **write** — `feed`/`executeLine`, reached through `postEvent` | input | **yes** (`mu_`) |
| read — the observe step `interp_.quitRequested() \|\| shutdownRequested()` (§3.10 step 2) | input | no |
| read — `res.quit = interp_.quitRequested()` in `tick()` | worker | **yes** (`mu_`) |

Pairwise: (write, input-read) are the **same thread**, so they are sequenced-before and need no lock at all;
(write, worker-read) both hold `mu_`, so the mutex orders them — this is the only pair that needed synchronizing,
and it was synchronized; (input-read, worker-read) are two reads, which can never race. v7's statement that "the
mutex does not synchronize those two accesses" is true only of the two **reads**, which never needed it. This is
the **third** round in which a correct direction arrived attached to an incorrect reason (cf. §7.2's W1 and
§8.3): the fix is adopted, the diagnosis is corrected rather than accepted.

**Why the fix is still worth taking.** "Correct because the sole writer happens to hold the lock" is precisely
the kind of argument a 7-day student project should not have to defend in a graded report. Deleting the worker's
read makes `quit_` single-threaded outright, gives the ownership table a row with no caveat, shortens `run()`,
and removes the last place in §3.8 that needed a footnote — a strictly smaller model for identical behaviour.

#### Verification performed before committing v2.6

No algorithm changed this round: it deletes one read and one return field. The evidence is a **static check**,
and it is the entire justification for removing `TickResult::quit`:

```text
# 1. Nothing left accesses the field: the only matches are prose/comments about the removal.
grep -n 'res\.quit\|if (tick(.*)\.quit' IMPLEMENTATION_PLAN_v2.md
  → 0 code hits (only the v2.6 header note, the §3.8 comment and the risk-28 row describe the removal)

# 2. Every preserved test already discarded tick()'s return, so none asserted `quit` and none needs editing.
grep -n 'sched\.tick(' IMPLEMENTATION_PLAN_v2.md
  → 25 hits, every one of them `(void)h.sched.tick(...)`
```

So the Step 1 / 1b / 1c test blocks stay **byte-for-byte unchanged**, which is what keeps §10.10's central claim
("the architecture changed; the tests did not") true through v2.6. `TickResult::rendered` is kept as the pure
step's observable result; it has the same discarded-return status today, and it is retained because it is the
contract's answer to "did this step emit a frame?", not because anything currently reads it.

**This is still not verification.** Nothing was compiled. Races, lifetime and deadlock freedom remain claims
argued from the §3.8 ownership table until T0.1's green `ctest` and T1.3's runs exist — which is the last thing
`gpt-v7.md` said, and it is correct.

#### Status — paper review is closed

**v2.6 is the last paper revision.** Every round so far has answered a reviewer; there is no reviewer left whose
concern is not either adjudicated with evidence or listed in §10.11/§10.12 as rejected with a reason. The plan's
own rule (v2.3, §10.9) is now the only rule that applies: **this file changes on evidence** — a green `ctest` on
the owner's hardware, a compile on all three CI images, a terminal transcript, or a failing acceptance case.

The next action is **T0.1**, then T0.2, then T1.3 exactly as written. The threaded scheduler's safety is not
established by this document and was never claimed to be; it will be established, or falsified, by
`cmake --preset debug`, `ctest`, and — where the toolchain has them — TSan and ASan.
