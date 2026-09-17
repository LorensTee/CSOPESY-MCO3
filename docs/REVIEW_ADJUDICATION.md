# Adjudication of the GPT Luna reviews against the implementation plan

This file covers **seven** review rounds:

| Round | Review | Reviewed artifact | Verdict | Section |
| --- | --- | --- | --- | --- |
| 1 | `gpt-v1.md` | `IMPLEMENTATION_PLAN.md` (v1) | ~75 % accepted, ~15 % amended, ~10 % rejected; 7 defects missed | §1–§5 |
| 2 | `gpt-v2.md` | `IMPLEMENTATION_PLAN_v2.md` (v2) | 5 of 5 accepted (2 amended/strengthened); 6 defects missed | §6 |
| 3 | `gpt-v3.md` | `IMPLEMENTATION_PLAN_v2.md` (v2.1) | 5 of 5 accepted (1 amendment that the review's own advice depended on, 1 narrowing, 1 broadening); 5 defects missed | §7 |
| 4 | `gpt-v4.md` | `IMPLEMENTATION_PLAN_v2.md` (v2.2) | 4 of 4 actionable accepted (1 verified by reproduction, 1 broadened); 1 defect it filed as philosophy was a real contradiction | **§8** |
| 5 | `gpt-v5.md` | `threading.md` + `IMPLEMENTATION_PLAN_v2.md` (v2.4) | 7 of 7 accepted + the closing warning adopted (3 amended/sharpered); **4 defects missed**, two in the exact area reviewed | **§9** |
| 6 | `gpt-v6.md` | the same, against the handout | 2 accepted in spirit, **4 rejected with reasons**; 0 new defects (its one defect is v5's, with a weaker fix) | **§10** |
| 7 | `gpt-v7.md` | the revised v2.5 plan + **this file** | **1 accepted in direction only — the filed race is a false positive**; 0 defects missed; all its "keep" recommendations confirmed | **§11** |

---

### Standing reference — `AGENTS.md` (provenance note, added 2026-09-17)

Several rounds below cite *this repo's `AGENTS.md`* as the authority mandating the Feature-Sliced layers
(point 21, §10.2). The instruction itself was real from round 1 and the substance is unchanged, but the
**file was not committed until 2026-09-17**: `AGENTS.md` is now in the repository root, and its §3 is the
layer rule restated as plan §3.1 and enforced by `scripts/check_layers.sh`. Until then the citations pointed
at a file that existed only in the working conversation, which round 2's reviewer (`gpt-v2.md` §"Keep the FSD
layer structure") then repeated back. The decisions and their reasoning are unaffected; the citation is now
verifiable in-tree.

---

## Round 1 — `gpt-v1.md` against `IMPLEMENTATION_PLAN.md` v1

> Written 2026-09-16 by the implementer. Every point below was checked against **primary evidence**, not
> against the review's own summary: the professor's handout (`MO3 - Marquee_Operator.pdf`, incl. the
> 600-dpi render of the console screenshots), the plan text itself, and JetBrains' current CLion docs.
> Nothing in this file is "GPT said so".

**Verdict on the review itself:** the *direction* is right — it found one genuinely architecture-breaking
defect and four real contradictions, and its simplification pressure is correct for a 7-day assignment.
But it is not all correct: **2 points are wrong / overstated**, several are *amended* rather than accepted,
and it **missed 7 defects of the same severity as the ones it found** (3 of them in the exact test file it
criticised). It also proposes a repository restructure that would violate this project's own coding
instructions (`AGENTS.md` §3 — see the provenance note at the top of this file). Result: **~75 % accepted,
~15 % amended, ~10 % rejected** → see §3.

---

### 1. Claims re-verified against primary sources

| Claim | Evidence | Verdict |
| --- | --- | --- |
| "T4.6 does not exist" | `grep '^#### T'` on v1 → 31 task headings; no `T4.6`, yet T3.3 cites "CSV telemetry (§T4.6)" | **True** |
| `renderBlockText("C S 0")` is not 17 wide | Computed from the plan's own `kA`/`kB` glyphs: 5 cells + 4 gaps = **29** (17 is 3 cells + 2 gaps) | **True** |
| Spec's version-date field is blank | 600-dpi crop of page 2: `Version date:` with nothing after it | **True** |
| Spec's expected-output sentences are not in the handout | `pdftotext` of all 3 pages: the handout names the commands and their effect; it never specifies response wording | **True** |
| Video must show Run/Debug; no alternate path | Handout verbatim: *"The video should always show you pressing Run/Debug from your IDE, which should initialize your program."* | **True** (runbook contradicts it) |
| `lukka/get-cmake@latest` is not needed | GitHub-hosted runners ship CMake ≥ 3.20 (3.3x today); the action also pins a moving `@latest` tag | **True** |
| CLion's output-console emulation is Windows-less | JetBrains' *Terminal in the output console* doc: availability *"depends on the OS, debugger, and toolchain"*; the table marks **MSVC LLDB: supported** on Windows (GDB: not supported). v1's blanket claim came from the **Rust** product doc | **GPT is right** — v1 was too broad |
| "one `write()` ⇒ tearing impossible" | A terminal still processes cursor-moves + glyph writes sequentially; one write only removes *application-level* interleaving | **True** (overstated) |
| Whitespace semantics contradict themselves | T2.1 requires `setText("  HELLO  ")` to trim; A4 requires leading/trailing spaces to be "echoed exactly" | **True** |
| Double input polling | §3.8 `runOnce()` calls `readEvent(...)`; §3.8 `run()` *also* does `readEvent(pollTimeoutMs()) -> runOnce()`; T2.5 repeats it for `ConsoleApp::run()` | **True** |
| Signal-handler restore is unsafe | `restore()` does ioctl/`tcsetattr`/`SetConsoleMode` on shared state; not async-signal-safe | **True** |
| "Three entry points for every parameter" is not implemented | Only `text`/`refreshMs`/`start-stop` have runtime commands; 8 of 11 config keys have no command | **True, but** it is a *wording* defect, not an architecture defect (§3.11) |

---

### 2. Defects the review MISSED (found in this pass, same severity as its findings)

| # | Defect | Evidence | Fix in v2 |
| --- | --- | --- | --- |
| M1 | **A second broken assertion in the very test file GPT flagged.** `CHECK_STR(rows[0], " ###  ####")` is a 10-char literal; the real output is `" ###  #### "` (11 chars, trailing pad). GPT caught only the 17-vs-29 line | Computed: `kA[0]+" "+kB[0] == " ###  #### "` | T4.1 rewritten; a **glyph-width invariant test** added so all ~45 hand-typed glyphs are checked |
| M2 | **`Welcome to CSOPESY!` is capitalised in the spec.** v1 line 906 asserts `welcome to CSOPESY!` | 600-dpi crop: `Welcome to CSOPESY!` | Fixed in the header contract + test |
| M3 | **T1.3's `typed_command_is_executed_during_animation` cannot pass.** It queues 13 chars and expects the command to run in 2 ticks, but the contract reads **one** event per tick | v1 T1.3 vs §3.8 | Dies naturally with the single-owner scheduler; test rewritten as a driver loop |
| M4 | **Windows `readEvent(timeoutMs)` is unspecified** — `_kbhit()` cannot block, so v1's contract implies a busy-wait that would burn a core *and* wreck the measurements | v1 §3.3 vs T3.1 | `WaitForSingleObject(consoleInput, timeout)` specified |
| M5 | **`--plain` means two different things.** Risk #3 wants an ANSI-free legacy mode; A9 uses `--plain` for "single-row text". Colliding names | v1 §3.5/Risk 3/A9 | `--plain` = single-row text only; the ANSI-free mode is **cut** (VT is verified at startup instead) |
| M6 | **T1.3's harness cannot compile**: `Interpreter interp;` default-constructs while the interpreter needs `Parameters&` (and `MarqueeProcess&` to run `start/stop_marquee`); `FrameBuffer` ownership is unstated | v1 T1.3 snippet vs §3.7 | Harness rebuilt with explicit wiring; `Scheduler` owns the `FrameBuffer` |
| M7 | **v1's render condition can never fire on the first tick.** `due = now - lastRenderMs >= refreshMs` with `lastRenderMs = 0` and `now = 0` is `0 >= 100`, i.e. false — yet T1.3 asserts `cycles == 1` at `clock = 0` "first frame immediately". So `start_marquee` would show a blank band for one whole refresh interval, and the test that says otherwise was unreachable. Neither v1 nor the review noticed | v1 §3.8 `tick` vs v1 T1.3 `render_respects_refresh_interval` | `MarqueeProcess::hasRendered` added; `due = running && (!hasRendered \|\| now - lastRenderMs >= refreshMs)`; `start()` clears it so a restart also repaints at once; `start_and_restart_render_immediately` test added |

Minor (fixed in passing): `std::count` used without `<algorithm>` in T4.3's test; `cmake -S . -B build` in
T0.1 vs `cmake --preset` in T0.2; `#`-mid-line comment rule is arbitrary complexity; duplicate time
sources (`Terminal::nowMs()` *and* injected `Clock`); `FrameBuffer` declared inside `renderer.hpp` while the
layout and T4.3 put it in `frame_buffer.hpp`; T4.1's `renderBlockText("")` edge case unspecified.

---

### 3. Point-by-point verdict

Legend: **A** = accepted and implemented in v2 · **A′** = accepted with amendment · **R** = rejected, with reason.

| # | GPT point | Verdict | Decision / reason |
| --- | --- | --- | --- |
| 1 | Scheduler/input double-polling is broken; one component must own input | **A** | v2: `Scheduler::run()` owns `readEvent`; `tick(const KeyEvent*)` is pure. Echo is no longer gated on `refreshMs` (a latency bug the review did not name) |
| 2 | "One write ⇒ no tearing" is too strong | **A** | v2 wording: *one assembled frame submission ⇒ no application-level interleaving*; no synchronized-update escapes (correctly called over-engineering) |
| 3 | T4.1's `"C S 0"` → 17 is wrong | **A** | Fixed **and** extended: M1 + the width-invariant test |
| 4 | Whitespace semantics contradict themselves | **A** | One rule: trim the line, split the command at the first whitespace run, then trim the *argument's* surrounding whitespace, preserving internal run(s). A4 reworded |
| 5 | "Three entry points for every parameter" is unneeded/unimplemented | **A′** | Kept as **two** paths that matter (config + CLI) for the *major* tunables, and one runtime command per required command. Claim reworded; the *plumbing* stays because it is ~30 lines and is the partial-credit hedge |
| 6 | "Every behaviour needs a second path" is an invented grading rule | **A** | Reworded to a precaution, not a rule. The handout's workaround column is quoted verbatim instead |
| 7 | Exact response strings are risky | **A′** | Messages simplified (no `(refresh = 100 ms)` decoration); tests now assert the **token** + `Usage:`/goodbye lines exactly. Clamp text kept, because A5 grades visible clamping |
| 8 | Header: version date should default blank | **A** | `version_date =` (blank) matches the mock; M2 capitalization fixed too |
| 9 | Signal-handler restore is unsafe/over-promised | **A** | Handler sets a `sig_atomic_t` flag only; the loop exits normally; RAII/`atexit` as backstop; SIGKILL explicitly out of scope |
| 10 | Windows backend is over-complicated; don't mix VT input with `_getch` | **A** | VT **output** on; VT **input** off; `_kbhit`/`_getch` with 0/224 prefixes; `WaitForSingleObject` for the timeout (M4) |
| 11 | T4.6 does not exist | **A** | v1's CSV telemetry replaced by a 5-line `--measure=FILE` (frame index + timestamp) and a manual sweep table |
| 12 | 49×4 measurement matrix is over-engineered | **A′** | 7×4 sweep, plus explicit "vary refresh and polling independently first" guidance. Must still cover 4 machines — that is what the PPT grades |
| 13 | PCB state `Ready`/`Finished` + `quantumMs` are unused | **A** | `{Stopped, Running}`; `quantumMs` dropped; `cycles`/`lastRenderMs` kept as the scheduler's evidence |
| 14 | `MarqueeProcess::asciiArt` duplicates `Parameters::asciiArt` | **A** | Single source of truth; `bandRows()` deleted (art height is a renderer constant) |
| 15 | Drop `stepCols` and `marqueeRow` | **A′/R** | `step_cols` **cut**. `marquee_row` **kept as a config-only knob**: the handout's two console images disagree about where the band sits (empty in mock 1, above `Group developer:` in mock 2), so a one-int layout knob is cheap insurance against a 3-second video failure. This is the one place the review's "cut it" is too aggressive |
| 16 | Remove `scheduler=threaded` | **A** | Removed. One sentence in the PPT explains the cooperative choice (the review's suggested wording is used) |
| 17 | Runbook contradicts the video rule | **A** | IDE Run/Debug is the **only** evidence path; the `.bat` is rehearsal/recovery only |
| 18 | Hash printing / committed binary is unnecessary | **A′** | Binary **never committed** (submission package / release asset). The SHA-256 stays as a one-line note in `docs/frozen-artifact.md` — it is discipline, not infrastructure; nothing prints it at launch |
| 19 | `lukka/get-cmake` unnecessary | **A** | Removed |
| 20 | CLion wording is stale | **A** | Wording softened per JetBrains' current doc (MSVC LLDB **is** supported on Windows); the verify-don't-assume gate is kept — it is what protects the graded video |
| 21 | Don't turn FSD into "architecture theatre" | **A′** | Only the layer/import invariants are used — which is exactly what v1 already did. But GPT's *proposed flattening* (`core/`, `commands/`, `marquee/`, `terminal/`) is **rejected**: `AGENTS.md` §3 mandates those FSD layers, and the file count is identical. "Don't over-engineer" ≠ "break the project's stated standard" |
| 22 | pty replay harness is too ambitious → optional | **A** | Moved to P2, non-gating |
| 23 | A6/A8/A10 are misleading tests | **A** | A6 drops "twice"; A10 drops the "100 % CPU" criterion (becomes an observation); A8 stays as a labelled stress case |
| 24 | The central architecture is worth keeping | **A** | Kept: Terminal abstraction → cooperative scheduler → PCB → renderer → single composed frame; config → CLI → runtime |

#### Also accepted as description-level corrections

- §1.2's confidence about the rubric is now backed by the handout's actual grading table.
- The `--dev` grab-bag (`ps`, `params`, `stats`) is cut to **one** flag, `--diag`, which exists solely to
  satisfy the CLion/terminal verification gate (risk 13). Removing it entirely would delete the tool that
  proves the graded run configuration is sane.
- `FakeTerminal` moves from production `shared/` into `tests/support/`.
- `ansi.cpp` and `responses.cpp` are folded into `frame_buffer.cpp` / `interpreter.cpp`.

#### Where this document disagrees with the review outright

1. **Repository restructure (point 21).** Flattening the layers would violate `AGENTS.md` §3. Rejected.
2. **`marquee_row` (point 15).** Kept, config-only, because the handout's own screenshots contradict each
   other. Rejected.
3. **Killing the pty replay harness *and* the measurement log *and* `--diag` *and* every optional knob.**
   Taken together these leave *no* automated end-to-end evidence and *no* numbers for the PPT. The review is
   right that each is optional; it is wrong to remove all of them at once. Each is now explicitly P2 or
   single-purpose instead of deleted.

---

### 4. Spec-fidelity findings that neither document had

Transcribed from the 600-dpi render of page 2 (the handout's own mock console):

```text
Welcome to CSOPESY!

Group developer:
De La Cruz, Juan
Santos, Alex

Version date:

Command>|
```

and the marquee state, whose art band sits **above** the `Group developer:` block:

```text
/ ___/ ___/  _ \/  _ \/ ___/ ___/ \ / /
| |   \__ \ | | | | | |\__ \\__ \  | | |
| |__ ___/ | |_| | |_| |___/ ___/ | | |
\____/____/ \____/\____/(___/(___/  |_|_|
```

Three things follow, all now in v2:

1. `Welcome` is capitalised (**M2**), and `Version date:` is blank (**GPT #8**).
2. The art is a **slanted outline font** (`/ \ _ |`, 4–5 rows) — *not* the filled `###` block letters v1's
   glyph table specifies. v2 keeps a fixed-width glyph table (uniform cell = safe column math) and records
   this as a **conscious, low-risk divergence**: the handout asks for *"Text marquee **or** ASCII graphics
   marquee"*, so the style is not graded; drift in glyph cell width *would* be. The one thing that must
   never happen — a ragged glyph that breaks the scroll width — is now covered by the width-invariant test.
3. The band's position is ambiguous between the two mocks ⇒ `marquee_row` is defensible (**point 15**).

---

### 5. What changed in v2 (summary)

- **One input owner**; `tick()` is pure and testable; echo latency decoupled from the refresh interval.
- Contracts trimmed to what the handout asks for: 8 parameters, 2 process states, 6 commands, 1 diagnostic flag.
- Every contradiction removed: whitespace, tearing wording, three-path claim, signal restore, runbook vs video, T4.6, version date.
- 6 additional defects fixed, including 3 in the tests the review itself flagged.
- Scope reduced: no threaded scheduler, no CSV telemetry, no `--self-test`, no live `ps`, no committed binary,
  no `lukka/get-cmake`, replay harness demoted to P2.
- Scope protected where the review over-trimmed: `marquee_row`, `--diag`, `--measure`, the 4-machine
  measurement matrix, and the FSD layer rules.

---

## Round 2 — `gpt-v2.md` against `IMPLEMENTATION_PLAN_v2.md`

`gpt-v2.md` reviews v2 (this time it read the plan, not just the adjudication). Result: **v2.1
(`IMPLEMENTATION_PLAN_v2.md`, §10.7)**. All five of its points were accepted, two of them strengthened with
evidence it did not have — but it again missed six defects, three of which are in the areas it was reviewing.

### 6.1 Claims re-verified against primary sources

| GPT-v2 claim | Evidence | Verdict |
| --- | --- | --- |
| The frozen-binary + Run/Debug setup is contradictory: "target `csopesy`" ≠ "executes `frozen/…`" | v2 line 196/983 (`target csopesy`) vs 1496/1764 (`the same binary copied into frozen/` / `PRESS RUN/DEBUG … on target csopesy`) — genuinely inconsistent | **True** |
| Therefore the quiz config should point explicitly at the frozen executable | JetBrains *Debug arbitrary executables* documents exactly this: create a custom build target and *"leave the other fields empty"*, then *"Specify the application binary in the Executable field"*; *Custom Build Application* has an `Executable` field, and a fake target means "no actual build will be performed" | **True, and stronger** |
| (not raised by GPT) the deeper reason a CMake-target config is wrong for the quiz | JetBrains *CMake Application*: *"**Build is the default pre-launch step for CMake applications.**"* A CMake-target Run press therefore rebuilds or at least relinks — precisely what *"no longer recompile the project when taking the quiz"* forbids | **New evidence, adopted** |
| `polling_ms` is a maximum wait timeout, not a sampling interval, so it may not create any typing delay | Our own §3.8: `readEvent` blocks in `poll()`/`WaitForSingleObject`, which return the instant a key arrives; and `pollTimeoutMs()` is separately capped by `msUntilNextRender`. So neither keystrokes nor frames can be delayed by it | **True** |
| `--measure` cannot produce physical key-to-echo latency | `eventMs` is the moment `readEvent` returned, i.e. after OS delivery; the physical keypress is invisible to the program | **True** |
| Multi-row command wrapping is not worth it; scroll horizontally | It also conflicts structurally with the fixed-row frame in §3.9 (a wrapped prompt must displace band/header rows) | **True, amended** |
| The tearing/delay threshold may legitimately not exist | Handout: *"Identify the limits of your refresh and polling rate – wherein there is the presence of screen tearing and/or a noticeable delay"* — asked for *where* they are present; nothing guarantees they are | **True** |
| "Simulation is not the same as executing the C++" | Correct; v2 already said "against the reference algorithm in simulation", so this is a wording sharpening rather than an error | **True, already true** |

### 6.2 Where the review was amended or strengthened

1. **Its reason for splitting the run configurations was incomplete.** GPT framed it as "those are not
automatically the same thing". The sharper, doc-verified reason is that a CMake Application config **builds by
default before launching**, so the setup does not merely risk pointing at the wrong file — it risks
recompiling during the graded run. v2.1 records the primary fix (Custom Build Application, empty build
target) *and* the alternative (override `Executable`, delete the `Build` pre-launch step), plus a T6.3 gate
that proves the SHA-256 is unchanged after a Run press.

2. **Its `polling_ms` framing was right but incomplete in a way that could have cost the knob its
existence.** GPT said it "primarily reduces idle wakeups rather than deliberately adding keystroke latency".
Accurate — but on Windows `SetConsoleCtrlHandler` runs on another thread and cannot interrupt the blocked
wait, so **`polling_ms` also bounds Ctrl+C-to-exit latency**. That is a real, measurable, user-visible job for
the parameter (and a legitimate measurement column), where "it only saves CPU" invites someone to delete it.

3. **Its horizontal-scroll point understated the case.** GPT called wrapping "surprisingly fiddly". In this
layout it is structurally incompatible with a fixed-row frame: the band, header and prompt occupy fixed rows,
so a wrapped prompt has to displace them. That is why v2.1 pins the prompt to one row and scrolls a
`visibleSlice` window.

### 6.3 Defects GPT-v2 missed (found while applying its advice)

| # | Defect in v2 | Evidence | Fix in v2.1 |
| --- | --- | --- | --- |
| R1 | **Windows `readEvent` silently drops the resize.** The snippet returns `false` on `!_kbhit()`, so the `Resize` event promised by the adjacent bullet is never produced — and nothing else in the loop resized the frame, so the layout stays at the old width after a window resize | v2 T3.1 snippet vs its own "compare on each poll to emit `Resize`" bullet | Compare `srWindow` **before** the `!_kbhit()` shortcut and return a real `Resize` event |
| R2 | **POSIX `EINTR` contradicts `SIGWINCH`.** v2 required "return `false` on timeout or `EINTR`" *and* "`SIGWINCH` ⇒ the next `readEvent` returns `Resize`". `SIGWINCH` is exactly what makes `poll` fail with `EINTR`, so the resize is swallowed | v2 T1.1, two adjacent bullets | On `EINTR`, inspect the resize flag first; return `false` only when it is unset |
| R3 | **`FrameBuffer::resize` was never called from the loop.** T4.3 tests resize invalidation, but no code path in `Scheduler::tick` ever told the buffer the terminal changed size | v2 §3.8 tick excerpt has no `resize` call | `Scheduler` tracks `fbRows_`/`fbCols_` and resizes+invalidates on change; `resize_event_forces_a_repaint` test added |
| R4 | **`--no-tty` / `isTty() == false` was undefined and self-contradictory.** v2 promised "no ANSI" for non-tty, while `FrameBuffer::renderDiff` unconditionally emits cursor addressing | v2 §3.3 vs §T4.3 | `--no-tty` is now **plain line mode** (stdin lines in, plain responses out, no frames), and it carries a CI end-to-end smoke test of the real binary |
| R5 | **`set_speed` argument parsing was ambiguous, with a latent `stoi` bug.** `std::stoi("5abc")` returns `5`, and v2 never said whether `-5` is a `Usage:` error or a clamp (A5 implies clamp) | v2 §3.7 rule 5 + A5 | Argument must match `[-+]?[0-9]+` in full; non-numbers ⇒ `Usage:`; out-of-range well-formed values (incl. negatives) ⇒ clamped |
| R6 | **`enterRawMode()` had no call site.** v2's `main` description named `installShutdownHandlers` and the RAII guard's `restore()`, but nothing invoked raw mode | v2 T2.5 Step 1 | The guard's constructor calls `enterRawMode()` (skipped when `noTty`/`!isTty()`) |

> **R1 and R2 were later redesigned, not just patched.** Round 3 (§7) established that a resize never needed to
> become an *event* on either platform, so v2.2 deletes `KeyType::Resize`, the Windows `srWindow` compare and
> the POSIX "check the resize flag on `EINTR`" rule entirely, and detects a size change from `size()` each tick
> instead. The v2.1 fixes above are recorded here as history; §7.3 explains why patching them was the wrong
> layer to fix.

### 6.4 Point-by-point verdict

| GPT-v2 point | Verdict | Decision |
| --- | --- | --- |
| 1. Two run configurations; quiz points at `frozen/` | **A′** | Adopted, with the build-pre-launch evidence and a post-freeze hash gate (T6.3 Step 0) |
| 2. `polling_ms` is a max wait, not a sampling interval — fix the wording, keep the design | **A′** | Adopted; extended to say what it *does* bound (idle wakeups, Windows Ctrl+C latency) so the knob is justified |
| 3. `--measure` cannot measure physical key→echo; rename it | **A** | Adopted verbatim; perceived typing delay becomes a human-observed column |
| 4. Horizontal input scrolling instead of multi-row wrapping | **A′** | Adopted; framed as structural (fixed-row layout), not merely simpler |
| 5. Let the tearing/delay threshold be "not observed" | **A** | Adopted in T1.4, T3.3, T5.3, T6.1 |
| + "one more thing": simulation ≠ verification | **A** | Adopted and sharpened in §6.3 of the plan |
| **Kept from v2 (GPT explicitly agreed):** `marquee_row`, FSD layers, `--diag`, `--measure`, cooperative scheduling | **✓** | Nothing reverted |

### 6.5 Verification performed before committing v2.1

The v2.1 scheduler (including the new `FrameBuffer::resize` step), `visibleSlice`, and the `set_speed` parsing
rule were executed against reference implementations in simulation — 17 checks, all passing, covering: the
refresh-deadline tests, `hasRendered` first/restart paint, resize-forces-repaint, `pollTimeoutMs` capped by the
render deadline and by `polling_ms`, echo redraw not gated by the refresh interval, `tick` never reading input,
the driven `stop_marquee` loop, `visibleSlice` width/tail, and `0 / -5 / 5abc / 1.5 / 10001` for `set_speed`.

**And that is exactly the claim §6.3 of the plan now qualifies:** this proves the *specified* algorithm is
self-consistent. It does not prove C++ compiles, that `ctest` is green, or that a real terminal behaves. The
first real verification event in this project is still T0.1's `OK 0 tests` and T1.1's hand-run on a tty.

---

## Round 3 — `gpt-v3.md` against `IMPLEMENTATION_PLAN_v2.md` (v2.1)

`gpt-v3.md` is the third review, and the first whose factual claims are **all** correct. Result: **v2.2
(`IMPLEMENTATION_PLAN_v2.md`, §10.8)**. All five points were accepted, but the review's own recommended fix for
the resize path would not have worked as written (W1), its analysis was **Windows-only** where the defect was
on both platforms (W2), its CI finding was **narrower than the actual failure** (W3), and the documentation
inconsistency it spotted was **broader than the three instances it named** (W5). It also missed the fact that
§3.1's layer rule had no enforcement mechanism at all — which is why the `entities/scheduler.cpp` violation
it correctly found had survived two previous rounds (W4).

### 7.1 Claims re-verified against primary sources

| GPT-v3 claim | Evidence | Verdict |
| --- | --- | --- |
| The plan says CMake ≥ 3.20 while its presets need 3.21 | The plan's own `CMakePresets.json` uses `"version": 3`. CMake's `cmake-presets(7)` marks `condition`, `installDir`, `toolchainFile` and "omitting `generator`/`binaryDir`" as *"Added in presets version 3"*; the **CMake 3.21 release notes** list those preset additions and state *"The Visual Studio 17 2022 generator was added."* So both the schema and the Windows generator require 3.21 | **True** |
| Windows resize cannot arrive through the path this backend uses | Microsoft's `SetConsoleMode` reference, verbatim: *"When a console is created, all input modes except ENABLE_WINDOW_INPUT and ENABLE_VIRTUAL_TERMINAL_INPUT are enabled by default."* and *"These events can be read by ReadConsoleInput, but they are always filtered by ReadFile and ReadConsole."* The backend uses `WaitForSingleObject` + `_kbhit`/`_getch`, i.e. neither `ReadConsoleInput` nor a path that receives them | **True** |
| A `printf … \| ./binary` CI step is not OS-neutral | GitHub Actions' default shell for `run:` steps on Windows runners is **pwsh**, which has no `printf` | **True** |
| `Scheduler` in `entities/` violates the plan's own layer rule | `include/csopesy/scheduler.hpp` includes `renderer.hpp` and holds `Renderer&`/`Interpreter&`. The local FSD skill states rule **4-1** (*"app → pages → widgets → features → entities → shared. Upward imports and cross-imports between slices on the same layer are forbidden."*) and rule **4-3** (*"No cross-imports between slices on the same layer"*). So `entities/` is an upward import and `features/` would be a cross-import; only `app/` is legal | **True** |
| The CLion frozen-binary solution is sound | Already verified in §6.1 against JetBrains' *Debug arbitrary executables* and *CMake Application* (*"Build is the default pre-launch step for CMake applications"*). GPT-v3 endorses the v2.1 split rather than revising it | **True, no change** |

### 7.2 W1 — the review's own recommendation would have shipped broken

This is the one place where accepting GPT-v3's advice verbatim would have produced a *worse* plan than
rejecting it.

GPT-v3 wrote, of the Windows resize:

> "Fortunately, you don't need the resize event at all. The Scheduler already does this every tick: … That is
> sufficient."

It was not sufficient. In v2.1, `Scheduler::tick` did this:

```cpp
const Size sz = term_.size();
if (sz.rows != fbRows_ || sz.cols != fbCols_) {
  fb_.resize(sz.rows, sz.cols); fbRows_ = sz.rows; fbCols_ = sz.cols;
}
// ...later...
if (due || redraw) { renderer_.drawFrame(...); term_.write(fb_.renderDiff()); ... }
```

`FrameBuffer::resize()` invalidates the frame, but **invalidation is not submission**: the frame is only sent
inside `if (due || redraw)`. With no event, `redraw` is `false`; and with the marquee stopped (or before the
next deadline) `due` is `false` too. So the exact scenario GPT described — a resize with no keypress — would
have resized the buffer and then displayed the old layout anyway, until some later event happened to repaint.
The one-word fix is `redraw = true;` inside the size-check branch, and it is now load-bearing in v2.2 (§3.8).

That this mattered is not a matter of opinion: the simulation in §7.5 fails on exactly the two assertions that
depend on it when the line is removed.

### 7.3 Defects GPT-v3 missed (found while applying its advice)

| # | Defect in v2.1 | Evidence | Fix in v2.2 |
| --- | --- | --- | --- |
| W1 | **The resize design it recommended did not repaint.** `fb_.resize()` invalidates, but submission is gated on `due \|\| redraw`; with no event and no due frame, nothing is written | §3.8 tick excerpt: the size branch set no flag, yet GPT-v3 called the existing behaviour "sufficient" | `redraw = true;` in the size-change branch; `size_change_forces_a_repaint_without_any_event` asserts it (and a second resize re-asserts it) |
| W2 | **The resize-event dependency was on POSIX too, not just Windows.** GPT-v3 only analysed the Windows path; v2.1's POSIX bullet had its own `SIGWINCH`-flag → `Resize`-event mechanism, created solely to patch the `EINTR` contradiction (R2). Fixing only Windows would have left two different resize designs and kept the contradiction | v2.1 T1.1 vs T3.1 | Both mechanisms deleted: `KeyType::Resize` removed from the enum, no `srWindow` compare, no `SIGWINCH` flag rule; a size change alone repaints (§3.3, §3.8, T1.1, T3.1) |
| W3 | **The CI smoke test's binary path is generator-dependent, so a shell fix would not have been enough.** GPT-v3 correctly flagged `printf` under pwsh, but `./build/debug/csopesy` is also wrong for the `windows-vs` preset (multi-config → `build/vs/Debug/csopesy.exe`) and for anyone using VS locally | v2.1 T0.3 + T0.2's `windows-vs` preset | The smoke test is a CTest job driven by `execute_process(INPUT_FILE)` with `$<TARGET_FILE:csopesy>`, so no shell and no path guessing; **and** it is registered in T2.5, because registering it in T0.2 (where the draft put it) would leave CI red until plain line mode exists |
| W4 | **§3.1 claimed the layer rules were "Enforced" with no mechanism.** The `entities/scheduler.cpp` violation GPT-v3 correctly identified had survived two earlier review rounds *because* nothing checked it — prose is not a guard | §3.1 "Enforced: …" vs no script anywhere in the plan | `scripts/check_layers.sh` (a ~15-line grep guard) added to T0.1 and to the CI matrix |
| W5 | **The count inconsistency was in three places, not one.** GPT-v3 found "five further defects" vs R1–R6 (six). §10.7's own introduction also said *"five changes come from the reviewer; four are defects"* while its table listed five reviewer rows and five defect rows — and R6 (`enterRawMode`) was missing from that table entirely | Plan header, §10.7 intro, §10.7 table | Header now says six and names `enterRawMode()`; §10.7 intro says five + six; the `enterRawMode()` row is added to the table |

Also corrected in passing (dangling after the resize redesign): T0.2's expected `ctest` count, T0.5 Step 5(d)'s
reference to "the `Resize`-event path", and §10.3's note about `Win32Terminal::lastSize_` (a field that no
longer exists).

### 7.4 Point-by-point verdict

| GPT-v3 point | Verdict | Decision |
| --- | --- | --- |
| 1. CMake ≥ 3.20 contradicts preset schema v3 + VS 17 2022 | **A** | Accepted verbatim. Floor, `cmake_minimum_required` and the CI comment all say 3.21; the reason is recorded next to the snippet (T0.2) |
| 2. `Scheduler` must leave `entities/`; put it in `app/` and add no interface layer | **A** | Accepted verbatim. Independently confirmed against FSD rules 4-1/4-3 (§7.1). All references updated: §3.1 rationale, §3.2 tree, §3.8 excerpt, §4.1, T0.2 `CORE_SOURCES`, T1.3 |
| 3. Drop the resize event; let `tick()` notice the size | **A′** | Accepted in direction, **fixed in substance** (W1) and **extended to POSIX** (W2). The result is smaller than either v2.1 or GPT-v3's sketch: one enum value, one Windows mechanism and one POSIX rule deleted |
| 4. Make the CI smoke test OS-neutral (`shell: bash` or PowerShell) | **A′** | Accepted, but solved one level deeper: a CTest job removes the shell *and* the generator-specific path (W3), and it moves to T2.5 so CI is green from day one |
| 5. Fix the counts/markers: "five"→"six", T2.3's "wrapping", T0.4's `contracts v2.0` | **A′** | Accepted and broadened (W5): all three named instances fixed, plus §10.7's second bad count and the missing `enterRawMode()` row |
| Closing recommendation: stop reviewing the plan and go build it | **A** | Adopted as a plan statement, not just a sentiment — §10.8's Status says this is the last revision before code, and names the next evidence (T0.1 `ctest`) instead of another round |

### 7.5 Verification performed before committing v2.2

The v2.2 scheduler — including the new `redraw = true;` on a size change — was executed against a reference
implementation in simulation: **50 checks, all passing**, covering the refresh-deadline rules, `hasRendered`
first/restart paint, cycle accounting (a redraw is not an "instruction"), echo redraw independent of
`refreshMs`, `pollTimeoutMs` capped by the render deadline and by `polling_ms`, "stopped ⇒ no repaint at all",
the shutdown flag, and the new resize path.

Two checks are specifically the v2.2 change:

```text
size_change_forces_a_repaint_without_any_event   → term.out grows after `sz = {30,100}` with NO event
second_resize_also_repaints                      → term.out grows again after `sz = {40,120}`
```

**And the counterfactual:** removing the single `redraw = true;` line (i.e. restoring v2.1's behaviour)
makes exactly those two assertions fail and nothing else — so the test is not vacuous and the defect W1
describes was real, not theoretical.

**This is still simulation, not verification** (§6.3 of the plan): it shows the specified algorithm is
self-consistent. It does not prove the C++ compiles, that the Win32/POSIX backends behave, that `ctest` is
green on three OS images, or that a real terminal renders a resize the way §3.8 says it will. Those are
T0.1–T1.1's job, on real hardware, and they are the only evidence this project will accept as done.

---

## Round 4 — `gpt-v4.md` against `IMPLEMENTATION_PLAN_v2.md` (v2.2)

The most substantive round since the first: all four actionable points are correct, and one of them is a
**genuine build blocker** that I reproduced before accepting it. Result: **v2.3** (§10.9 of the plan). Two
must-fix corrections, two cleanups — and one point the review filed as philosophy that turned out to be a real
contradiction inside the plan's own definition of done.

### 8.1 Claims re-verified against primary sources

| GPT-v4 claim | Evidence | Verdict |
| --- | --- | --- |
| `add_compile_options($<…>/W4$<…>-Wall -Wextra>)` breaks the build | Reproduced locally (CMake 4.3.0, GCC 16.2.1, Ninja): configure succeeds, `flags.make` holds the fragments `-Wall"` and `-Wextra>"`, and the build fails with `c++: error: unrecognized command-line option '-Wextra>'; did you mean '-Wextra'?` | **True — build-blocking** |
| The fix is `if(MSVC) /W4 else() -Wall -Wextra endif()` | Same probe, second variant: configures, compiles, links; `-Wall` and `-Wextra` both appear on the real compile line | **True, verified** |
| Nothing says how `config/quiz_case_<n>.ini` becomes the file that is loaded | `grep quiz_case` on v2.2 returns exactly **one** hit — the runbook line — while the graded run config passes the literal `--config=config/csopesy.ini` (§T0.5). No task creates the case files; no step selects one | **True** |
| The §10 navigation row is stale | §0's table says §10 is for "anyone auditing v1→v2" while §10 now holds the v1→v2, v2→v2.1 and v2.1→v2.2 logs | **True** |
| Don't demand "failing test first" for platform-integration details | §5 said *"Every task is TDD"*, §6.1 item 1 said *"The task's test existed, failed before the implementation…"* and §6.3 said *"the task's test must have failed before the change"* — while T1.1/T3.1 ship hand-run checklists instead of tests | **True — and worse than filed** (§8.3) |

### 8.2 The CMake defect, in full

One line, and it would have stopped T0.1 inside its first minute on every non-MSVC machine at once:

```cmake
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/W4>$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>)
```

CMake's language parser splits **unquoted arguments on whitespace before generator expressions are
evaluated**, and `$<…>` does not protect that space. The argument breaks in two, the second piece degenerating
into the literal string `-Wextra>`. Configuration stays clean; only compilation fails — the worst kind of build
error, because it survives every "does it configure?" sanity check and shows up on the machines nobody is
sitting at.

Two things were added beyond the substitution itself:

1. a comment at the fix forbidding the one-liner's return ("do not *simplify* this back"), and
2. a T0.2 Step 1 check that the warning flags **actually reach the compile line**, because a silent split is
   invisible otherwise.

Note: GPT-v4 reported the mangled intermediate text as `"$<1:-Wall" "-Wextra>"`. My reproduction shows the
same failure with slightly different artifacts (`-Wall"` / `-Wextra>"` in `flags.make`); the diagnosis and the
conclusion are identical, and the exact spelling depends on the CMake version. The *test* is the compiler
error, not the mangled string.

### 8.3 Where the review undersold its own point

1. **Its TDD point was not philosophy — it was a contradiction.** GPT-v4 offered it as advice on how to *read*
the plan. It is stronger than that: §5, §6.1 and §6.3 all *state* a rule that T1.1/T3.1 violate structurally.
Left as a matter of reading, four people resolve it four ways — one writing a theatrical `tcsetattr` unit test,
another quietly ignoring the rule. v2.3 settles it in the document: the platform-surface tasks are named as
exceptions and their recorded transcript is declared to *be* the test.

2. **The quiz-case fix needed a definition, not just a step.** GPT-v4 asked for the copy procedure to be made
explicit in §7 and T6.3 — correct — but `config/quiz_case_<n>.ini` was referenced *only* in that single runbook
line, so the convention also had to be **defined** (§3.6) or the runbook keeps pointing at files no task
produces. Its danger is sharper than "unnecessary failure" too: the plan itself specifies that a missing config
file is **non-fatal**, so skipping the copy yields the *previous* case's parameters with no error message — a
silent wrong answer under time pressure, not a crash.

### 8.4 Point-by-point verdict

| GPT-v4 point | Verdict | Decision |
| --- | --- | --- |
| 1. Fix the broken `add_compile_options()` | **A** | Accepted verbatim — reproduced first, then fixed, with a regression comment and a T0.2 check that the flags reach the compiler |
| 2. Make quiz-case → `config/csopesy.ini` explicit | **A′** | Accepted; extended to define the convention in §3.6 and to add the stale-config verification to T6.3 |
| 3. Shorten T1.1's stale resize-history explanation | **A** | Accepted. The bullet states the instruction and points to this file for the post-mortem |
| 4. Fix the stale §10 navigation label | **A** | Accepted; now reads "the plan revision history (v1 → v2 → v2.3)" |
| 5./6./7. Scheduler design, the resize redesign and the portable CTest smoke test are right — leave them | **✓** | Nothing touched. The `redraw = true` counterfactual and the `$<TARGET_FILE:>` smoke test both stand as written |
| 8. Interpret TDD pragmatically for platform work | **A′** | Accepted and promoted from advice to contract (§8.3 item 1) |
| Final call: fix these two, start T0.1, and don't redesign again without build/test evidence | **A** | Adopted as the plan's own status rule (§10.9): v2.3 changes only on evidence — a green `ctest`, a three-image compile, a terminal transcript, or a failing acceptance case |

### 8.5 Verification performed before committing v2.3

No algorithm changed in this round, so the v2.2 scheduler simulation of §7.5 was **re-run unchanged: 50
checks, all passing**. The new evidence is the CMake A/B probe (CMake 4.3.0 + GCC 16.2.1 + Ninja):

```text
A (v2.2's one-liner):  configure: OK   build: c++: error: unrecognized command-line option '-Wextra>'
B (v2.3's if/else):    configure: OK   build: [2/2] Linking CXX executable p   (-Wall -Wextra on the line)
```

What this does **not** establish: that the plan's CMake builds as a whole (it is a snippet, not a project),
that MSVC accepts `/W4` (unverifiable here — the Windows owner confirms it under the T0.2 checkbox), or that
any of it behaves in a real terminal. Those remain T0.1–T1.1's job. As before: a simulation and a two-file
probe are not verification of the project.

---

## Round 5 — `gpt-v5.md` against `threading.md` + `IMPLEMENTATION_PLAN_v2.md` (v2.4)

The first review of the **threading** artifact rather than of the plan as a whole. `gpt-v5.md` reviewed
`threading.md` and v2.4 as a design (deliberately without source code, which does not exist yet) and says so
explicitly. Result: **v2.5** (§10.11 of the plan). Its direction is right, **all seven points and its closing
warning are accepted** — three of them sharpened past what it proposed — but it **missed four defects of the
same class as the ones it found, two of them inside the `--measure` area it was reviewing**. Its recommendation
to *document* the coalescing rather than queue timestamps was the correct call; its assumption that the
critical section contains only rendering was not.

### 9.1 Claims re-verified against primary sources

| GPT-v5 claim | Evidence | Verdict |
| --- | --- | --- |
| `--measure` cannot produce "one line per keystroke" | §3.6 said *"one `eventMs,echoMs` line per keystroke"* and then, two paragraphs later, *"if several keys land between two frames they share one `echoMs`"*; §3.8 had a single `pendingEventMs_` slot overwritten by every `postEvent` | **True — an internal contradiction** |
| The mutex is held across rendering | §3.8's snippet calls `renderer_.drawFrame(...)` and `fb_.renderDiff()` while `mu_` is held, unlocking only before `term_.write()` | **True** |
| "the slow terminal write cannot stall the input thread" is true but incomplete | Same snippet: the *write* is outside the lock, the *render* is inside it, so `postEvent()` can still block | **True** |
| The concurrency tests do not create genuinely concurrent activity | Step 1b's `drainInput`/`typeLine` are called by the test thread itself, and `threaded_stress_typing_while_animating_at_one_millisecond` pushes all 20 keys, then drains, then wakes | **True** |
| `start()` is not exception-safe | §3.8: `{lock; if (started_) return; started_ = true;}` then `worker_ = std::thread(...)` — a throwing constructor leaves `started_ == true` with no worker | **True** |
| `joinable()` is safe only because the ownership rule is strict | §3.8's table assigns `worker_` to the input thread alone | **True** (a discipline to preserve, not a defect) |
| The `tick(ev)` compatibility path invites future drift | Two input paths — `tick(ev)` and `postEvent(ev) → tick(nullptr)` — with one body | **True** (a maintainability warning, not a defect) |
| Keep the condition-variable design; no `sleep_for`-only shutdown | §3.8's `cv_.wait_for` predicate plus §3.6's "even the threading tests need no sleeps" | **True** |
| Closing: "implementation-ready and frozen for paper review" is a paper-review + simulation claim, not evidence the C++ is safe | §10.10's own Status said exactly that; §6.3 already says *"simulation is not verification"* | **True — and the plan was overclaiming in its own words** |

### 9.2 Where the review was amended or sharpened

1. **Its `--measure` fix was necessary but incomplete.** GPT offered two options: document the coalescing, or
   replace the single slot with a timestamp queue. Both leave the actual defects in place. The slot was
   **overwritten** by every `postEvent`, so N queued keys reported the **newest** — precisely the sample that
   *understates* worst-case latency in a column whose purpose is a latency tail; and it was **never cleared**,
   so the "owed" pair would have been re-emitted on **every** subsequent frame. v2.5 documents the coalescing
   (GPT's option A, adopted) **and** fixes both (Y3), which needs one `if` and one `bool`.

2. **Its mutex point was right about the wrong boundary.** GPT compared "rendering under the lock" with "a
   snapshot". It did not distinguish the *pure* render from the **syscall** in the same critical section:
   §3.8's `tick()` also called `term_.size()` — an `ioctl`/`GetConsoleScreenBufferInfo` — under `mu_`, even
   though the ownership table gives `size()` to the marquee thread alone. Rule 3 hoists it (Y2), which shrinks
   the critical section below what either of GPT's options proposed and makes the latency bound easier to state.

3. **Its test-strength point needed two artifacts, not one.** GPT asked for "one real integration/manual test".
   The plan needs a real **two-thread test** that is still deterministic (GPT did not say how to keep it
   sleep-free — the answer is a barrier on the existing `waitForFrames`, not a timing window) *and* the named
   A8 transcript. §6.1 DoD 7 and §6.3 now say which one proves what, and explicitly stop claiming the
   deterministic suite demonstrates overlap.

4. **Its exception-safety fix is superseded by a smaller one.** GPT proposed making the state transition
   exception-safe. v2.5 instead **deletes `started_`**: `worker_` is already input-thread-only, so
   `joinable()` *is* the state, the field count drops, and `start()` becomes exception-safe by construction.

### 9.3 Defects GPT-v5 missed (found while applying its advice)

| # | Defect in v2.4 | Evidence | Fix in v2.5 |
| --- | --- | --- | --- |
| Y1 | **The `--measure` file had no plumbing.** The snippet wrote `measure += …` against a stream and a path that exist nowhere; no task said how `--measure=FILE` reaches the writer | §3.5's `Parameters` had no path field; §3.8's class had no stream; §3.6 named only the flag | `Parameters::measurePath` (infra, CLI-only, written before any thread exists) + the marquee-thread-only `std::ofstream measure_`, opened lazily |
| Y2 | **`tick()` held `mu_` across `Terminal::size()`** — a syscall — although §3.8's own ownership table assigns `size()` to the marquee thread exclusively | §3.8 table vs its own snippet | Rule 3: read `size()` before the lock; only the `fbRows_`/`fbCols_` comparison stays inside; the T1.1 double's `size()` counter remains the tick barrier because `size()` is still tick()'s first action |
| Y3 | **The event stamp was lossy and sticky.** `postEvent` overwrote `pendingEventMs_` (newest key wins ⇒ understated worst case) and nothing ever cleared it (the pair repeats forever) | §3.8's `postEvent` and the un-cleared field | `noteEventLocked` keeps the **oldest** stamp; `eventOwed_` makes it a one-shot. A `0` sentinel is unsafe because the injected clock legitimately starts at `0` (Step 1 uses `clock = 0`), hence a bool |
| Y4 | **The frame line's write site was ambiguous.** Its comment sat inside `if (due \|\| redraw)` while §3.6 says "one line per rendered frame", so keystroke echo frames would have entered the refresh-cadence series and corrupted the p50/p95 interval numbers | §3.6's metric intent vs §3.8's comment placement | The frame line is emitted **only** under `if (due)`; an echo frame emits the event pair and no frame line |

### 9.4 Point-by-point verdict

| GPT-v5 point | Verdict | Decision |
| --- | --- | --- |
| 1. `--measure` cannot give one line per keystroke | **A′** | Contract rewritten to "one pair per echo frame"; coalescing documented **and** two further defects fixed (Y3), with the plumbing supplied (Y1) |
| 2. The lock is held across rendering | **A′** | Accepted and extended: rule 3 removes the syscall from the critical section (Y2), and §3.8 states the honest bound ("the unbounded console write cannot stall input", not "input never waits"). The snapshot alternative is rejected as scope |
| 3. Concurrency tests are weaker than claimed | **A** | Real two-thread overlap test added (barrier-stopped, no sleeps); A8 becomes the named real-concurrency acceptance case; DoD 7 + §6.3 record what the deterministic suite does and does not prove |
| 4. `start()` exception safety | **A′** | Accepted; solved more simply by deleting `started_` |
| 5. `joinable()` relies on strict ownership | **A** | Ownership rule stated in the header, the table and the snippet, with `join()`-from-`run()` forbidden |
| 6. `tick(ev)` compatibility path is awkward | **A** | Labelled TEST-ONLY in the contract, in T1.3 Step 1 and in §10.3; both paths share one locked body |
| 7. Keep the cv/timed-wait design, avoid rigid `sleep_for` bans | **✓** | Unchanged |
| Closing: paper-review-ready ≠ implementation-safe | **A** | §10.10's Status reworded: races, lifetime and deadlock freedom are unverified claims until T0.1 + T1.3 exist |

### 9.5 Where this document disagrees with the review outright

1. **Its `--measure` "option B" (a queue of timestamps) is rejected.** A queue buys per-keystroke fidelity that
   the handout does not grade and the PPT does not need, while adding a data structure to the one place v2.4 was
   already too clever. The oldest-unconsumed stamp gives the latency **tail** — the number a "noticeable typing
   delay" claim rests on — with one slot and one boolean.
2. **Its implied reading that the deterministic suite is the wrong kind of test is rejected.** The suite is the
   right test for race-freedom and state serializability; it was the *claim* around it that was wrong. The fix
   is to add real-overlap evidence, not to weaken the deterministic suite (which is what keeps threading tests
   from flaking).

### 9.6 Verification performed before committing v2.5

The only algorithm v2.5 changes is the `--measure` rule, so it was exercised against a reference implementation
in simulation, the same standard as §7.5 and §8.5: **11 checks, all passing**, reproducing both new T1.3 Step 1c
tests plus four counterfactual checks against v2.4's rules — the counterfactual makes the new tests fail, so
they are not vacuous and **Y3 was real, not theoretical**.

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

**Not verified, and not claimed:** the `size()`-before-the-lock hoist (Y2) and the lock-cost paragraph are
*arguments* from the §3.8 ownership table, not measurements — no C++ was compiled for this round either, so
TSan cleanliness, real-terminal behaviour and the practical size of the render-wait window all remain T0.1–T1.4's
job. That gap is exactly what the review's closing warning was about, and it is now stated in the plan.

---

## Round 6 — `gpt-v6.md` against `threading.md` + the handout

`gpt-v6.md` is a **scope** review: it re-read the handout, concluded the PDF never mandates threads, and asked
for a "minimum safe" design with the synchronization and measurement machinery trimmed. Its factual claims are
almost all correct, and its central instinct — don't over-engineer a 7-day student assignment — is the same
instinct rounds 1 and 3 already applied. But it was written **without `REVIEW_ADJUDICATION.md`**, so it
re-litigates points already settled (FSD enforcement, the CI matrix, `--diag`, macOS, the quiz-case files), and
it proposes deleting three pieces of state that are load-bearing. Result: **2 accepted in spirit, 4 rejected with
reasons, 0 new defects** — its one defect (the `--measure` coalescing) is v5's, and it arrives at the weaker fix
(delete the metric).

### 10.1 Claims re-verified against primary sources

| GPT-v6 claim | Evidence | Verdict |
| --- | --- | --- |
| The handout does not explicitly mandate threads | The handout asks for a scheduler implementation and a refresh-vs-polling explanation; it never names threads. `threading.md` is the **owner's** instruction, which is why the design stands regardless | **True, no consequence** — recorded as risk 9's framing, not acted on |
| The `pendingEventMs_` scheme doesn't actually give one measurement per keystroke | Same evidence as §9.1 | **True** (v5's defect; v6's fix is weaker) |
| `wakeTick_` is "probably unnecessary" unless it solves a concrete missed-wakeup problem | It does solve one: it is the edge that lets a test force a re-evaluation under the injected clock **without repainting**. Deleting it forces real-millisecond waits (breaking §6.1 DoD 2) or `dirty_`-faked redraws (which change the frame semantics the tests assert) | **False as stated** |
| `SchedulerSnapshot` is "fine for tests/debugging, but not essential" | It is the only lock-safe read of live scheduler state for tests and `--diag`; without it, the live-state assertions would have to move after `join()` and would weaken. It is 6 lines | **Half-true — kept** |
| "Extensive revision-history/self-audit sections are not part of the product" | True as a statement about the *product*; these logs are this project's evidence trail and the handout grades the explanation | **True, out of scope** |
| "Thread per process" should not be introduced | Correct; §9 Q3 no longer floats it | **True — adopted** |
| Don't expand FSD enforcement because of threading | Correct, and the plan never did: §3.1's threading note is three lines stating that no layer rule changes | **True, already true** |
| CI across three OS families is unnecessary | Already adjudicated in round 3: the group has four machines and CI is what catches a compile break on a machine nobody is sitting at. It is cheap and it is the §6.1 DoD 3 carrier | **Rejected** |
| All six commands, text/ASCII marquee, scheduler, one terminal writer, deterministic tests, no detached thread | Directly agree with the plan | **✓ — nothing to do** |

### 10.2 Where the review is right but already applied

Everything in its 🔴 column except `wakeTick_`, the `--measure` metric and the concurrency matrix was
already decided in rounds 1–3: `--diag` exists solely for the T0.5 verification gate, the CI matrix is the
DoD-3 carrier, macOS is a compile-only target, and the FSD layers are mandated by `AGENTS.md` §3 — the
same reason round 1 rejected GPT's flattening proposal. v6 arrived at the same conclusion itself
("keep existing structure if already required by your project"). No action.

### 10.3 Point-by-point verdict

| GPT-v6 recommendation | Verdict | Decision / reason |
| --- | --- | --- |
| Keep: 2 threads, 1 mutex, 1 cv, 1 terminal writer, no detach | **✓** | Already the design; nothing reverted |
| Keep: worker joins on shutdown, `stop`/`start` deterministic, input responsive | **✓** | Already the design |
| Delete `wakeTick_`/`wake()` | **R** | Load-bearing for the no-sleep test property (§10.1) |
| Delete `SchedulerSnapshot` | **R** | The only lock-safe live read; 6 lines |
| Trim the concurrency test matrix | **R** | Each test maps to a `threading.md` requirement (1–9, 11); the matrix is deterministic and fast, and it *is* the evidence §6.3 accepts |
| Cut `--measure` to `timestamp,frame_number`, measure latency by hand | **R** (partly A) | The implementation is ~5 lines and generates two of T1.4's four columns automatically; perceived typing delay is *already* manual. Cutting it deletes T1.4 columns 1–2 |
| Don't build a testing framework around threading | **✓** | No framework exists — a zero-dependency harness plus two sync counters |
| Simplify the plan's prose / drop revision logs | **R** | The logs are the audit trail the adjudication process depends on |
| Don't float thread-per-process as P2 | **A** | §9 Q3 de-scoped |
| "Two threads are 🟡, not required by the PDF" | **✓** | Recorded as framing (risk 9, §1.2); not a reason to change the design, because `threading.md` is the instruction |

### 10.4 Where this document disagrees outright

1. **Deleting `wakeTick_` would trade determinism for a smaller header.** It is not speculative state: it is
   the mechanism that makes "no real-time sleeps anywhere in the suite" (§6.1 DoD 2) possible for a threaded
   scheduler. This is the round's load-bearing rejection.
2. **The `--measure` metric should be fixed, not deleted.** v6's own instinct (remove complexity) is better
   served by v5's option A plus Y3/Y4 than by removing a measurement the PPT is graded on.
3. **A smaller test matrix is not a smaller risk.** The threaded scheduler is the one place in this project
   where a defect is a hang or a crash rather than a wrong string; each of those tests exists because a specific
   `threading.md` requirement names it.
4. **"Don't over-engineer" cannot mean "de-scope already-adjudicated evidence."** CI, `--diag` and the layer
   guard were each accepted in earlier rounds on reproduced evidence; re-litigating them from the handout alone
   is what the adjudication file exists to prevent.

### 10.5 Verification

v6 changed no algorithm, so no new simulation was run. The single change any of its accepted points touches is
covered by §9.6's 11-check `--measure` simulation. Its one *rejected* algorithmic suggestion (delete
`wakeTick_`) has a counterfactual already in the plan: §6.1 DoD 2's `grep -n 'sleep_'` on the test file would
start printing matches the moment the tests had to wait for real time.

---

## Round 7 — `gpt-v7.md` against the revised v2.5 plan + this adjudication

The first review that read `REVIEW_ADJUDICATION.md` rather than only the plan. `gpt-v7.md` found **one** issue it
would block implementation on, endorsed the v2.5 changes and every "keep" decision from §9–§10, and closed by
saying to stop reviewing the paper and start compiling. Result: **v2.6** (§10.12 of the plan) — one accepted
change, one **false positive corrected**, and its closing advice adopted as the plan's status rule.

### 11.1 The filed defect, re-verified

| GPT-v7 claim | Evidence | Verdict |
| --- | --- | --- |
| "`quit_` still has two unsynchronized readers" — the worker reads it under `mu_` in `tick()`, the input thread reads it in its observe step without the lock, "so the mutex does not synchronize those two accesses" | §3.8's `tick()` had `res.quit = interp_.quitRequested();` inside the lock; §3.10 step 2 has the input thread test `interp_.quitRequested()` outside it. Both reads are real | **The accesses are real; the race is not** (below) |
| "the plan's own ownership table says `Interpreter` is shared and protected by `mu_`" | §3.8's table lumped `line_`/`message_`/`quit_` into one row with "reader: both, protected by `mu_`" | **True — and that row was inaccurate.** Fixed either way |
| "the worker doesn't actually need to know whether the user requested exit; the input thread already detects quit" | §3.10 steps 2–3 already have the input thread observe `quit_` and call `requestStop()`, and `run()`'s top-of-loop `if (stop_)` already returns | **True — the worker's read was redundant** |
| "`TickResult::quit` can remain for the preserved old scheduler tests, or be removed if you are willing to update those tests" | Every `tick()` call in the T1.3 blocks is `(void)h.sched.tick(...)` — 25 hits, none an assertion | **Removed; no test needed updating** |
| Keep `wakeTick_`, `SchedulerSnapshot`, the one `--measure` mechanism, CI, FSD | It read §9–§10's reasons and agreed with them | **✓ — nothing reverted** |

**Why there was no data race.** A data race requires two accesses to the same memory location, at least one a
write, on different threads, with no happens-before edge. `quit_` had exactly three accesses:

| Access | Thread | Lock held? |
| --- | --- | --- |
| **write** — `Interpreter::feed`/`executeLine`, reached through `postEvent` | input | **yes** (`mu_`) |
| read — `interp_.quitRequested() \|\| shutdownRequested()` (§3.10 step 2) | input | no |
| read — `res.quit = interp_.quitRequested()` in `tick()` | worker | **yes** (`mu_`) |

Pairwise: **(write, input-read)** are the same thread, so they are sequenced-before and need no lock;
**(write, worker-read)** both hold `mu_`, so the mutex orders them — this is the only pair that required
synchronization, and it was synchronized; **(input-read, worker-read)** are two reads, which cannot race by
definition. v7's sentence is true only of the two reads, i.e. of the one pair that never needed synchronizing.
The model was consistent; the *table* was not.

This is the **third** round in which a correct direction arrived with an incorrect reason attached
(§7.2's W1 — where the review's own resize fix would have shipped broken; §8.3's "it's only philosophy" point —
which was a real contradiction). The pattern is worth naming: a reviewer who cannot see the source reasons about
the *shape* of the synchronization, and shape-level thinking produces both false positives and useful
simplifications in the same paragraph.

### 11.2 Why the fix is still accepted (as A′)

"Correct, because the sole writer happens to hold the lock and happens to be the only writer" is exactly the kind
of inherited argument a 7-day project should not have to defend in a graded report. Deleting the worker's read:

- makes `quit_` **single-threaded outright**, so §3.8's table gains a row that needs no caveat;
- removes the last hedge in §3.8 (the old table row, and §3.11's parenthetical, both had to explain it);
- shortens `run()` by a branch, and makes `stop_` the worker's single, explicit exit condition;
- costs nothing: §3.10 already routes shutdown through `requestStop()`, and no test read the removed field.

So the change is **adopted**, and the diagnosis it came with is **rejected and recorded** rather than quietly
absorbed — the same treatment as §7.2/§8.3, in the opposite direction.

### 11.3 Point-by-point verdict

| GPT-v7 point | Verdict | Decision |
| --- | --- | --- |
| 1. `quit_` has unsynchronized readers — fix before implementation | **A′** | Worker's read deleted, `quit_` is input-thread-only, table row split out, `TickResult::quit` removed. **Reason corrected:** no data race existed; the redundancy was the real justification |
| 2. Production `run()` should not depend on `TickResult::quit` | **A** | `run()` exits solely on `stop_`; the field is gone |
| 3. The mutex scope is now defensible; the remaining CPU-render cost is acknowledged | **✓** | No change |
| 4. The `--measure` fixes, `started_` removal, tests and real-concurrency evidence are right | **✓** | No change |
| 5. Keep `wakeTick_`, `SchedulerSnapshot`, the measurement mechanism, CI, FSD | **✓** | Nothing reverted; §10.11's rejections stand |
| Closing: stop reviewing the paper; let the compiler/tests/TSan find the next problems | **A** | §10.12's status: v2.6 is the last paper revision; the next edit requires evidence |

### 11.4 Defects GPT-v7 missed

**None.** Unlike rounds 1–5, this round surfaced no additional defect of its own class. The only inaccuracy it
touched — §3.8's ownership table conflating `quit_` with `line_`/`message_` — is the one its (correct) direction
already forced us to fix, and it is recorded here as the table split, not as a separate finding.

### 11.5 Verification performed before committing v2.6

No algorithm changed: the round deletes one read and one return field. The evidence is a **static check**, and
it is the entire justification for removing the field:

```text
grep -n 'res\.quit\|if (tick(.*)\.quit' IMPLEMENTATION_PLAN_v2.md   → 0 code hits
grep -n 'sched\.tick('               IMPLEMENTATION_PLAN_v2.md   → 25 hits, all `(void)h.sched.tick(...)`
```

So the Step 1 / 1b / 1c test blocks remain **byte-for-byte unchanged**, and §10.10's central claim ("the
architecture changed; the tests did not") survives to v2.6. `TickResult::rendered` is kept as the pure step's
observable result, with the same discarded-return status.

**Not verified, and not claimed:** nothing was compiled; races, lifetime and deadlock freedom remain arguments
from the §3.8 ownership table. That is the last thing `gpt-v7.md` said, it is correct, and §10.12 now makes it the
plan's closing status.

### 11.6 Where this document disagrees with the review outright

1. **The race claim.** As filed, "a real concurrency defect" is wrong (§11.1). It is accepted as a *simplification*,
   not as a fix, and the plan records the corrected reason rather than the reviewer's — otherwise a future reader
   would inherit a false belief about how the mutex protects `quit_`.
2. **Nothing else.** This round's remaining recommendations are consistent with §9–§10, and re-opening a settled
   design on paper would violate the plan's own status rule (§10.9/§10.12): change on evidence, not on review count.

---

### Closing — paper review is closed (confirmation pass: `gpt-v8.md`)

`gpt-v8.md` is deliberately **not** a numbered review round. It read the v2.6 plan and this file, found **no
design findings**, confirmed the §11 adjudication — explicitly including the corrected `quit_` reasoning — and
asked only that this header's round count be fixed. That fix is applied above (line 3), a fittingly small end to
a file that had already caught a count inconsistency of exactly this kind in §7.3 (W5).

It also endorsed the freeze in its own words: *"v2.6 is good enough to implement… do not keep adding
synchronization abstractions"*, and listed what must **not** be added from here (a second mutex, atomics per
flag, a third thread, a pool, a queue, a per-process worker, a snapshot/render redesign, a timestamp queue).
**Nothing in the plan changed in response to this pass** — that is the point of freezing it.

So **v2.6 remains the last paper revision.** The next entry in this file should adjudicate **implementation**
evidence — a green `ctest`, a TSan/ASan transcript, a terminal recording, or a failing acceptance case — rather
than another hypothetical paper defect. One caveat worth keeping: `gpt-v8.md` was not in the repo when this file
was last counted, so if it is later promoted to a formal review round, it takes number 8 and this closing note
becomes §12.
