# AGENTS.md — instructions for coding agents in this repository

**Project:** CSOPESY MCO3 — "Marquee Console". A C++17 terminal program (one animation process + a live
command prompt), a 4-member group submission due **2026-09-28** (revised from 2026-09-23 on 2026-09-22).
`src/main.cpp` is the graded **entry file**;
`README.txt` is required by the handout. Architecture is **two threads**: an input/command thread (`main`)
and a marquee worker, one `std::mutex`, one `std::condition_variable`.

**Current state (2026-09-22): Phase 0 is complete and fully closed; Phase 1 has started.** T0.1–T0.6 are done,
the T0.6 group announcement has been sent, and the CI run on the v3.0 commit is **green on all three OSes**
(run `35724109198`). The CLion-side live gate lives at **T3.4** because nothing about it is observable before
features exist. `include/csopesy/*.hpp` are the **v3.0** frozen contracts. Of Phase 1, `T1.1`
(`src/platform/terminal_posix.cpp`), `T1.2` (`src/entities/process.cpp`) and `T1.3`
(`src/app/scheduler.cpp`) are implemented and CI-green on all three OSes; the remaining
`src/**` files are still `TODO` stubs, so do not assume a task is done because the file exists — check
`docs/IMPLEMENTATION_PLAN_v3.md` §5 for the phase it belongs to.

**Contracts v3.0 landed 2026-09-22 (task T0.6, ratified by W2 per §4.5).** The tree now matches the v3 plan:
no config layer, a plain-text (ASCII) marquee, no `marquee_row`, no `--diag`, no `--measure`, no `FrameBuffer`
diffing, and no injected `Clock` — while keeping the professor-mandated two threads, the FSD layers, the
contract freeze and the 3-OS CI matrix. `CONTRACTS.md` is the v3.0 marker. Read
`docs/PLAN_V3_PROGRESS.md` for the decision list (D1–D18) and the current state.

**The v3 plan is in force (added 2026-09-22).** `docs/IMPLEMENTATION_PLAN_v3.md` simplifies the plan after the
professor's answers: no `.ini`/config layer, a plain-text (ASCII) marquee instead of the 5×5 glyph engine, no
`marquee_row`, no `--diag`, no `--measure`, no `FrameBuffer` diffing, and no injected `Clock` — while keeping
the professor-mandated two threads, the FSD layers, the contract freeze and the 3-OS CI matrix. The header
change landed as **T0.6** (see above), so `docs/IMPLEMENTATION_PLAN_v2.md` is now **frozen history**, not a
second live plan. Read `docs/PLAN_V3_PROGRESS.md` for the current state, the decision list (D1–D18) and the
agreed sequence.

## 1. Sources of truth (read before you write)

| Question | Answer |
| --- | --- |
| How should this be built? | `docs/IMPLEMENTATION_PLAN_v3.md` — the authoritative plan (contracts v3.0). `IMPLEMENTATION_PLAN_v2.md` is **frozen history**: v3 §3 quotes its §3.3/§3.8/§3.10/§3.11 normatively and must not be edited |
| What is the interface? | `CONTRACTS.md` + `include/csopesy/*.hpp` — **frozen** |
| Why is it like this? | `docs/REVIEW_ADJUDICATION.md` — every accepted/rejected review point, with reasons |
| What is graded? | plan §1 (spec decode) and §6 (definition of done, acceptance cases A1–A12) |
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

## 4. Contracts are frozen (v3.0, 2026-09-22)

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
  taking the lock, and the whole frame is built as one string **before** the lock is dropped so the single
  `Terminal::write()` happens *after* releasing it.
- `Terminal::nowMs()` is the **only** clock source — `Scheduler` calls `term_.nowMs()`. (v3.0 removed the
  injected `Clock` functor, D7.) Do not call `std::chrono` directly in scheduler logic.
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
- **Precedence** is defaults → CLI flags → runtime commands (three layers since v3.0; the config-file layer is
  gone, D1). Bad input **warns and keeps going** — a typo must not cost the quiz.
- **There is no config file.** `config/csopesy.ini` and `config/quiz_case_<n>.ini` no longer exist and the
  graded run config passes **no** program arguments; the parameter surface is the six commands plus three flags
  (`--no-tty` dev/CI-only, `--refresh-ms=N`, `--poll-ms=N`).
- `--no-tty` means plain line mode: no raw mode, no ANSI, no animation, **no worker thread** — it is what makes
  the real-binary smoke test portable across the three CI OS images. **Dev/CI only** (D13): never the graded run.
- **`--diag` and `--measure` are gone (v3.0).** A Windows VT-enable failure is now a **loud startup error**
  instead of a reported field (D5), and the PPT's refresh/polling numbers come from a documented **manual
  sweep on the frozen binary** — change the run config's arguments and press Run again, never rebuild (D14).

## 8. Definition of done

1. The task's test **failed before** the change and passes after (`ctest` output pasted into the PR).
   Sole carve-out: the **platform-surface tasks** (T1.1 termios, T3.1 Win32 console modes, T3.4 CLion terminal
   gate — which absorbed T0.5's live gate) — no unit test can observe raw-mode restoration or a live resize; do
   the hand-run checklist on the
   owner's hardware and record the result. Do not invent a test-shaped ritual to satisfy the wording.
2. CI green on all three OS families.
3. No frozen header changed without §4.5.
4. Demonstrated on the owner's own hardware (transcript/screenshot in the PR).
5. The commit message names the requirement (R1–R9) it advances.
6. `bash scripts/check_layers.sh` passes.

**Simulation is not verification.** Arithmetic or algorithm self-consistency proves nothing about C++,
compilation, or a real terminal.

## 9. Comment style

Comments are written for the next maintainer reading the code, not as notes to an agent. Keep them concise
and intentional.

- Prefer readable code over explanatory comments; a comment earns its place only when it says something the
  code cannot.
- Explain the non-obvious **why**, plus invariants, ownership rules, platform constraints and safety
  requirements. Do not narrate control flow or restate a variable/function name in prose.
- Do not put plan history, review history, agent/LLM reasoning, decision ids (D-numbers) or rejected designs
  in source comments — that lives in `docs/`.
- Avoid long block comments unless they document a real contract or invariant.
- Do not delete a comment merely because it is short; delete it when the code already says the same thing.
- Keep every `TODO(task)` marker until that task is complete.
- A landmine comment (`CMakeLists.txt`'s flag block, `Threads::Threads`, the `ctest` timeout, the frozen
  headers) stays — it is the only thing stopping the next person from "cleaning it up".
- `include/csopesy/*.hpp` are **frozen**: a comment edit there is a contract change under §4, not a cleanup.
