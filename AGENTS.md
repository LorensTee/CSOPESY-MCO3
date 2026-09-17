# AGENTS.md — instructions for coding agents in this repository

**Project:** CSOPESY MCO3 — "Marquee Console". A C++17 terminal program (one animation process + a live
command prompt), a 4-member group submission due **2026-09-23**. `src/main.cpp` is the graded **entry file**;
`README.txt` is required by the handout. Architecture is **two threads**: an input/command thread (`main`)
and a marquee worker, one `std::mutex`, one `std::condition_variable`.

**Current state: Phase 0.** `include/csopesy/*.hpp` are frozen contracts; `src/**` and `tests/**` are stubs
and partial implementations. Do not assume a task is done because the file exists — check
`docs/IMPLEMENTATION_PLAN_v2.md` §5 for the phase it belongs to.

## 1. Sources of truth (read before you write)

| Question | Answer |
| --- | --- |
| How should this be built? | `docs/IMPLEMENTATION_PLAN_v2.md` — the authoritative plan (v1 is gone; do not implement from it) |
| What is the interface? | `CONTRACTS.md` + `include/csopesy/*.hpp` — **frozen** |
| Why is it like this? | `docs/REVIEW_ADJUDICATION.md` — every accepted/rejected review point, with reasons |
| What is graded? | plan §1 (spec decode) and §6 (definition of done, acceptance cases A1–A10) |
| CLion toolchain / run configs | `docs/clion-toolchain.md`, `docs/clion-run-config.md` |

The course handout PDF and `docs/replies/` are **deliberately untracked** (see `.gitignore`) — do not add them.
Before re-litigating a design decision, search `docs/REVIEW_ADJUDICATION.md`: it usually already records the
answer, including decisions taken *against* a reviewer's advice.

## 2. Commands

```bash
# build + test (verified on this machine: CMake 4.3, GCC 16.2, Ninja 1.13)
cmake --preset debug
cmake --build --preset debug
ctest --preset debug

# the layer guard — run this before every commit; CI runs it too
bash scripts/check_layers.sh          # prints "check_layers: OK (N files scanned)" or fails

# contract freeze check
git hash-object include/csopesy/*.hpp # compare against the table in CONTRACTS.md
```

CMake floor is **3.21** (presets schema v3 + the `Visual Studio 17 2022` generator both require it).
`CMakePresets.json` is committed and shared; **presets are read-only in CLion**, and local overrides go to the
gitignored `CMakeUserPresets.json` — never edit machine-specific paths into the shared file.
CI (`.github/workflows/ci.yml`) does *not* use presets: it runs `cmake -S . -B build`, then
`cmake --build build --config Debug`, then `ctest --test-dir build -C Debug --output-on-failure`, on
Ubuntu / Windows / macOS.

## 3. Layer rules (FSD-adapted — enforced, not advisory)

```text
app/  >  features/  >  entities/  >  shared/        (downward-only imports)
platform/  is a peer of shared/ and may import shared/ ONLY
```

- `features/marquee/` and `features/commands/` must not cross-import each other.
- Test doubles (e.g. `FakeTerminal`) live in `tests/support/` — never under `src/` or `include/`.
- Contract headers are **flat** in `include/csopesy/`, so a header's layer is not inferable from its path:
  the authoritative map lives inside `scripts/check_layers.sh` (`layer_of_header`).
  **Adding a new contract header without adding it to that map is a hard failure** — the guard must never
  silently stop guarding a file.
- `Scheduler` lives in `app/` (it holds `Renderer&` and `Interpreter&`), not `entities/`.

## 4. Contracts are frozen (v2.6, 2026-09-16)

Everything under `include/csopesy/` is an interface contract: signatures, struct fields, constants, and the
semantics attached to them (ownership, locking, one-writer rules, error behaviour).

Change a contract only by the §4.5 protocol: **propose in chat → the freeze owner (W2 Byron) and the affected
owner agree → bump the `CONTRACTS.md` marker → announce.** Never by a silent edit — a mid-phase header change
invalidates teammates' builds on machines you cannot fix.
Re-freezing means regenerating the `git hash-object` table **in the same commit** as the header change.
A tool (formatter, `clang-format`, an IDE "optimize includes") that rewrites a frozen header **is** making a
contract change; re-running a tool is not an exemption.

**One writer per file at any time.** RACI is plan §4.5.

## 5. Threading invariants (do not break)

- The **marquee worker is the only terminal writer.** The input thread never writes a frame.
- Production scheduler coordination uses exactly **one `std::mutex` and one `std::condition_variable`**.
  Do not add another production synchronization layer. Test doubles (`FakeTerminal`) deliberately carry their
  own mutex/condition variables so the threaded tests can be deterministic — that is expected and is not the
  production coordination layer. No syscall inside the critical section: `Terminal::size()` is read *before*
  taking the lock; `--measure` appends happen *after* releasing it.
- `Terminal::nowMs()` is the **only** clock source, injected into `Scheduler`. Do not call `std::chrono`
  directly in scheduler logic.
- `Interpreter::quit_` is read and written by the **input thread only**; the worker stops via `stop_` +
  condition variable.
- **No `sleep_for` / `sleep_until` in concurrency tests.** They must be deterministic (barrier-synchronized,
  driven input); a real-time sleep is a flake waiting to happen. T1.3 Step 4 runs
  `grep -n 'sleep_' tests/unit/test_scheduler.cpp` and expects no output — that grep is a *task step*, not a
  CI job (CI runs the layer guard and `ctest`).
- A green deterministic suite does **not** prove two activities overlapped in real time. Concurrency claims
  need the real-overlap test plus the A8 transcript (plan §6.1 item 7, §6.3).

## 6. Landmines — do **not** "simplify" these

- `CMakeLists.txt` warning flags use a plain `if(MSVC)/else()`. The tempting one-liner with two generator
  expressions is split by CMake's argument parser at the space inside it: it *configures* cleanly, then fails
  at compile time with `unrecognized command-line option '-Wextra>'`. The comment there says so.
- `find_package(Threads REQUIRED)` + `Threads::Threads` (PUBLIC) is what makes the POSIX build **link**.
  Without it the compile succeeds and only the link fails, on a machine you may not be holding.
- The 60 s `ctest` `TIMEOUT` on the `unit` test converts a missing `join()` from a hung CI job into a red test.
- `tests/support/check.hpp` is a ~20-line zero-dependency assertion harness. **Do not add GoogleTest or any
  other framework** — zero install friction on four machines, no network, works on all three CI images.
- `.gitignore` intentionally **tracks** `.idea/runConfigurations/` (they are the graded run configuration) and
  intentionally **ignores** `/docs/replies/`, the handout PDF, `build/`, and `frozen/`.

## 7. Behaviour contracts that tests assert (do not drift)

- **Command strings are exact** — plan §3.7. Match commands **case-sensitively** (`HELP` is unrecognized);
  trim the argument's outer whitespace but preserve internal runs verbatim; `set_speed`'s argument must match
  `[-+]?[0-9]+` in full (so `5abc` is a `Usage:` error, never a silent partial parse via `std::stoi`); a
  well-formed out-of-range value (including `-5`) is **clamped and reported**, not a usage error.
- **Precedence** is defaults → `config/csopesy.ini` → CLI flags. Bad input **warns and keeps going**; a missing
  config file is never fatal (a typo must not cost the quiz).
- **`config/csopesy.ini` is the single live path.** Quiz cases (`config/quiz_case_<n>.ini`) are inputs copied
  *over* `config/csopesy.ini`; nothing loads them directly, and the graded run config passes
  `--config=config/csopesy.ini` literally.
- `--no-tty` means plain line mode: no raw mode, no ANSI, no animation, **no worker thread** — it is what makes
  the real-binary smoke test portable across the three CI OS images.
- `--diag` is the only diagnostic flag (one-shot report to stderr, exit 0); `--measure=FILE` has two distinct
  line kinds with an explicit coalescing rule — read plan §3.6 before touching it.

## 8. Definition of done

1. The task's test **failed before** the change and passes after (`ctest` output pasted into the PR).
   Sole carve-out: the **platform-surface tasks** (T1.1 termios, T3.1 Win32 console modes, T0.5 CLion terminal
   gate) — no unit test can observe raw-mode restoration or a live resize; do the hand-run checklist on the
   owner's hardware and record the result. Do not invent a test-shaped ritual to satisfy the wording.
2. CI green on all three OS families.
3. No frozen header changed without §4.5.
4. Demonstrated on the owner's own hardware (transcript/screenshot in the PR).
5. The commit message names the requirement (R1–R9) it advances.
6. `bash scripts/check_layers.sh` passes.

**Simulation is not verification.** Arithmetic or algorithm self-consistency proves nothing about C++,
compilation, or a real terminal.
