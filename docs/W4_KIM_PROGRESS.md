# W4 / Kim Progress

Persistent resume anchor for W4's Phase 1A work (`T4.1`–`T4.3`, the plain-text marquee renderer). Read this
FIRST after a lost context; then continue from **## Exact next action**.

## Current status

* **Phase 1A (T4.1–T4.3) — COMPLETE, GREEN.** `src/features/marquee/renderer.cpp` is implemented; the two W4
  test files carry 27 new tests. Phase 1 is now code-complete across all four workstreams.
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

## Problems / blockers

* None. `cmake` is not on `PATH` on this machine; use CLion's bundled binary (see above).

## Exact next action

* W4's Phase 1A is done and committed. The next W4-owned items are **not actionable yet**:
  * `T1.4`/`T5.3` (measurement sweep + PPT numbers) need `frozen/csopesy.exe` from `T6.3` and a frozen renderer
    — do the sweep on the frozen binary only, never rebuild between runs.
  * `T3.4` is W3's live CLion gate; it needs the finished app, which now exists.
* If a future session finds the renderer untested or regressed, re-run the exact commands in
  **## Verification performed**.

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

### 2026-09-25 — session start

* Fast-forwarded `main` 5ea6ca9 → 04b49e4 (`git pull --ff-only`).
* Read `AGENTS.md`, `docs/PLAN_V3_PROGRESS.md`, `docs/IMPLEMENTATION_PLAN_v3.md` §3.9 + §5 + §6, `CONTRACTS.md`,
  `include/csopesy/renderer.hpp`, `include/csopesy/parameters.hpp`, `include/csopesy/process.hpp`,
  `include/csopesy/interpreter.hpp`, `src/app/scheduler.cpp`, `src/app/console_app.cpp`,
  `src/features/commands/interpreter.cpp`, `src/features/commands/line_editor.cpp`, `scripts/check_layers.sh`,
  `CMakeLists.txt`.
* Enabled CLion's bundled CMake on `PATH`.
* Baseline: configure + build OK; `ctest --preset debug` 2/2 (unit 90 + smoke); guard `OK (22 files scanned)`;
  11/11 hashes match `CONTRACTS.md`.

### 2026-09-25 — T4.1 + T4.2/T4.3 tests

* Wrote `tests/unit/test_scroll.cpp` (11 tests) and `tests/unit/test_renderer.cpp` (16 tests).
* First RED run **aborted** (`std::out_of_range`) because the stub returns `""` and the split helper called
  `substr(3)`. Made the helper abort-proof (report + blank rows; a safe `window()` instead of `substr`).
* Clean RED: `FAILED  180 assertion(s)` (scroll 79, renderer 101), exit 1.

### 2026-09-25 — implementation and GREEN

* Implemented `scrollOffset`, `sliceRow`, `buildFrame`, `bandWidthFor` in `src/features/marquee/renderer.cpp`.
* First GREEN attempt: 2 assertions failed — both were test-expectation bugs, not implementation bugs:
  the band row legitimately has a leading margin space (`" CSOPESY"`), and the welcome line also contains
  `"CSOPESY"`, so the band-uniqueness test was ambiguous. Fixed by expecting the margin and by using a
  distinctive band text (`"ZQX"`) for the uniqueness check.
* GREEN: `OK  117 tests`.

### 2026-09-25 — verification

* Clean rebuild warning-free; `ctest` 2/2 (unit 117 + smoke); guard OK; 11/11 hashes; v2 sweep clean.
* Printed a visual menu frame and a help-message frame from a scratch program; layout matches §3.9.
