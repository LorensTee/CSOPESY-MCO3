# W4 / Kim Progress

Persistent resume anchor for W4's work. **Phase 1A** (`T4.1`–`T4.3`, the plain-text marquee renderer) is
complete; **Phase 2** (`T5.2` macOS second opinion, `T5.3` measurement scaffolding) is recorded below with
its evidence. Read this FIRST after a lost context; then continue from **## Exact next action** and the
**## Phase 2 final state report**.

## Current status

* **Phase 1A (T4.1–T4.3) — COMPLETE, GREEN.** `src/features/marquee/renderer.cpp` is implemented; the two W4
  test files carry 27 new tests. Phase 1 is now code-complete across all four workstreams.
* **Phase 2 (T5.2 macOS second opinion) — COMPLETE as far as macOS can take it (2026-09-24).** Baseline gate
  green, CI green on three OSes for `5bdc7d0`, and the app driven on a real kernel pty for the acceptance
  cases A1–A11 with byte-exact frame validation. See **## Phase 2 — T5.2 macOS second-opinion verification**
  and the final **## Phase 2 final state report** below. One acceptance/spec inconsistency was found (A4
  non-ASCII is unreachable from the keyboard) and recorded, not patched.
* **T5.3 — scaffolding only.** `docs/measurements/` now exists with the D14 methodology, a row template and the
  six per-OS tables, all marked **PENDING (T6.3)**. No measurement value is filled before the frozen binary.
* Still not actionable by W4: `T3.4`/`T5.1` (Windows-only, Nathan) and `T3.3` (Windows sweep, Nathan); `T1.4`
  (Linux sweep, Lorens). `T5.3`'s real numbers need `frozen/csopesy.exe` (`T6.3`).

### Phase 1A history (unchanged)

* Remaining W4-owned work is later-phase and not yet actionable: `T1.4` (Linux measurement sweep) and `T5.3`
  (measurement documentation) both need the **frozen** binary (`T6.3`), and `T3.4` is the live CLion gate.

## Repository state observed

* Branch `main`, fast-forwarded from `5ea6ca9` to `04b49e4` at session start (origin was 3 commits ahead: W2's
  Eof-quit fix + progress docs).
* `include/csopesy/renderer.hpp` is the frozen v3.0 contract (`buildFrame` → one `std::string`, `bandWidthFor`,
  `kBandRow == 3`, `scrollOffset`, `sliceRow`). Blob `3b0bd9663f5127ecd4a354354dec5181aa8220fe` matches
  `CONTRACTS.md`.
* `tests/unit/test_scroll.cpp` and `tests/unit/test_renderer.cpp` were header-only stubs before this session.
* Toolchain: `cmake`/`ctest` are bundled in CLion, not on `PATH`. Use
  `/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin` prepended to `PATH`.

## Task status

| Task | Status |
| --- | --- |
| T4.1 — scroll math + `sliceRow` | **DONE** — tests red-before (79 assertions) / green-after |
| T4.2 — frame layout | **DONE** — chrome, band at `kBandRow`, capital-W welcome, clipping |
| T4.3 — frame invariants / tight terminals | **DONE** — band+prompt never dropped, message rows, full rebuild |

## Tests added / expected red-before-green evidence

* `tests/unit/test_scroll.cpp` — 11 tests: `scrollOffset` wrap period/in-range/one-column-per-cycle/negative
  cycles/degenerate period; `sliceRow` exact-width invariant for empty/short/equal/long text, `offset == 0` all
  spaces, `offset == bandWidth` puts `text[0]` at column 0, both window edges, degenerate widths.
* `tests/unit/test_renderer.cpp` — 16 tests: `bandWidthFor` floor, fixed `kBandRow`, the mock chrome verbatim,
  band uniqueness, capital-W welcome, one column per cycle, clipping, prompt-last + buffer tail, message rows,
  tight terminals (rows 2..6), tiny cols (1..4), degenerate-size rejection, size-change rebuild, purity.
* **RED evidence:** `./build/debug/csopesy_tests` on the stub → `FAILED  180 assertion(s)` (scroll 79,
  renderer 101), exit 1. The test helper reports a malformed frame and returns blank rows instead of throwing,
  so a stub is a clean red rather than an abort.
* **GREEN evidence:** after implementation → `OK  117 tests` (was 90; +27), `ctest --preset debug` **2/2**.

## Implementation decisions

* **Scroll model.** `windowLeft = offset - bandWidth`; output column `c` reads `text[windowLeft + c]`, padded
  with spaces. `scrollOffset = cycles mod (textWidth + bandWidth)` with a non-negative modulus; a non-positive
  period returns 0 (no division by zero).
* **Frame height.** `buildFrame` returns exactly `rows` lines, each padded/clipped to exactly `cols`: a full
  rebuild must leave no stale columns, and the plan's invariant is *"the frame's width equals the terminal's
  width"*. The prompt is bottom-anchored (the v2 `FrameBuffer` was terminal-sized, so this is the carried-over
  layout).
* **Line separator is `\r\n`, not `\n`.** Raw mode clears `OPOST`/`ONLCR` on POSIX (`terminal_posix.cpp` line
  56) and Windows sets `DISABLE_NEWLINE_AUTO_RETURN`, so the frame must emit its own carriage return.
* **Cursor home** is `"\x1b[H"`, the first bytes of the frame (§3.9).
* **Message rows.** `buildFrame` receives the interpreter's response but the §3.9 row table does not list it;
  dropping it would make `help` invisible in raw mode, so the message is rendered one line per `\n` directly
  above the prompt (bottom-anchored; the newest lines survive when `rows` is tight), with one blank separator
  between the chrome and the message/prompt block. Empty message ⇒ that separator is the mock's blank line
  before `Command>`.
* **Chrome drop order** follows the plan literally: `Version date:`, then the developer names, then
  `Group developer:`, then the welcome line. The band stays at `kBandRow` when `rows >= 4`; for `rows < 4` it
  falls back to the row immediately above the message/prompt block. The prompt always wins the last row.
* **Prompt window.** The renderer cannot call `visibleSlice` (it lives in `features/commands`, and features may
  not cross-import), so `renderer.cpp` reproduces the tail window locally: `avail = cols - prompt.size() - 1`.
* **Known cosmetic note:** every row including the prompt is padded to `cols`, so the physical terminal cursor
  rests at the right margin rather than immediately after the typed text. Not pinned by the frozen contract;
  revisit only if the live demo (T3.4) makes it matter.

## Files changed

* `src/features/marquee/renderer.cpp` (implementation)
* `tests/unit/test_scroll.cpp`, `tests/unit/test_renderer.cpp` (tests)
* `docs/W4_KIM_PROGRESS.md` (this file), `docs/PLAN_V3_PROGRESS.md` (session entry)

## Verification performed

* **Commands** (from the repo root, with CLion's CMake on `PATH`):
  * `cmake --preset debug`
  * `cmake --build --preset debug`
  * `./build/debug/csopesy_tests` → `OK  117 tests`
  * `ctest --preset debug` → `100% tests passed, 0 tests failed out of 2` (unit + smoke)
  * `bash scripts/check_layers.sh` → `check_layers: OK (22 files scanned)`
  * `git hash-object include/csopesy/*.hpp` → **11/11** match `CONTRACTS.md` (no frozen header touched)
  * clean `rm -rf build/debug` + reconfigure + rebuild → **zero** warning/error lines under `-Wall -Wextra`
  * v2-deleted-feature sweep (`glyph|FrameBuffer|ascii_art|marquee_row|measurePath|artWidthFor|composeBand|drawFrame|--measure|--diag|injected Clock|renderDiff`) → only prose in frozen headers explaining that those things are gone; **no code reintroduced**
* **Visual smoke:** a scratch program (not committed) printed a `rows=16, cols=60` menu frame and a 7-line
  help-message frame; both matched §3.9 (welcome, blank, band at row 3, chrome, message, prompt last).
* **Cross-platform CI (pushed tip `daee787`):** run
  [35978176723](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/35978176723) — `completed success` on
  ubuntu / macOS / Windows, each running the layer guard, build and both `ctest` targets (created 2026-09-24T08:56Z).

## Problems / blockers

* None. `cmake` is not on `PATH` on this machine; use CLion's bundled binary (see above).

## Exact next action

* **Phase 2 T5.2 is done as far as macOS allows; the remaining W4 item is a human, not an agent, action:** a
  person should run `csopesy-dev` from CLion (or `./build/debug/csopesy` in Terminal.app) and confirm the two
  subjective items the pty harness cannot judge — that the band *looks* fluid and that no tearing/flicker is
  *visible to the eye* — then fill the **macOS (W4)** column of the Step 5 gate table in
  `docs/clion-run-config.md` (that file is W2's record, so W4 fills only its own column).
* `T5.3`: wait for `T6.3` (`frozen/csopesy.exe` + `docs/frozen-artifact.md`), then run the D14 sweep on the
  frozen binary and fill `docs/measurements/*-{refresh,polling}.md` per `docs/measurements/README.md`.
* `T3.4`/`T5.1` are Nathan's (Windows); `T1.4` is Lorens's (Linux). Do not attempt them on macOS.

### Phase 1A history (unchanged)

## Resume instructions

1. Read `AGENTS.md`, `docs/IMPLEMENTATION_PLAN_v3.md` §3.9 + §5 (Phase 1A), `CONTRACTS.md`, then this file.
2. Do **not** reintroduce glyphs, `FrameBuffer`, `ascii_art`, `marquee_row`, `--measure`, `--diag`, or an
   injected `Clock`.
3. Do **not** edit `include/csopesy/*.hpp` (frozen). If a contract change looks necessary, document it here
   first.
4. The acceptance criteria for T4.1–T4.3 live in `tests/unit/test_scroll.cpp` and `tests/unit/test_renderer.cpp`.

## T4 completion checklist

* [x] T4.1 scrollOffset contract
* [x] T4.1 sliceRow exact-width invariant
* [x] T4.1 tests red-before-green
* [x] T4.2 buildFrame normal layout
* [x] T4.2 fixed row 3
* [x] T4.2 exact `Welcome to CSOPESY!` capitalization
* [x] T4.2 clipping
* [x] T4.3 tight-terminal behavior
* [x] T4.3 band preserved
* [x] T4.3 prompt preserved
* [x] T4.3 full rebuild
* [x] Unit tests green
* [x] ctest green
* [x] warning-free build
* [x] frozen headers untouched
* [x] no removed v2 features reintroduced

## Chronological work log

### 2026-09-24 — session start

* Fast-forwarded `main` 5ea6ca9 → 04b49e4 (`git pull --ff-only`).
* Read `AGENTS.md`, `docs/PLAN_V3_PROGRESS.md`, `docs/IMPLEMENTATION_PLAN_v3.md` §3.9 + §5 + §6, `CONTRACTS.md`,
  `include/csopesy/renderer.hpp`, `include/csopesy/parameters.hpp`, `include/csopesy/process.hpp`,
  `include/csopesy/interpreter.hpp`, `src/app/scheduler.cpp`, `src/app/console_app.cpp`,
  `src/features/commands/interpreter.cpp`, `src/features/commands/line_editor.cpp`, `scripts/check_layers.sh`,
  `CMakeLists.txt`.
* Enabled CLion's bundled CMake on `PATH`.
* Baseline: configure + build OK; `ctest --preset debug` 2/2 (unit 90 + smoke); guard `OK (22 files scanned)`;
  11/11 hashes match `CONTRACTS.md`.

### 2026-09-24 — T4.1 + T4.2/T4.3 tests

* Wrote `tests/unit/test_scroll.cpp` (11 tests) and `tests/unit/test_renderer.cpp` (16 tests).
* First RED run **aborted** (`std::out_of_range`) because the stub returns `""` and the split helper called
  `substr(3)`. Made the helper abort-proof (report + blank rows; a safe `window()` instead of `substr`).
* Clean RED: `FAILED  180 assertion(s)` (scroll 79, renderer 101), exit 1.

### 2026-09-24 — implementation and GREEN

* Implemented `scrollOffset`, `sliceRow`, `buildFrame`, `bandWidthFor` in `src/features/marquee/renderer.cpp`.
* First GREEN attempt: 2 assertions failed — both were test-expectation bugs, not implementation bugs:
  the band row legitimately has a leading margin space (`" CSOPESY"`), and the welcome line also contains
  `"CSOPESY"`, so the band-uniqueness test was ambiguous. Fixed by expecting the margin and by using a
  distinctive band text (`"ZQX"`) for the uniqueness check.
* GREEN: `OK  117 tests`.

### 2026-09-24 — verification

* Clean rebuild warning-free; `ctest` 2/2 (unit 117 + smoke); guard OK; 11/11 hashes; v2 sweep clean.
* Printed a visual menu frame and a help-message frame from a scratch program; layout matches §3.9.

### 2026-09-24 — post-push consistency pass

* Pushed the three W4 commits (`cc4e879`, `e3ae9e8`, `daee787`) to `origin/main`; CI run 35978176723 is green on
  all three OSes.
* A review (`docs/replies/gpt-reply-v2.md`) flagged two doc issues, both verified against the repo and fixed:
  the W4 log date was off by one day (commit timestamps are `2026-09-24 16:55 +0800`, not 2026-09-25), and
  `PLAN_V3_PROGRESS.md` still carried stale Phase-1 status text (§1 "Current stage"/"Next action", §6's
  "T4.2/T2.4 still a stub" finding, §8's S6 row, §9 step 5). Docs-only; no source, test or frozen file touched.

### 2026-09-24 — Phase 2 session start (T5.2 macOS second opinion, T5.3 prep)

* Pulled `main` (`git pull --ff-only`): already up to date at `5bdc7d0` (`docs(progress): fix stale Phase 1
  status and W4 log date (R1)`), working tree clean except untracked `.DS_Store` files.
* Reading order followed: `AGENTS.md`, `docs/PLAN_V3_PROGRESS.md`, `docs/IMPLEMENTATION_PLAN_v3.md`
  (§1.2/§1.3 D14 sweep, §4.5 RACI, §5 Phase 2, §6.2 acceptance cases), `CONTRACTS.md`,
  `docs/clion-run-config.md`, `docs/measurements/` (does not exist yet).
* Scope for this session is **W4/macOS only**: T5.2 macOS second opinion + T5.3 synthesis preparation.
  T3.4 (Windows CLion gate), T5.1 (Windows acceptance), T3.3 (Windows sweep) and T1.4 (Linux sweep) stay with
  their owners; nothing here claims they passed.
* No `TODO(` stubs remain anywhere under `src/`, `include/` or `tests/`, so Phase 1 is code-complete.
* Toolchain: CLion's bundled CMake 4.3.1 on `PATH`; Apple clang 21.0.0; Ninja 1.13.2; macOS 27.0 / arm64.
* **Next action:** run the baseline gate (`cmake --preset debug`, build, `ctest`, `check_layers.sh`,
  `git hash-object`) and record exact results here.

### 2026-09-24 — Phase 2 baseline gate (all green)

* `cmake --preset debug` → configured clean (CMake 4.3.1, Apple clang 21.0.0, Ninja 1.13.2).
* `rm -rf build/debug` + clean rebuild → **exit 0**, `grep -icE 'warning|error'` = **0**.
* `ctest --preset debug` → `100% tests passed, 0 tests failed out of 2` (`unit` 2.50 s, `smoke` 0.28 s).
* `./build/debug/csopesy_tests` → `OK  117 tests`.
* `bash scripts/check_layers.sh` → `check_layers: OK (22 files scanned)`.
* `git hash-object include/csopesy/*.hpp` → **11/11 match `CONTRACTS.md`** (cli `4d2f3f25`, console_app
  `af282ac8`, interpreter `bb7d888a`, keys `1f6eeea8`, line_editor `5f229d78`, parameters `d190e4f5`,
  process `104ed6f8`, renderer `3b0bd966`, scheduler `6c006cdc`, shutdown `02bb0a2c`, terminal `e9031d85`).
  No frozen header touched.
* No `TODO(` markers remain under `src/`, `include/` or `tests/` — Phase 1 is code-complete in all four
  workstreams.
* **Next action:** drive the built binary on a real kernel pty for the T5.2 interactive checks (the bash tool
  has no TTY, so the harness allocates one with a real `TIOCGWINSZ` and real termios).

## Phase 2 — T5.2 macOS second-opinion verification

Commit under test: `5bdc7d07b59b2bc8014d65dbdc03e749a672ee2d` (`main`, 2026-09-24). CI for that exact SHA:
[run 35978839739](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/35978839739) — **completed/success** on
ubuntu-latest, macos-latest and windows-latest.

### Environment

| | |
| --- | --- |
| OS | macOS 27.0 (build `26A428`) |
| Architecture | `arm64` (Apple Silicon) |
| Terminal used | a **kernel pty** (`openpty`, real termios + real `TIOCGWINSZ`) — see the method note below |
| Compiler | Apple clang 21.0.0 (`clang-2100.3.34.2`, target `arm64-apple-darwin27.0.0`) |
| CMake / Ninja | CMake 4.3.1 (CLion-bundled), Ninja 1.13.2 |
| Build | `cmake --preset debug` (Debug) |

**Method note (honest scope).** The agent shell has no controlling tty (`TERM=dumb`), so the interactive checks
ran the built binary on a pty the harness allocated, sized, and drove one keystroke at a time. That is a real
terminal device (real raw-mode termios, real `ioctl(TIOCGWINSZ)`, real ANSI byte stream), and the frame stream
could be validated byte-exactly, but it is **not** a human watching Terminal.app. Two consequences are recorded
rather than glossed: (1) quantitative claims below are frame-stream measurements, not eyeball impressions; (2)
the subjective "looks fluid / no visible tearing to the eye" items remain **pending a human run** in
Terminal.app or the CLion terminal, which the harness cannot substitute for.

### Automated verification

* `cmake --preset debug` → configured clean.
* clean rebuild (`rm -rf build/debug`) → exit 0, **0** warning/error lines under `-Wall -Wextra`.
* `ctest --preset debug` → `100% tests passed, 0 tests failed out of 2` (`unit` 2.50 s, `smoke` 0.28 s).
* `bash scripts/check_layers.sh` → `check_layers: OK (22 files scanned)`.
* `git hash-object include/csopesy/*.hpp` → **11/11** match `CONTRACTS.md` (table in the baseline-gate entry
  below); no frozen header touched.
* CI: run `35978839739`, all three OS jobs green, every step (Layer guard / Configure / Build / Test) success.

### Interactive verification

Every captured frame was validated against `Renderer::buildFrame`'s contract: it must begin `\x1b[H`, be
exactly `3 + rows*cols + 2*(rows-1)` bytes, separate rows with `\r\n`, and contain no further `\x1b`. A stream
that is a clean concatenation of such frames is the frame-level evidence that nothing was half-drawn and no
frame interleaved another. Counts:

| Scenario | Frames | Malformed | Exit | termios restored |
| --- | --- | --- | --- | --- |
| full command sweep at 80×24 (`--refresh-ms=100`) | 894 | **0** | 0 | yes |
| A8 coexistence | 71 | **0** | 0 | yes |
| resize mid-animation + 20×5 + back | 43 | **0** | 0 | yes |
| Ctrl+C (`--refresh-ms=50`) | 18 | **0** | 0 | yes |
| very fast (`--refresh-ms=1 --poll-ms=1`) | 1828 | **0** | 0 | yes |
| very slow (`--refresh-ms=10000`) | 22 | **0** | 0 | yes |

Startup frame, 80×24 (exact rows, each padded to 80):

```text
row00 |Welcome to CSOPESY!                                                             |
row01 |                                                                                |
row02 |                                                                                |
row03 |                                                                                |   <- band (blank at cycle 0)
row04 |Group developer:                                                                |
row05 |De La Cruz, Juan                                                                |
row06 |Santos, Alex                                                                    |
row07 |                                                                                |
row08 |Version date:                                                                   |
...
row23 |Command>                                                                        |
```

### Acceptance observations

* **A1 `help`** — all six lines present (`Available commands:` + `help`, `start_marquee`, `stop_marquee`,
  `set_text`, `set_speed`, `exit`); identical while animating.
* **A2 `start_marquee`** — `Marquee started.`; the band changes every cycle (10 distinct band positions in the
  first ~0.8 s at `refreshMs=100`); second invocation → `Marquee is already running.`
* **A3 `stop_marquee`** — `Marquee stopped.`, then **0** further frames in the next 0.6 s (the worker is idle,
  not spinning); second invocation → `Marquee is not running.`
* **A4 `set_text`** — single word `"hello"`; multi-word/punctuation/digits
  `"Hello, World 42!"`; internal runs preserved `"alpha    beta"`; lowercase preserved; 220 chars accepted
  (echo is 243 chars, fully visible at 300 cols); empty → `Usage: set_text <text>`; **non-ASCII: see the
  finding below**.
* **A5 `set_speed`** — `16` → `Marquee speed set to 16 ms.`; `0` → `Speed 0 ms is out of range [1, 10000];
  clamped to 1 ms.`; `-5` → `… clamped to 1 ms.`; `10001` → `… clamped to 10000 ms.`; `abc`, `5abc`, `1.5` →
  `Usage: set_speed <milliseconds>`; a 20-digit literal → clamped to 10000 ms, no crash. The rate visibly
  changed in every accepted case.
* **A6 `exit` / signals** — `exit` → `Exiting CSOPESY. Goodbye!`, exit code **0**, prompt returns, termios
  byte-identical to the pre-run slave termios; **Ctrl+C** (0x03 → `Eof`) → same clean shutdown while
  animating; **SIGTERM** → same; **Ctrl+D with a non-empty line buffer** → same. `ps -M` shows exactly the two
  expected threads while running, and none after exit.
* **A7 unknown/empty** — `foo` → `Command not recognized: "foo". …`; `HELP` → unrecognized (case-sensitive);
  `start marquee` → `"start"` unrecognized; whitespace-only → **no new message**, prompt intact;
  `set_speed 100 extra` → `Usage:`.
* **A8 coexistence (the real-concurrency case)** — `start_marquee`, then `set_text Coexistence A8` typed
  **one key at a time** (23 keys, 120 ms apart, 2.84 s). Observed: **56 frames** in the window, **33 distinct
  band positions** between the first keystroke and `Enter`, the full command echoed, the band switching from
  `CSOPESY` to the new text, and **0 malformed frames**. Typing stayed responsive throughout.
* **A9 parameter-path parity** — `--refresh-ms=50` → 20 distinct band positions/s, `set_speed 50` → 20, and
  `--refresh-ms=100` → 10. Same-effect parity holds on this build; the **frozen-binary** take is still `T6.3`.
* **A10 extremes** — `--refresh-ms=1 --poll-ms=1` → ~753 frames/s measured, and a command typed one key at a
  time was still fully echoed (79 frames carried the complete command) with **0 malformed frames** in 3.6 MB of
  stream. `--refresh-ms=10000` → 0 animation frames in 1.5 s idle, yet **16 echo frames during 0.8 s of
  typing** — echo is not gated by `refreshMs`.
* **A11 tiny terminal** — resize to 100×40 repainted at the new width (row 3 band, prompt on the last row);
  20×5 keeps band and prompt with no row exceeding 20:

  ```text
  row0 |Welcome to CSOPESY! |
  row1 |                    |
  row2 |                 CS |   <- band (text tail), never dropped
  row3 |Marquee started.    |
  row4 |Command>            |   <- prompt, always the last row
  ```

  Resizing back to 80×24 fully recovered the layout.
* **A12 plain line mode (dev/CI only)** — exercised locally: `--no-tty` prints plain responses, emits no ANSI,
  starts no worker thread, exits 0. (The CI `smoke` test is the portable form of this case.)

### Failures / limitations

1. **A4 non-ASCII is unreachable from the keyboard (acceptance/spec inconsistency, not silently patched).**
   Typing `set_text café` in raw mode produces `Marquee text set to "caf".` — `PosixTerminal::readEvent`
   (`src/platform/terminal_posix.cpp`, W1 Lorens) only maps bytes `0x20`–`0x7E` to `KeyEvent{Char}`, so the two
   UTF-8 bytes of `é` are dropped before `Interpreter::executeLine` sees them, and §3.7 rule 7's
   `set_text: only printable ASCII characters are supported.` line never fires. The **same input via
   `--no-tty`** (stdin lines, a path §3.7 rule 7 explicitly names) **does** print the rejection line —
   verified. §3.7's own rationale says this is by design (*"the interactive line editor appends printable ASCII
   only … so typing cannot produce an invalid argument"*), but §6.2 case **A4** lists non-ASCII as an
   interactive varying input and expects the rejection line. **The two acceptances cannot both hold.** The
   spirit of A4 survives — no mojibake, no crash, band width unchanged — only the letter (the rejection line)
   does not. Ownership: §3.7/A4 wording is W2's (interpreter/plan), the byte mapping is W1's (POSIX backend).
   **No code or contract was changed.** Recommendation for the team: either reword A4 to say non-ASCII is
   *ignored on the interactive path, rejected in plain line mode*, or have `readEvent` pass high bytes through
   as `Char` so rule 7 fires — that second option changes T1.1's behavior and needs its owner.
2. **A4's 200+-char echo is clipped by the frame width.** At 80×24 the 243-char confirmation line is shown as
   its first 80 columns; at 300 cols it is the full 243 chars. This follows from §3.9's binding invariant
   (*"every row is clipped to `cols` so no line ever wraps"*) and the message rows the renderer added in
   Phase 1A, so it is a documented consequence, not a defect. The accepted text itself is intact and scrolls in
   the band.
3. **CPU numbers are coarse and are NOT the T5.3 sweep.** `ps -o time=` has 0.01 s granularity, so the
   current-build observation (2 threads; ~0.3–0.5 % of one core idle; ~0.5 % at `refreshMs=100`; ~2.8 % at
   `refreshMs=1`) is resolution-limited. The published table must come from the frozen binary (`T6.3`, D14).
4. **Human-visual items pending.** Visible tearing/flicker as seen by an eye, and "the band looks fluid", were
   not observed by a human on this machine. The frame-stream evidence is exact, but this is a gap a person
   should close in Terminal.app / the CLion terminal.
5. **Out of scope here.** `T3.4` (Windows CLion gate) and `T5.1` (Windows acceptance) are **not** verified —
   they need Windows. `T1.4` (Linux) and `T3.3` (Windows) sweeps are **not** verified. Nothing in this document
   claims otherwise.

## Phase 2 — T5.3 preparation (structure only, no fabricated values)

The repository had **no** `docs/measurements/` directory, although the plan names
`docs/measurements/linux-{refresh,polling}.md` (T1.4) and `docs/measurements/windows-{refresh,polling}.md`
(T3.3), and T5.3 synthesizes "the three `docs/measurements/*` tables". The gap was structural, so W4 created
the scaffolding:

* `docs/measurements/README.md` — the D14 frozen-binary procedure, the "no invented numbers" rule, the
  per-machine + per-thread reporting requirement (plan risk 24), the honest limitation (observed thresholds,
  **not** per-keystroke latency), and the file/owner/status matrix.
* `docs/measurements/TEMPLATE.md` — the refresh and polling row shapes (values, fluidity, tearing/flicker,
  typing delay, idle CPU process/per-thread, Ctrl+C and key-echo latency, recommended value, observed
  thresholds).
* `docs/measurements/{linux,windows,macos}-{refresh,polling}.md` — six per-OS tables, each naming its recorder
  (W1 / W3 / W4) and marked **PENDING (T6.3)**.

**Nothing was measured and nothing was filled.** Every value cell says `PENDING (T6.3)`. This is deliberate:
D14 requires the sweep on the **frozen** binary, and the frozen binary is a `T6.3` artifact that does not exist
yet. The only measurement-shaped data recorded anywhere in this session is the clearly-labelled *current-build*
idle-CPU observation under T5.2 above (2 threads; ~0.3–0.5 % of one core idle; ~0.5 % at `refreshMs=100`;
~2.8 % at `refreshMs=1`), which is explicitly **not** the sweep and must be re-taken on the frozen binary.

## Phase 2 — documentation readiness review (Part 6 findings)

Scope: only the live `docs/*.md` and `AGENTS.md`/`README.txt`; `IMPLEMENTATION_PLAN_v2.md` and
`REVIEW_ADJUDICATION.md` are frozen history and were not treated as defects. `v2`-feature words
(`--measure`, `--diag`, `FrameBuffer`, glyphs, `marquee_row`, `ascii_art`, `config/csopesy.ini`) occur in the
live docs only in "removed / deleted / no longer" contexts, so there is no live v2-feature instruction to
follow. No measurement instruction suggests rebuilding between takes — §1.2's D14 block (line ~120) and the
Table C `--refresh-ms`/`--poll-ms` row (line ~254) both say "without a rebuild", correctly.

Defects found (**recorded, not edited** — see the decision note below):

| # | Location | Defect | Recommended minimal fix | Owner |
| --- | --- | --- | --- | --- |
| 1 | `docs/IMPLEMENTATION_PLAN_v3.md` Table C, T0.5 row (~line 263) | Says the live CLion gate "moves to Phase 2 (**T5.1**)". Eight other places (lines 752, 802, 924, 1074, 1181, 1219, §5) say **T3.4** absorbs T0.5's live gate. T5.1 is the Windows **acceptance** run, a different task. | `(T5.1)` → `(T3.4)` | W1/plan author |
| 2 | `docs/IMPLEMENTATION_PLAN_v3.md` Table C, T1.4 row (~line 262) | Attributes T1.4 to "**(W4, was W1+W4)**". §5 Phase 1B lists T1.4 under **W1** ("W1 records Linux; W4 synthesizes"), and §4.5's RACI gives the Linux sweep to W1 with W4 as synthesis A/R. | `**T1.4 (W4, was W1+W4)**` → `**T1.4 (W1)**` | W1/plan author |
| 3 | `docs/clion-run-config.md` line ~82 | "the frozen artifact does not exist until **T2.6/T6.3**". v3 moved T2.6's frozen-artifact half to **T6.3** (PLAN_V3_PROGRESS records the move). | `T2.6/T6.3` → `T6.3` | W2 (owns the record) — **fixed upstream `07c80f2`** before this branch was pushed |
| 4 | `docs/clion-run-config.md` line 1 | Title still reads "the record (T0.5 Steps 3–6)"; the v3.0 note below already corrects the gate to T3.4, so this is cosmetic. | optionally retitle | W2 |
| 5 | `docs/IMPLEMENTATION_PLAN_v3.md` T5.3 row (~line 944) | "the three `docs/measurements/*` tables" while the plan names four Linux/Windows files plus a macOS set; "three" is ambiguous (three machines?). | say "the per-machine tables" | W1/plan author |
| 6 | `docs/IMPLEMENTATION_PLAN_v3.md` §5 T1.4 row | Minor: `...linux-{refresh,polling}.md...` is under Phase 1B/W1 — consistent with the RACI, so finding 2 is the outlier, not this. Listed for completeness. | none | — |

**Decision on editing (recorded deliberately).** Every one of these is in a file another member owns —
`IMPLEMENTATION_PLAN_v3.md` is the group's authoritative deliverable and `docs/clion-run-config.md` is
explicitly "W2 owns the record" (plan risk 13). The task brief says Byron is concurrently fixing stale Phase 1
documentation and that W4 must not race him or rewrite history, and that documentation should be edited "only
when necessary for your current W4 Phase 2 work". None of the six blocks W4's work — the correct ownership and
the correct freeze task are unambiguous from §5/§4.5 — so **no other member's document was modified**. The
table above is the handoff instead. Files W4 *did* change this session: `docs/W4_KIM_PROGRESS.md` and the new
`docs/measurements/` tree. No source, test, CMake, script or frozen header was touched.

**Post-rebase confirmation of that decision.** Between committing and pushing, W2 pushed `07c80f2` and
`debe270`, and `07c80f2` independently fixed finding **3** (`docs/clion-run-config.md` now cites `T6.3` alone,
with the message *"W2 owns this record per plan §6 risk 13"*) — exactly the fix recommended here. W4's
branches were rebased onto those commits with no conflict, and findings **1, 2, 4, 5** were re-checked and are
still open in `docs/IMPLEMENTATION_PLAN_v3.md` at HEAD (`3a9ef79`). Not editing them was the right call; had W4
edited `clion-run-config.md` it would have collided with W2's commit.

One Phase-2 ownership note worth stating plainly: the plan's D5 schedule row (~line 817) reads
"**Nathan + Kim**" and sweeps several Windows-only items (T3.4, T5.1, A1–A11 on the Windows build) into one
line. On macOS W4 can only contribute the **macOS** column of T5.2, the T5.3 synthesis and the slides; the
Windows-only items in that row belong to Nathan. Nothing in this log claims otherwise.

## Phase 2 final state report

### Current status

W4's Phase 2 work that is genuinely reachable from macOS is **complete**: the T5.2 macOS second-opinion
verification (automated + real-pty interactive) and the T5.3 measurement-documentation scaffolding. The one
remaining self-contained W4 item is the human visual/CLion pass noted under **Exact next action**.

### T5.2 status

* Baseline gate green at commit `5bdc7d0`: clean warning-free build, `ctest` 2/2 (unit 117 + smoke), guard
  `OK (22 files scanned)`, 11/11 frozen hashes unchanged.
* CI run `35978839739` green on ubuntu-latest / macos-latest / windows-latest for that exact SHA.
* Interactive verification on a real kernel pty: **9 scenarios, 2 916 frames, 0 malformed frames**, every exit
  code 0, termios restored byte-identically in every scenario. A1–A11 observed; A8 recorded **33 distinct
  marquee updates** in a 2.84 s one-key-at-a-time window. Full detail in the T5.2 section above.

### T5.3 status

* **Prepared:** `docs/measurements/` structure, D14 procedure, template, six per-OS status tables.
* **Blocked on `T6.3`:** the frozen `frozen/csopesy.exe` and its SHA-256 record. No value may be filled before
  then. Afterwards W4 fills `macos-{refresh,polling}.md`, W1 fills the Linux pair, W3 the Windows pair, and W4
  synthesizes the PPT table with the honest limitation.

### Evidence

* Automated: `ctest --preset debug` → `100% tests passed … out of 2`; `./build/debug/csopesy_tests` →
  `OK  117 tests`; `check_layers.sh` → `OK (22 files scanned)`; the 11 `git hash-object` blobs listed in the
  baseline-gate entry.
* CI: <https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/35978839739> (completed/success, three OSes).
* Interactive: the frame/layout dumps, per-scenario frame counts and the A8 count in the T5.2 section. Method:
  `os.openpty()` + `TIOCSWINSZ` + `TIOCSCTTY`, keystrokes written to the master one byte at a time, the raw
  byte stream re-parsed into frames and each frame checked to be exactly `3 + rows*cols + 2*(rows-1)` bytes
  with `\r\n` separators and no embedded `\x1b`. The harness lived in `/tmp` and is **not** committed (W4 does
  not own `scripts/`); the method is described here so the result is reproducible.
* Working tree at report time: only `docs/W4_KIM_PROGRESS.md` and `docs/measurements/*` changed; `.DS_Store`
  files remain untracked and were never added.

### Known issues

1. **A4 vs §3.7 rule 7** — typing non-ASCII in raw mode silently drops the high bytes (→ `"caf"`), so the
   rejection line never fires on the interactive path; it does fire via `--no-tty`. Acceptance/spec
   inconsistency; no code changed (see T5.2 limitation 1).
2. **A4 200+-char echo is width-clipped** at normal terminal widths — a consequence of §3.9's no-wrap
   invariant, not a defect (limitation 2).
3. **Human visual pass pending** (limitation 4).
4. Six documentation defects in other owners' files, listed above and deliberately left for their owners.

### Remaining W4 work (no Windows/Linux/frozen-binary access needed)

* The human visual + CLion macOS column pass.
* Nothing else until `T6.3`.

### Resume instructions

1. `git pull --ff-only`; read `AGENTS.md`, `docs/IMPLEMENTATION_PLAN_v3.md` §5 (Phase 2) + §6.2 + D14,
   `CONTRACTS.md`, then this file top-to-bottom.
2. Do **not** re-run the T5.2 interactive suite from scratch unless you need fresh evidence; the results above
   are recorded with the exact commit and method.
3. Do **not** fill `docs/measurements/*` before `T6.3`; do **not** edit `include/csopesy/*.hpp`; do **not**
   reintroduce glyphs, `FrameBuffer`, `ascii_art`, `marquee_row`, `--measure`, `--diag`, an injected `Clock`, a
   config layer, or a second production mutex/condition variable.
4. Do **not** take Windows-only work (T3.4, T5.1, T3.3) from macOS; those are Nathan's.
5. Keep `bash scripts/check_layers.sh` and the 11 frozen hashes green before every commit.

## Phase 2 completion checklist

* [x] Phase 2 session-start checkpoint written before the first long command
* [x] Baseline build / ctest / layer guard / frozen hashes green
* [x] CI green on all three OS families for the tested commit
* [x] Interactive macOS verification on a real pty (A1–A11)
* [x] A8 real-overlap count recorded (33 marquee updates)
* [x] Frame-stream validation (0 malformed frames) across all scenarios
* [x] Terminal restoration verified after exit / Ctrl+C / Ctrl+D / SIGTERM
* [x] Resize mid-animation, 20×5 tiny terminal, and recovery verified
* [x] `docs/measurements/` structure created, all values pending `T6.3`
* [x] Documentation defects identified and handed off without racing other owners
* [ ] Human visual pass + macOS CLion column of `clion-run-config.md` (needs a person at the GUI)
* [ ] T5.3 sweep values (blocked on `T6.3`)

### 2026-09-24 — Phase 2 commits, rebase onto W2, push

* Committed the T5.3 scaffolding as `1236d8f` (now `dbc2430` after rebase) and the T5.2 record as `380a1b3`
  (now `3a9ef79`). `git push` was rejected (non-fast-forward): W2 had pushed `07c80f2` and `debe270`.
* Fetched and inspected rather than forcing: the remote diff touched `README.txt`,
  `docs/PLAN_V3_PROGRESS.md`, `docs/clion-run-config.md` — disjoint from W4's files. Rebased cleanly
  (`git rebase origin/main`), no conflicts, no other member's work overwritten.
* `07c80f2` turned out to fix finding 3 independently, which is direct evidence that recording rather than
  editing the other owners' documents was the correct choice.
* Re-ran the gate at the rebased tip before pushing: layer guard `OK (22 files scanned)`, `ctest` 2/2,
  11/11 frozen hashes unchanged, `git status` shows only W4's two doc paths.

### 2026-09-24 — pushed tip and its CI

* Pushed as **`7f64b5f`** (`main`, on top of W2's `debe270`). CI for that exact SHA:
  [run 35981966258](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/35981966258) —
  **completed/success**: ubuntu-latest (16 s), macos-latest (21 s), windows-latest (49 s), each running the
  layer guard, configure, build and both `ctest` targets. The commits are documentation-only, so the compiled
  tree is identical to `debe270`'s.
