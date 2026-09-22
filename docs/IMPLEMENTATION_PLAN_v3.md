# CSOPESY Marquee Operator — Implementation & Workload Plan (**v3**)

**Status: COMPLETE — and v3.0 is APPLIED to the tree (T0.6 landed 2026-09-22).**
Stages S1–S4 have landed: §0, §1, §2, §3.0, §3.1–§3.12 (the delta contract), §4 (workstreams + RACI),
§5 (task backlog, incl. **T0.6**) and **§6–§10** (definition of done + A1–A12, runbook, risks,
the professor-answer record, self-review + revision log). **Read `docs/PLAN_V3_PROGRESS.md` first** — it says
what is done, what is next, and what is blocked.

**The headers on disk ARE v3.0.** T0.6 was ratified by W2 Byron under §4.5 and landed as commit **`d6379df`**:
the header set went 13 → 11, `CONTRACTS.md`'s marker is **v3.0**, and the guard reports
`check_layers: OK (22 files scanned)`. So §5.0's **D1 gate** — the ratification — is satisfied and recorded as
**D18**; everything below describes the tree as it now stands, and the next work is **Phase 1** (step 5 of
`PLAN_V3_PROGRESS.md` §9), not any more migration.

**Version:** v3.0-draft · **Established:** 2026-09-22 · **Deadline:** Monday **2026-09-28** (revised from
2026-09-23 on 2026-09-22 — D17, `PLAN_V3_PROGRESS.md` §6) · **Supersedes:** `docs/IMPLEMENTATION_PLAN_v2.md`
(v2.6, 2026-09-16 — kept in place, marked superseded, never edited again).

**What v3 is, in one sentence:** v2.6 with the professor's five answers applied — no config file, no ASCII-art
glyph engine, no `marquee_row`, no `--diag`, no `--measure` telemetry, no `FrameBuffer` diffing and no injected
clock — and with everything Phase 0 already verified (two threads, FSD layers, the contract freeze, the 3-OS CI
matrix, the zero-dependency harness) deliberately kept.

**Why v3 rather than trimming v2.6:** every `src/*.cpp` is still a `TODO` stub (230 lines) and the headers are
303 lines, so nothing compiles against these contracts yet. This is the cheapest possible moment to change them
(§"Findings" in the progress log).

---

## 0. How to read this document

| Question | Answer |
| --- | --- |
| How should this be built? | §5 (task backlog) — arrives in S3. Until then, v2 §5 is the only task list, and it is **wrong** wherever §3.0 below says so. |
| What is the interface? | §3 (contracts) + `include/csopesy/*.hpp`. Until S2 lands, the headers on disk are **v2.6**, not v3. |
| Why is it like this? | `docs/REVIEW_ADJUDICATION.md` for decisions up to v2.6; §3.0 and the v3 revision log (§10) for the v3 deltas; `docs/PLAN_V3_PROGRESS.md` §4 for the dated decision list. |
| What is graded? | §1 (spec decode) and §6 (definition of done, acceptance cases A1–A12). |
| What may I not "simplify"? | §3.0 table B — the kept list — and `AGENTS.md` §6. |

**Convention inherited from v2, kept:** task ids keep their v2 numbers where the task survives. `T1.3`,
`T2.5`, `T0.5`, `T4.1`–`T4.3` still mean what the handoff and the adjudication think they mean; §6 of the
progress log records which of them were re-scoped.

---

## 1. Spec decode — what is actually graded

### 1.1 Functional requirements (traceability)

| # | Requirement (verbatim intent) | Where v3 satisfies it |
| --- | --- | --- |
| R1 | "OS emulator" that accepts command input with display output | `src/app/console_app.cpp` + `src/features/commands/interpreter.cpp` |
| R2 | Main menu console: welcome line, group developer names, version date, prompt | `src/features/marquee/renderer.cpp`'s frame builder (§3.9) |
| R3 | *"Text marquee **or** ASCII graphics marquee"* (handout p.2, verified verbatim) | **Plain text scroll**, §3.9 — the handout offers the choice and the professor confirmed the text branch (answer #2) |
| R4 | Commands that change the marquee's behavior | `set_text`, `set_speed`, `start_marquee`, `stop_marquee` |
| R5 | `help` — displays the commands and their descriptions | §3.7 response table |
| R6 | `start_marquee` / `stop_marquee` | `MarqueeProcess::start()/stop()` |
| R7 | `set_text` — accepts text input, displays it as a marquee | `Parameters::text` → the band re-slices from the new text |
| R8 | `set_speed` — marquee refresh **in milliseconds** | `Parameters::refreshMs` (clamped) |
| R9 | `exit` — terminates the console | also restores the terminal (raw mode off, cursor shown), exit 0 |

**Assessment constraint (not a functional requirement):** *"You should only modify the parameters and no longer
recompile the project when taking the quiz."* (handout p.2, verbatim). See §1.2.

### 1.2 The assessment method, and the five answers that produced v3

The handout: *"The project will be assessed through a black box quiz system in a time-pressure format. This is to
minimize drastic changes or 'hacking' your project to ensure the test cases are met. You should only modify the
parameters and no longer recompile the project when taking the quiz."*

**The professor answered our five questions on 2026-09-22.** These answers are the primary new evidence for v3;
they are recorded here as they appear in `docs/replies/gpt-v11.md`, because the raw reply is not committed
(progress log §6, provenance gap).

| # | Question | Answer | What it changed in v3 |
| --- | --- | --- | --- |
| 1 | Does "modify the parameters" mean editing a **parameter file** on the frozen build, or typing parameters **as commands**? | **No `.ini` file is needed.** The quiz uses the already-built program, and parameters are changed through the supported inputs/commands, without modifying or recompiling the source. | `.ini`, `config_io.*`, `--config`, `quiz_case_<n>.ini` and the copy-over runbook step are **removed** (D1) |
| 2 | Is the marquee expected to be **ASCII-art graphics**, or is plain text scroll sufficient? | **Plain text scroll is sufficient** — the handout allows *"Text marquee or ASCII graphics marquee"*. | The 5×5 glyph engine, `ascii_art` and `--plain`/`--art` are **removed** (D2) |
| 3 | For *process representation* and *scheduler implementation*: is the expected shape **two threads** — one for the marquee/scheduler, one for the command interpreter? | **Yes** — the professor instructed two threads: one for managing the marquee, one for the command interpreter. | The two-thread design is **kept and is now professor-mandated**; `gpt-v9`/`gpt-v10`'s attempt to cut it is rejected (D8) |
| 4 | Is the quiz run on your machine or ours, and does the platform matter? | **The professor's Windows machine.** | Windows is the **graded** platform; the graded run config targets Windows; CI keeps the other two compiling (D9, §2) |
| 5 | Where exactly should the marquee band sit? | **Near the top of the console where it is clearly visible.** The exact row does not need to be configurable. | `marquee_row` is **removed**; the band row is a renderer constant (D3) |

**Three-graded-outcomes table, quoted verbatim from the handout:**

| No points | Partial points | Full points |
| --- | --- | --- |
| "The CLI did not pass the test case. **NO WORKAROUND is available** to produce the expected output." | "The CLI did not pass the test case. **A workaround is available** to produce the expected output." | "The CLI passed the test case using **varying inputs** and produced the expected output." |

**What survives of v2's "partial-credit hedge", and what it cost.** v2 argued that major tunables should be
reachable through *three* interfaces (file, flag, command) so a failed command interaction could still be worked
around on the frozen binary. The professor's answer #1 removed the file path as a requirement, so **v3 carries two
paths for the two quantities the PPT measures** (`refreshMs` via `set_speed` **and** `--refresh-ms=`; `pollingMs`
via `--poll-ms=`) and **one path** for everything else. `text` is command-only: `set_text` is an explicitly
required command, and a pre-armed band is worth nothing at quiz time. This is a deliberate reduction of the hedge,
not an oversight — the trade is recorded in §8's risk table.

**"Varying inputs" means robustness is graded.** Empty input, whitespace, very long text, punctuation, mixed
case, both `set_speed` extremes, repeated start/stop, `set_text` while stopped, unknown commands, resize
mid-animation, `exit` while animating, a **very small terminal**, and **non-ASCII text** — which v3 **rejects by
contract** rather than normalizing (§3.7, D15). See §6.2.

### 1.3 Submission deliverables (hard constraints)

| Deliverable | Constraint from the handout | Owner |
| --- | --- | --- |
| **SOURCE** | source + **README.txt** with member name(s), run instructions, and **the entry file where `main` is**; or a GitHub link | W2 |
| **PPT** | technical report covering *command recognition*, *console UI implementation*, *command interpreter implementation*, *process representation*, *scheduler implementation*, and *the balance between a fluid refresh rate and a polling rate* — recommended values **for your current hardware** plus the limits where **screen tearing** and/or **noticeable typing delay** appear | §4.5 RACI |
| **VIDEO** | seamless/uncut; 480p min, 720p max; ≤ 1 GB; readable IDE font; **must always show pressing Run/Debug from the IDE** which initializes the program; no code access/modification once running; MP4 **embedded inside the PPTX** and reflected in the total size | W3 runs, W4 directs |

> "Entry class file" is Java-flavoured wording; the professor confirmed C++. `README.txt` answers it literally:
> entry file **`src/main.cpp`**, function `main`, class `csopesy::ConsoleApp` in `src/app/console_app.cpp`.

**The PPT's measurement requirement is not cut.** What is cut (D4) is only the *automated* machinery for
collecting it: no `--measure=FILE`, no CSV, no `scripts/measure.*`. §T1.4 becomes a written, repeatable **manual
sweep** — change the interval, watch the band and the echo, record the observation — and its output is what the
PPT quotes. The honest limitation is stated where the numbers are published: the sweep is *observed*, so the PPT
reports thresholds and recommended values, **not** per-keystroke latency figures.

**The sweep's frozen-binary workflow (D14).** Because measurement now has no in-process instrumentation, the
procedure must be reproducible *without* touching source — otherwise the sweep would violate the no-recompile
rule that the whole measurement exists to respect:

```text
For each take (one row of the table):
  1. In the IDE, set the run configuration's Program arguments to  --poll-ms=N --refresh-ms=M
     (`csopesy-quiz`, whose Executable is the frozen artifact and whose Before-launch has NO Build step).
  2. Press Run/Debug.  The frozen binary starts with those two values and nothing else changed.
  3. Observe: is the band fluid? does it tear or flicker? does typing echo lag?  Record the observation
     and the terminal/hardware it was made on.
  4. Change ONLY the arguments for the next take.
  5. Never edit source, never rebuild, never re-copy the executable.  The artifact's SHA-256 is unchanged
     across the whole sweep, which is itself the evidence that the numbers describe the frozen build.
```

Inside the running console, `set_speed <ms>` reaches `refreshMs` without even restarting the process, which is
the second path the partial-credit hedge retains (§1.2). `pollingMs` has no runtime command, by design: the
professor's six commands are fixed, and inventing a seventh to move a measurement knob would put a non-required
command into the graded interface.

### 1.4 The handout's own mock console (transcribed from a 600-dpi render)

```text
Welcome to CSOPESY!          <- capital W

Group developer:
De La Cruz, Juan
Santos, Alex

Version date:                <- blank in the mock

Command>|                    <- cursor on the prompt row
```

and the marquee state, whose band sits **above** the `Group developer:` block:

```text
/ ___/ ___/  _ \/  _ \/ ___/ ___/ \ / /
| |   \__ \ | | | | | |\__ \\__ \  | | |
| |__ ___/ | |_| | |_| |___/ ___/ | | |
\____/____/ \____/\____/(___/(___/  |_|_|
```

Two consequences, both now decided rather than hedged:

1. **The mock's art is a slanted outline font.** The handout offers *"Text marquee **or** ASCII graphics
   marquee"*, and the professor confirmed plain text is sufficient → **v3 renders one row of horizontally
   scrolling text** and there is no glyph table at all. The consequence to respect: **one byte is one column**,
   which is why §3.7 **rejects** non-printable and non-ASCII bytes at `set_text` time instead of slicing
   mid-character — the input contract, not a normalization step (D15).
2. **The two mocks disagree about where the band sits** (empty in one, art there in the other). v2 kept
   `marquee_row` as "cheap insurance"; the professor's answer #5 retired that insurance. The band row is now
   the renderer's constant `kBandRow = 3` — *near the top, clearly visible* — and nothing configures it.

---

## 2. Your four machines — and the one that is graded

### 2.1 What the OS spread actually changes now

| Machine | Role in v3 | Why |
| --- | --- | --- |
| **Windows (W3 Nathan; 3 of 4 members)** | **The graded platform.** The quiz runs on the professor's Windows machine (answer #4). `terminal_win32.cpp`, the graded run config, the rehearsal kit and the recorded takes all live here. | No other platform's behaviour can substitute for the one the professor runs. |
| **Linux (W1 Lorens; CI `ubuntu-latest`)** | Primary development platform for POSIX; the POSIX backend is verified here first. | `termios`/`poll` failures are observable only on a real tty and can be iterated without a VM. |
| **Windows + macOS (W2 Byron; W4 Kim)** | Byron is the only member who can compile **and run** both a POSIX and a Win32 build — so Byron is the only one who can prove the `Terminal` abstraction abstracts anything. | Capability, not seniority. |
| **macOS (W4 Kim; CI `macos-latest`)** | **Non-blocking second opinion** on the POSIX path. No macOS-specific deliverable is required; the CI job already exists and costs nothing. | macOS is not in the handout's shell reference. |
| **CLion (all four)** | The shared IDE, because the video must show Run/Debug **from the IDE**. | Handout §VIDEO. |

### 2.2 What stays, and why the 3-OS CI matrix is not the over-engineering it looks like

`gpt-v9`/`gpt-v10` recommend deleting the matrix and the macOS job. Kept (D9), because the matrix is the **only**
mechanism that compiles `terminal_win32.cpp` for the two members who own no Windows toolchain, and the only thing
that catches a `#ifdef` mistake before the graded machine sees it. It is one YAML block and three jobs that
already pass (T0.3, runs `35118223659` / `35118445611` / `35119216051` / `35120070872`). Deleting it would convert
a compile error on the professor's machine into a zero-points outcome.

### 2.3 Run configurations in v3 (the `--config` removal lands here)

| Config | Type | Before (v2.6) | In v3 | Why |
| --- | --- | --- | --- | --- |
| `csopesy-dev` | `CMakeRunConfiguration` | `PROGRAM_PARAMS="--config=config/csopesy.ini"`, Build **on** | **`PROGRAM_PARAMS=""`**, Build **on** | The config path no longer exists; dev keeps its build (§5.1 of the handoff — that drafting slip is not reopened). |
| `csopesy-quiz` | `CustomBuildApplication`, `EXECUTABLE=$PROJECT_DIR$/frozen/csopesy.exe`, **no** Build step | `PROGRAM_PARAMS="--config=config/csopesy.ini"` | **`PROGRAM_PARAMS=""`** | The no-recompile guarantee becomes *stronger*: the graded press now passes nothing that could name a file. The case is set entirely by the commands typed at the prompt. |

`EXECUTABLE` stays `frozen/csopesy.exe` because the **graded** machine is Windows (answer #4). The POSIX
equivalent remains the `--no-tty` path plus a `./frozen/csopesy` invocation documented in the README, exactly as
v2 had it.

**Which flags exist, and which of them the quiz uses (D13).** Two of the three flags are a *development* surface
and only one pair is a *quiz* surface. Conflating them was v2's mistake and would put a testing mechanism in
front of the professor:

| Flag | Dev / CI | Quiz (graded run) | Function |
| --- | --- | --- | --- |
| `--no-tty` | **yes** — it is what makes the real-binary smoke test portable across the three CI images | **no.** The graded run is a real console, and v2 §T2.5's acceptance cases are about *animated* behavior, which plain line mode does not produce | forces plain line mode: no raw mode, no ANSI, no frames, no worker thread |
| `--refresh-ms=N` | yes | **yes** — during a measurement take (D14), and as the partial-credit workaround for `set_speed` | `Parameters::setRefresh(N)`, clamped and reported |
| `--poll-ms=N` | yes | **yes** — during a measurement take only (D14); it has no runtime command | `Parameters::setPolling(N)`, clamped and reported |

So the **default** graded press passes no arguments and is driven entirely by the six commands; a *measurement*
press adds nothing but the two numeric flags. `--no-tty` never appears in the graded configuration.

---

## 3.0 What v3 removes, keeps and reshapes

This is the whole v2.6 → v3 delta in one place. **Every removal below is a contract change** unless the table
says otherwise, and therefore goes through the §4.5 protocol and a `CONTRACTS.md` marker bump (D9 keeps that
protocol; removing a header does not exempt the change from it).

### Table A — removed

| Removed | Files / symbols | Authority | Contract change? |
| --- | --- | --- | --- |
| **The `.ini` configuration system** | `config/csopesy.ini` (delete) · `include/csopesy/config_io.hpp` (delete) · `src/entities/config_io.cpp` (delete) · `tests/unit/test_config.cpp` (delete) · `--config=PATH` · the `config/quiz_case_<n>.ini` convention and v2 §7's copy-over step | Professor answer #1 (D1) | **Yes** — one header deleted, `Parameters` loses its file layer |
| **The 5×5 ASCII-art glyph engine** | `include/csopesy/glyphs.hpp` (delete) · `src/features/marquee/glyphs.cpp` (delete) · `tests/unit/test_glyphs.cpp` (delete) · `Parameters::asciiArt` · `--plain` / `--art` · `kCellCols`/`kCellRows`/`kArtRows` · `renderBlockText` · `Renderer::artWidthFor` | Handout p.2 + professor answer #2 (D2) | **Yes** — one header deleted, `Parameters` and `Renderer` shrink |
| **`marquee_row`** | `Parameters::marqueeRow`; replaced by the renderer constant `kBandRow = 3` | Professor answer #5 (D3) | **Yes** |
| **The measurement telemetry** | `Parameters::measurePath` · `--measure=FILE` · `Scheduler::appendMeasure`, `measure_`, `pendingEventMs_`, `eventOwed_`, `noteEventLocked`'s stamping · `scripts/measure.*` · the CSV under `docs/measurements/` | D4 (operator carve-out: the **PPT requirement stays**, as a manual sweep — §T1.4) | **Yes** — `Scheduler`'s private state and `Parameters` shrink |
| **`--diag`** | the flag and its one-shot report; `main`'s short-circuit | D5 | **No contract change** (it was a `main`-level feature), but §8 risk 3's mitigation changes to a hard startup error and the runbook loses its non-visual pre-flight step. Recorded so it does not look like an oversight. |
| **`FrameBuffer` row-diffing** | `include/csopesy/frame_buffer.hpp` (delete) · `src/shared/terminal/frame_buffer.cpp` (delete) · the diff assertions in `tests/unit/test_renderer.cpp` · `Scheduler::fb_`, `fbRows_`, `fbCols_` | D6 | **Yes** — one header deleted; `Renderer::drawFrame(FrameBuffer&, …)` becomes `Renderer::buildFrame(…)->std::string` |
| **The injected `Clock`** | `scheduler.hpp`'s `using Clock = std::function<long long()>`, the `now_` member and the constructor parameter | D7 (narrowed: **one** clock source, `Terminal::nowMs()`) | **Yes** |

### Table B — kept, deliberately (do not "simplify" these)

| Kept | Why it survives v3 |
| --- | --- |
| **Two threads, one `std::mutex`, one `std::condition_variable`, one terminal writer** | The professor **instructed** it (answer #3). One mutex and one cv is the floor that keeps risk 21 (lock inversion) structurally impossible. |
| **`Interpreter::quit_` owned by the input thread only; shutdown crosses as `stop_`** | v2.6's fix for a real cross-thread read; restoring `TickResult::quit` re-introduces it (risk 28). |
| **FSD layers + `scripts/check_layers.sh`** | `AGENTS.md` §3 mandates it; T0.1's evidence exists; the guard is ~100 lines and fails loudly on an unmapped header. |
| **`CONTRACTS.md` freeze + the `git hash-object` table + the §4.5 protocol** | T0.4's evidence (it caught a mutation test); four members build on machines the others cannot fix. |
| **3-OS CI matrix + macOS job** | §2.2 above. |
| **Zero-dependency harness (`tests/support/check.hpp`)** | `AGENTS.md` §6: no framework, no network, all three CI images. |
| **`--no-tty` plain line mode** | It is what makes the real-binary smoke test portable across the three CI OSes (adjudication R4). |
| **`SchedulerSnapshot`, `wakeTick_`, the concurrency tests** | Adjudicated and kept (§10.3, §11 of the adjudication). `--diag`'s removal does **not** remove `snapshot()`'s other reason: it is the only race-free read of live scheduler state for the threaded tests. |
| **`MarqueeProcess` as a small PCB** (`pid`, `name`, `state`, `cycles`, `lastRenderMs`, `hasRendered`) | 17 lines; *process representation* is a graded PPT topic. |
| **`Terminal::nowMs()` as the only clock source** | Now the *single* clock source (v2 had it **and** an injected `Clock` — a duplicate that v9/v10 half-noticed). Tests drive time through `FakeTerminal::nowMs()`, so no `sleep_for` is needed and `AGENTS.md` §5 stays true. |
| **`--refresh-ms=` / `--poll-ms=`** | Two flags, and the only way to sweep the two quantities the PPT must quantify without a rebuild (D10). |

### Table C — reshaped (same task id, different work)

| Area | v2.6 | v3 |
| --- | --- | --- |
| **T4.1–T4.3 (W4)** | glyph table (~45 cells), scroll math, `FrameBuffer` diffing | **T4.1** scroll math + `sliceRow` width invariant · **T4.2** the frame layout (band + chrome) · **T4.3** frame assembly + resize-by-rebuild. No glyph table, no diffing. |
| **T1.3 (W1)** | two threads + `FrameBuffer` invalidation + `--measure` plumbing + injected `Clock` | two threads, `buildFrame` written by one thread with **one `write()`**, `Terminal::nowMs()` deadlines. Fewer moving parts, same contract. |
| **T1.4 (W4, was W1+W4)** | `--measure` CSV → derived table | the **manual sweep** procedure + `docs/measurements/<os>-{refresh,polling}.md` (D4) |
| **T0.5 (Phase 0)** | scriptable half done; live gate pending — and unobservable before T2.5 | scriptable half stays done; the **live CLion gate moves to Phase 2** (T5.1) where features exist to observe (gpt-v12, accepted) |
| **A9** | the same effect through config **and** flag **and** command | the same effect through a flag **and** the command, on the frozen binary (two paths, not three) |
| **§3.9 Invariants** | `sliceRow` length; `scrollOffset` wrap; *"a frame contains every band row exactly once"*; *"header rows are repainted only when their text changes"* | `sliceRow` length, `scrollOffset` wrap, and the new full-rebuild properties: every frame carries every chrome row, and the frame's width equals the terminal's width (no stale columns). |
| **Parameters** | 10 fields | **7 fields**: `text`, `refreshMs`, `pollingMs`, `developers`, `versionDate`, `noTty` (+ the clamp constants and methods) |
| **Header set** | 13 headers, `CONTRACTS.md` **v2.6** | **11 headers** (+`cli.hpp`, −`config_io.hpp`, −`glyphs.hpp`, −`frame_buffer.hpp`); marker becomes **contracts v3.0**, with a regenerated hash table in the same commit |
| **Risk set** | 28 risks | risks 8, 19, 20, 27 are **deleted or transformed** (no glyphs; resize is handled by rebuilding; no telemetry to misread) and new risk 29 (**non-ASCII input**) is added as a *defined rejection* rather than undefined behavior — see §8 |

### Table D — the proposed contract deltas (text for S2 to land)

Shown so S2 is mechanical. **Not yet frozen** — S2 applies these, updates the checker map and the marker together.

```cpp
// NEW  include/csopesy/cli.hpp        layer: entities   (takes over the slot config_io.hpp vacates)
#pragma once
#include <string>
#include <vector>
#include "csopesy/parameters.hpp"
namespace csopesy {
struct CliResult {
  Parameters params;                  // defaults + whatever the flags provided; NOTHING is layered on top of it
  std::vector<std::string> warnings;  // one line per unknown flag / malformed value — never fatal (§3.6)
};
// args EXCLUDING argv[0]. Recognized: --no-tty, --refresh-ms=N, --poll-ms=N (§3.6).
CliResult parseCli(const std::vector<std::string>& args);
}
```

```cpp
// CHANGED  include/csopesy/parameters.hpp
//   - bool asciiArt            (removed — D2)
//   - int  marqueeRow          (removed — D3)
//   - std::string measurePath  (removed — D4)
//   + enum class TextResult    (D15: ASCII-only text contract)
struct Parameters {
  std::string text   = "CSOPESY";
  int  refreshMs     = 100;            // set_speed target. range [1, 10000]
  int  pollingMs     = 10;             // MAX idle wait when nothing is ready. [1, 1000]
  std::vector<std::string> developers = {"De La Cruz, Juan", "Santos, Alex"};
  std::string versionDate = "";         // blank is valid
  bool noTty = false;                   // plain line mode; DEV/CI ONLY (D13), never the graded run
  static constexpr int kRefreshMin = 1, kRefreshMax = 10000;
  static constexpr int kPollMin = 1,    kPollMax = 1000;
  ClampReport setRefresh(int ms);
  ClampReport setPolling(int ms);
  TextResult setText(const std::string& t);  // Ok | Empty | NonAscii — rejects, never normalizes (D15)
  static Parameters defaults();
};
```

```cpp
// CHANGED  include/csopesy/renderer.hpp        (no longer includes frame_buffer.hpp)
//   - drawFrame(FrameBuffer&, ...)  -> buildFrame(...) -> std::string   (D6)
//   - static int artWidthFor(...)   -> removed; the band's source width is text.size() (D2)
class Renderer {
 public:
  // ONE complete frame, ready for ONE write(): cursor-home, every chrome row, band, prompt row.
  std::string buildFrame(const Parameters&, const MarqueeProcess&, const std::string& prompt,
                         const std::string& buffer, const std::string& message,
                         int rows, int cols) const;
  static int bandWidthFor(int cols);            // cols - 2*kMargin, floored at 1
  static constexpr int kBandRow = 3;            // fixed, near the top (professor answer #5 / D3)
};
```

```cpp
// CHANGED  include/csopesy/scheduler.hpp
//   - Clock / now_ / the constructor's Clock parameter   (removed — D7; use term_.nowMs())
//   - FrameBuffer fb_ / fbRows_ / fbCols_                (removed — D6)
//   - measure_ / appendMeasure() / pendingEventMs_ / eventOwed_  (removed — D4)
// Everything else — postEvent/wake/requestStop/join/joinable/start/run/tick/pollTimeoutMs/snapshot,
// TickResult{bool rendered}, SchedulerSnapshot — is UNCHANGED from v2.6 and remains frozen.
```

**Layer-map consequence (`scripts/check_layers.sh`).** `AGENTS.md` §3: a contract header that is not in
`layer_of_header()` is a **hard failure**, so the map is edited in the same commit: drop `frame_buffer.hpp`
(shared), `config_io.hpp` (entities) and `glyphs.hpp` (features:marquee); add `cli.hpp` (entities). The guard's
own `check_layers: OK (N files scanned)` line is the evidence that the file count moved as expected.

---

## 3. Contracts — the v3 delta contract

### 3.0.1 How this section is organized (D16)

Every changed contract is written out **in full** below. Every unchanged frozen contract is incorporated
**by version-pinned reference to v2.6**, which is frozen and (per D11) will never be edited. Duplicating ~500
lines of prose that seven review rounds already adjudicated would create two copies to keep in sync — precisely
the drift the freeze exists to prevent. **If v2 is ever edited, this incorporation breaks**, so v2's superseded
banner carries the rule: *its §3 text is quoted normatively by v3; edit it and v3's references go stale.*

| Header | v3 status | Normative text |
| --- | --- | --- |
| `keys.hpp` | **unchanged** | v2 §3.3 |
| `terminal.hpp` | **unchanged** | v2 §3.3. One v3 note: `nowMs()` is now the **single** clock source — v2 also had an injected `Clock`, removed by D7 |
| `shutdown.hpp` | **unchanged** | v2 §3.10 (the five-step shutdown sequence) |
| `process.hpp` | **unchanged** | v2 §3.8 |
| `line_editor.hpp` | **unchanged** — still declares nothing, on purpose | v2 §3.11 + `CONTRACTS.md` §"Deliberately deferred" |
| `interpreter.hpp` | **unchanged** except one added message | v2 §3.11, delta in §3.11 below |
| `console_app.hpp` | **unchanged** | v2 §T2.5 |
| **`cli.hpp`** | **NEW** | §3.6 below (full) |
| `parameters.hpp` | **changed** | §3.5 below (full) |
| `renderer.hpp` | **changed** | §3.9 below (full) |
| `scheduler.hpp` | **changed** (constructor + private state) | §3.8 below (delta list) + v2 §3.8 verbatim for everything else |
| `config_io.hpp` | **DELETED** (D1) | — |
| `glyphs.hpp` | **DELETED** (D2) | — |
| `frame_buffer.hpp` | **DELETED** (D6) | — |

**Header set: 13 → 11.** `CONTRACTS.md`'s marker goes from **v2.6** to **v3.0** and its hash table is
regenerated in the same commit (§3.12).

### 3.1 Layer rules — unchanged, with a map delta

`AGENTS.md` §3 / v2 §3.1 are normative and unchanged: `app > features > entities > shared`, `platform/` a peer
of `shared/` importing only `shared/`, no cross-import between `features/marquee/` and `features/commands/`, test
doubles in `tests/support/` only.

The guard's `layer_of_header()` map changes in the same commit as the headers, because `AGENTS.md` §3 makes an
unmapped contract header a hard failure:

| Header | v2.6 mapping | v3 mapping |
| --- | --- | --- |
| `config_io.hpp` | `entities` | **removed** — `cli.hpp` takes the slot |
| `glyphs.hpp` | `features:marquee` | **removed** |
| `frame_buffer.hpp` | `shared` | **removed** |
| `cli.hpp` | — | **`entities`** |

`src/shared/` ends up with **no translation unit** (`frame_buffer.cpp` was its only one). The layer stays in the
guard, because `keys.hpp`, `terminal.hpp` and `shutdown.hpp` are `shared`-layer contracts and future platform
work may add a shared `.cpp`.

### 3.2 Repository layout (v3)

```text
src/main.cpp                            app/       argv → parseCli, TerminalGuard, ConsoleApp, exit code
src/app/console_app.cpp                 app/       run loop; the §3.10 sequence: observe → requestStop → join → restore
src/app/scheduler.cpp                   app/       the two threads, one mutex, one cv, the only terminal writer
src/entities/parameters.cpp             entities   clamping + the ASCII text acceptance rule (§3.5)
src/entities/cli.cpp                    entities   argv parsing — no file, no ini, no case files (§3.6)
src/entities/process.cpp                entities   the PCB
src/features/marquee/renderer.cpp       features   scroll math + the plain-text frame builder (§3.9)
src/features/commands/interpreter.cpp   features   the six commands and their exact messages (§3.7)
src/features/commands/line_editor.cpp   features   the editing rules (declares nothing — v2 §3.11)
src/platform/terminal_posix.cpp         platform   termios / poll / SIGWINCH
src/platform/terminal_win32.cpp         platform   console modes, VT output, QuickEdit
include/csopesy/*.hpp                   —          the 11 contract headers
scripts/check_layers.sh                 —          the layer guard (+ the §3.1 map delta)
tests/support/check.{hpp,cpp}           —          the zero-dependency harness
tests/unit/test_*.cpp                   —          unit tests
tests/smoke/…                           —          the real-binary `--no-tty` end-to-end test (T2.5)
```

**Deleted:** `config/csopesy.ini` and the now-empty `config/` · `src/entities/config_io.cpp` ·
`src/features/marquee/glyphs.cpp` · `src/shared/terminal/frame_buffer.cpp` · `tests/unit/test_config.cpp` ·
`tests/unit/test_glyphs.cpp`.

### 3.3 `Terminal` (unchanged) and the single clock source

`include/csopesy/terminal.hpp` is **verbatim v2 §3.3** — `enterRawMode`, `restore`, `size`, `readEvent(ev, timeoutMs)`,
`write`, `flush`, `isTty`, `nowMs`, and `Terminal::create()` defined once per platform. Nothing about it changes
with the plain-text marquee: it never knew about glyphs.

**v3 note (D7).** `nowMs()` is the **only** clock source. v2 injected a second one
(`Scheduler(..., Clock)`) and the adjudication had already flagged the duplication; v3 removes the functor and
calls `term_.nowMs()`. Consequences: (a) `AGENTS.md` §5's invariant stays true — no `std::chrono` call enters
scheduler logic; (b) the threaded tests stay deterministic **without sleeps** because `FakeTerminal::nowMs()`
is what they drive; (c) `Terminal::nowMs()` must therefore be monotonic and cheap — it is `clock_gettime` /
`GetTickCount64`, never a syscall that can block.

### 3.4 Platform selection (unchanged)

v2 §3.4 is normative: exactly one of `terminal_posix.cpp` / `terminal_win32.cpp` is compiled, chosen by
`CMakeLists.txt`'s per-platform source list, and `Terminal::create()` has exactly one definition in the linked
binary. Unchanged in v3, and still verified by the 3-OS CI matrix (D9).

### 3.5 `Parameters` (changed — full text)

```cpp
// include/csopesy/parameters.hpp
#pragma once
#include <string>
#include <vector>
namespace csopesy {
struct ClampReport { bool clamped = false; int requested = 0; int applied = 0; const char* field = ""; };

// D15: the text contract is ASCII. setText reports WHY it refused instead of mangling the argument.
enum class TextResult { Ok, Empty, NonAscii };

struct Parameters {
  // marquee
  std::string text   = "CSOPESY";   // INVARIANT: every byte is in [0x20, 0x7E]. Mutated only via setText().
  int  refreshMs     = 100;          // set_speed target. range [1, 10000]
  int  pollingMs     = 10;           // MAX idle wait when nothing is ready. [1, 1000]; does NOT delay keys
  // chrome  (matches the handout's mock)
  std::vector<std::string> developers = {"De La Cruz, Juan", "Santos, Alex"};
  std::string versionDate = "";       // blank in the mock; blank is valid
  // infra
  bool noTty = false;                 // plain line mode. DEV/CI ONLY (D13) — never in the graded run

  static constexpr int kRefreshMin = 1, kRefreshMax = 10000;
  static constexpr int kPollMin = 1,    kPollMax = 1000;

  ClampReport setRefresh(int ms);      // clamps + reports; never throws
  ClampReport setPolling(int ms);
  TextResult  setText(const std::string& t);   // trims; Ok | Empty | NonAscii; state unchanged unless Ok
  static Parameters defaults();
};
}
```

**Removed from v2.6:** `asciiArt` (D2), `marqueeRow` (D3), `measurePath` (D4). Ten fields become seven.

**Precedence is now three layers, not four:** runtime command > CLI flag > built-in default. There is no file
layer (D1). Each layer assigns only what it explicitly provides, exactly as v2 §3.5 defined.

**ASCII invariant (D15), and why it is a correctness contract rather than validation polish.** After `setText`
accepts a value, `text` contains only bytes in `[0x20, 0x7E]`. `Renderer::sliceRow` and `scrollOffset` are byte-
and column-oriented (`bandWidthFor(cols)` is a column count), so *one byte must equal one column*. A UTF-8
argument would let a slice split a multi-byte character, producing mojibake **and** a band whose width no longer
matches its content. v3's answer is the input contract, not a mangling step: `setText` **rejects** out-of-range
bytes and the marquee text is left untouched, so the text the operator asked for is never silently altered.

`setText` steps, in order: (1) trim leading/trailing spaces; (2) internal space runs are preserved verbatim;
(3) empty ⇒ `TextResult::Empty`, nothing changes; (4) any byte outside `[0x20, 0x7E]` ⇒ `TextResult::NonAscii`,
nothing changes; (5) otherwise assign and return `TextResult::Ok`.

**Shared-state rule (v2.4/§3.5, unchanged).** `Parameters` is read by the marquee thread while the input thread
mutates it through the interpreter, so **every access originating in `tick()` happens under `Scheduler::mu_`**
(§3.8). The input thread reads its own `pollingMs` for the `readEvent` timeout without the lock, which is safe
because that thread is also the only writer — the rule is *one writer, all cross-thread readers under the
mutex*. `noTty` is the exception that proves the rule: `main` writes it once **before any thread exists** (it
must be known before raw mode is attempted), and the marquee thread then only reads it.

### 3.6 CLI (changed — full text; the only remaining parameter layer besides the commands)

```text
usage: csopesy [--no-tty] [--refresh-ms=N] [--poll-ms=N]
```

```cpp
// include/csopesy/cli.hpp   — layer: entities (takes the slot config_io.hpp vacates)
#pragma once
#include <string>
#include <vector>
#include "csopesy/parameters.hpp"
namespace csopesy {
struct CliResult {
  Parameters params;                  // defaults + whatever the flags provided
  std::vector<std::string> warnings;  // one line per unknown flag / malformed value / clamp; NEVER fatal
};
// args EXCLUDING argv[0]. Recognized: --no-tty, --refresh-ms=N, --poll-ms=N.
CliResult parseCli(const std::vector<std::string>& args);
}
```

| Flag | Effect | Failure handling |
| --- | --- | --- |
| `--no-tty` | `params.noTty = true` — plain line mode: no raw mode, no ANSI, no frames, **no worker thread** | `--no-tty=1` ⇒ unknown-option warning; the flag takes no value |
| `--refresh-ms=N` | `params.setRefresh(N)` → clamped to `[1, 10000]` and reported | malformed/absent `N` ⇒ warning, field keeps its default; out-of-range ⇒ **clamped with a report** (never a usage error) |
| `--poll-ms=N` | `params.setPolling(N)` → clamped to `[1, 1000]` and reported | same as above |

**Rules (all of them testable, all of them without a file):**

1. `--flag=value` is the **only** accepted form. `--refresh-ms 50` is two tokens; the first is a recognized flag
   with no value, the second is unknown — both warn, nothing breaks. Stated because it is exactly the kind of
   thing a test should pin rather than a reader should guess.
2. An **unknown** token warns: `Unknown option ignored: "--foo".` — and parsing continues with everything else.
3. A repeated flag: **last one wins**, no warning (it is not bad input).
4. Out-of-range values are *clamped and reported*, matching `set_speed`'s visible clamp: e.g.
   `--refresh-ms=99999 clamped to 10000 ms.` — a well-formed out-of-range number is never a usage error.
5. **A typo must never cost the quiz** (v2 §3.6's rule, kept): missing file, unknown flag, malformed number and
   absent value all degrade to defaults plus a warning line. Nothing here can abort startup.
6. Parsing happens in `main` **before** `Terminal::create()` and before any thread exists, because `noTty` must
   be known before raw mode is attempted.
7. **No file is read.** There is no `--config`; an old `--config=config/csopesy.ini` therefore produces rule 2's
   warning and runs on defaults, which is the harmless-and-loud behavior we want during the transition.

### 3.7 Command contract (unchanged strings + the ASCII rule)

v2 §3.7's table is normative **verbatim** — the same six commands, the same messages — with **one added row**:

| Input | Response |
| --- | --- |
| `set_text ` *[non-ASCII text]* | `set_text: only printable ASCII characters are supported.` — marquee text unchanged |

The `help` block stays the same six lines; `Marquee started.` / `Marquee is already running.` /
`Marquee stopped.` / `Marquee is not running.` / `Marquee text set to "…".` / `Marquee speed set to N ms.` /
`Speed N ms is out of range [1, 10000]; clamped to 1 ms.` / `Usage: <cmd> …` / `Command not recognized: "…".`
/ `Exiting CSOPESY. Goodbye!` are unchanged, and the test rule is unchanged (assert the **token** for state
messages, the **whole line** for every `Usage:`, the `help` header and the goodbye line).

**Recognition rules:** v2 §3.7 rules 1–6 are normative and unchanged (case-sensitive matching; trim the
argument's outer whitespace; preserve internal runs; `set_speed`'s argument must match `[-+]?[0-9]+` in full, so
`5abc` is a usage error and never a partial `std::stoi` parse; a well-formed out-of-range value **including
`-5`** is clamped and reported). **One rule is added:**

> **7 (D15).** After trimming, every byte of a `set_text` argument must be printable ASCII `[0x20, 0x7E]`.
> Otherwise the command is rejected with the message above and `text` is unchanged.

**Why the rule is needed at all, given the line editor.** The interactive line editor appends **printable ASCII
only** (Tab, control bytes and non-ASCII keys are ignored on entry, like the arrows), so typing cannot produce
an invalid argument. But `executeLine` is also reached by (a) the unit tests, which call it directly, and
(b) `--no-tty` plain line mode, which reads **raw stdin lines** that may carry arbitrary bytes. Rule 7 is what
keeps the §3.5 ASCII invariant true on those two paths.

### 3.8 Process + scheduler (delta list; everything else is v2 §3.8 verbatim)

`process.hpp` is unchanged. `scheduler.hpp`'s **public** API is unchanged except for the constructor, and v2
§3.8's threading contract — two threads, one mutex, one condition variable, one owner per resource, the three
lock rules, `tick()` never called holding `mu_`, no blocking I/O inside `mu_`, `Terminal::size()` read before the
lock, `TickResult{ bool rendered }`, no `detach()` anywhere — is normative **verbatim**.

| v2.6 element | v3 | Why |
| --- | --- | --- |
| `using Clock = std::function<long long()>`; `now_`; the constructor's `Clock` parameter | **removed** — `term_.nowMs()` | D7; one clock source, no `std::chrono` in scheduler logic |
| `FrameBuffer fb_`, `fbRows_`, `fbCols_` | **removed** — the worker builds a `std::string` and issues **one** `write()` + one `flush()` | D6; `drawFrame(FrameBuffer&,…)` became `buildFrame(…)→std::string` (§3.9) |
| `std::ofstream measure_`, `appendMeasure()`, `pendingEventMs_`, `eventOwed_`, `noteEventLocked`'s stamping | **removed** | D4; the measurement is a manual sweep (§1.2, D14) |
| `postEvent(const KeyEvent&)` | now `{ lock; dirty_ = true; } + cv_.notify_all()` — no stamp to record | nothing consumes a stamp any more |
| `snapshot()` | same shape; its `now` field is `term_.nowMs()` | mechanical |

**Structurally unchanged and still frozen:** `mu_`, `cv_`, `stop_`, `dirty_`, `wakeTick_`, `worker_`,
`postEvent`/`wake`/`requestStop`/`join`/`joinable`/`start`/`run`, the test-only `tick(const KeyEvent*)` overload,
`pollTimeoutMs()` (never 0, never above `pollingMs`), and the render rule
`due = state == Running && (!hasRendered || now - lastRenderMs >= refreshMs)`, with a redraw also emitted for
every consumed event (that is what keeps echo latency independent of `refreshMs`) and `cycles` incrementing once
per ***rendered*** frame, not per redraw. The ownership table's job for each field is unchanged; `wakeTick_` and
`snapshot()` stay for the test suite (§3.0 Table B).

**Resize needs no state any more.** v2 had to call `FrameBuffer::resize` *and* set a repaint flag, and the
adjudication filed a real defect where invalidation was never submitted. In v3 every frame is built whole from
`Terminal::size()` read in `tick()`, so the next frame is simply laid out for the new size — the defect class is
structurally absent (risk 19 deleted, §8).

### 3.9 Renderer and layout (changed — full text; the plain-text marquee)

```cpp
// include/csopesy/renderer.hpp      (no longer includes frame_buffer.hpp)
#pragma once
#include <string>
#include <string_view>
#include "csopesy/parameters.hpp"
#include "csopesy/process.hpp"
namespace csopesy {
// Pure scroll math. `offset` is the band's left edge expressed in *text* coordinates shifted by one band
// width: windowLeft = offset - bandWidth, so offset==0 draws an all-blank band (the text has just left) and
// offset==bandWidth puts text[0] in the band's first column. Wrap period = textWidth + bandWidth, i.e. the
// text scrolls fully off before it re-enters.
int scrollOffset(long long cycles, int textWidth, int bandWidth);   // returns [0, textWidth + bandWidth)
std::string sliceRow(std::string_view text, int bandWidth, int offset);  // ALWAYS exactly bandWidth bytes

class Renderer {
 public:
  // ONE complete frame, ready for ONE write(): cursor-home, every chrome row padded to `cols`, then the
  // prompt row last. `rows`/`cols` come from Terminal::size() as read in the caller (never under mu_).
  std::string buildFrame(const Parameters&, const MarqueeProcess&, const std::string& prompt,
                         const std::string& buffer, const std::string& message,
                         int rows, int cols) const;
  static int bandWidthFor(int cols);   // max(1, cols - 2): one space of margin each side
  static constexpr int kBandRow = 3;   // FIXED, near the top (professor answer #5 / D3). Nothing configures it.
};
}
```

**Layout** (1-based rows; the band occupies columns 2..`cols-1`, i.e. `bandWidthFor(cols)` columns):

```text
row 1        Welcome to CSOPESY!
row 2        (blank)
row 3        band: ONE row of sliceRow(params.text, bandWidthFor(cols), scrollOffset(...))   <- kBandRow
row 4        (blank)
row 5        Group developer:
row 6..n     one developer name per row
             (blank)
             Version date: <versionDate>          <- no trailing space when blank
             (blank)
last row     Command> <visibleSlice(buffer, cols - len("Command> ") - 1)>    <- ONE row, scrolls horizontally
```

**Tight terminals — a defined priority, not an accident.** When `rows` cannot hold the whole chrome, rows are
dropped in this order: `Version date:`, the developer names, `Group developer:`, the welcome line. **The band
row and the prompt row are never dropped**: a missing prompt makes a working program look wedged, and a missing
band fails the assignment's only visual requirement. When `cols` is tiny, `bandWidthFor` floors at 1 and every
row is clipped to `cols` so no line ever wraps (a wrapped row would push the prompt off its expected row).

**Invariants asserted in tests** (T4.1–T4.3, S3): `sliceRow`'s result length is **always exactly** `bandWidth`;
`scrollOffset` is always in `[0, textWidth + bandWidth)` and a blank band occurs once per period at `offset == 0`;
no frame line exceeds `cols` visible columns; `buildFrame`'s output is a **single** string containing the band
row and the prompt row for every non-degenerate `rows`/`cols`; and the frame is rebuilt from scratch each time,
so a size change needs no invalidation flag.

**Plain-line mode.** When `params.noTty || !term.isTty()`, `buildFrame` is **not called at all** and no worker
thread is started: responses are printed as ordinary lines (`--no-tty` is what makes the real-binary smoke test
portable across the three CI images). The renderer therefore never has to emit ANSI-free frames.

**Tearing claim — the v2.4 wording, kept and still not overstated.** Emitting one assembled frame per `write()`
prevents *application-level* interleaving of two frames; the threaded design strengthens exactly that (the frame
is assembled and written by one owning thread, so two frames cannot even be built concurrently). It does **not**
made terminal repaint atomic, no escape sequence is added to chase that, and "we use threads" is not offered
anywhere as a tearing fix. The PPT uses this wording.

### 3.10 Shutdown (unchanged)

v2 §3.10's five-step sequence is normative verbatim: intent (`quit_` / the `sig_atomic_t` flag) → observe (the
**input thread**, the only reader of both) → `requestStop()` → `join()` → restore. `shutdown.hpp` is unchanged.
Nothing in v3 touches it: the worker still never reads `quit_`, `stop_` is still the worker's only exit signal,
and the goodbye line is still printed by the input thread once it is the only thread again.

### 3.11 Interpreter / line editor (unchanged, one added outcome)

v2 §3.11 is normative verbatim: `visibleSlice` and its "show the TAIL so the cursor stays visible" rule, the
one-fixed-prompt-row decision, `Interpreter(Parameters&, MarqueeProcess&)`, `feed` / `executeLine` /
`quitRequested` / `prompt` / `buffer` / `lastMessage`, the rule that **`Interpreter` itself takes no lock**
(the caller owns it), and the `quit_` single-threaded-ownership note.

**Deltas.** (1) `set_text` now has three outcomes to render — `Ok` (`Marquee text set to "…".`), `Empty`
(`Usage: set_text <text>`), `NonAscii` (§3.7's new line) — so the interpreter branches on
`Parameters::setText`'s `TextResult` instead of a `bool`. (2) For the same reason, the interactive line editor
appends **printable ASCII only** (§3.7 rule 7). Nothing else changes.

### 3.12 What the v3.0 freeze commit must contain (S2, bytes)

One commit, in this order, with the §4.5 announcement recorded in `PLAN_V3_PROGRESS.md` §4:

1. `include/csopesy/cli.hpp` added; `parameters.hpp`, `renderer.hpp`, `scheduler.hpp` rewritten per §3.5/§3.9/§3.8;
   `config_io.hpp`, `glyphs.hpp`, `frame_buffer.hpp` deleted.
2. `scripts/check_layers.sh`'s `layer_of_header()` updated per §3.1 — **the guard fails on an unmapped header, so
   this cannot be forgotten silently**, but a wrong mapping could stop guarding, so the map is reviewed by hand.
3. `CONTRACTS.md`: marker → **contracts v3.0**, header table and `git hash-object` table regenerated **in the
   same commit**, and the "Re-frozen in v2.6" section replaced by a v3.0 one that names the deletions and the
   removals (`asciiArt`, `marqueeRow`, `measurePath`, `Clock`).
4. `.idea/runConfigurations/*.xml`: `PROGRAM_PARAMS` cleared in both configs (§2.3); `config/csopesy.ini` deleted.
5. Source/test deletions per §3.2 — deleting a stub is not a behaviour change, but the `ctest` target list must
   stop naming the deleted test files or the build breaks on all three OSes.

**Evidence required before this is called done** (`AGENTS.md` §8): `bash scripts/check_layers.sh` prints `OK (N
files scanned)` with the expected new count; `cmake --preset debug && cmake --build --preset debug && ctest
--preset debug` passes; every hash in the new `CONTRACTS.md` table matches `git hash-object include/csopesy/*.hpp`;
and CI is green on all three OSes.

---

## 4. Workload split — four workstreams

### 4.0 Why the split is drawn along OS capability (unchanged)

v2 §4.0 is normative and unchanged: the risk is not "who writes the most code" but the ~14 `#ifdef`/API
subtleties that only surface by *running* on that platform (raw-mode flags, `\r\n` under raw mode, `Ctrl+C`
no longer producing `SIGINT`, `_getch` arrow prefixes, explicitly enabling VT, QuickEdit freezing the process,
`TIOCGWINSZ`, resize events). Each workstream therefore gets **one platform primary plus one pure-logic or
documentation deliverable**, so nobody idles behind an interface they cannot test.

The v3 cuts change the *content* of two workstreams — W2's config parser became a three-flag CLI, and W4's
glyph table and `FrameBuffer` are gone — but not the principle, and not the membership.

### 4.1 W1 — Lorens (Linux-only)

**Primary:** POSIX console backend `src/platform/terminal_posix.cpp`.
**Supporting:** `src/entities/process.cpp`, `src/app/scheduler.cpp` (the two-thread model).

| | |
| --- | --- |
| **Why this person** | Linux-only ⇒ every hour must land on Linux-verifiable work. Instrument-level failures (raw mode not restored, `\r\n`, `Ctrl+C`, `EINTR`) are observable only on a real tty, and can be iterated without a VM. macOS shares the code path, so W4's Mac is a second opinion, never a blocker. |
| **Why the scheduler goes here** | The scheduler *is* defined by the backend's timing primitive: one marquee tick is a bounded timed wait behind a condition variable, and the render deadline is capped by the same `pollingMs` the backend's `readEvent` timeout uses. The owner of the timeout semantics owns the deadline logic. |
| **Deliverables** | `terminal_posix.cpp`; `process.cpp`; `scheduler.cpp` (two threads, one mutex, one cv, one writer, `term_.nowMs()` deadlines); `docs/threading-model.md`; `docs/measurements/linux-{refresh,polling}.md`; PPT *Process representation* + *Scheduler implementation* |
| **Budget** | v2 ~10 h → **v3 ~8 h**. Δ: no `--measure` plumbing or keystroke stamping, no injected `Clock`, no `FrameBuffer` resize/invalidation path |
| **Owns as a standing duty** | `AGENTS.md` §5's threading invariants — one mutex, one cv, `tick()` never called holding `mu_`, no syscall inside the critical section, `Terminal::size()` read before the lock |

### 4.2 W2 — Byron (Linux + Windows)

**Primary (infrastructure):** the contract set, CMake/presets, the CI matrix, `CONTRACTS.md`, integration.
**Secondary (logic):** `parameters.cpp`, `cli.cpp`, `interpreter.cpp`, `line_editor.cpp`, `main.cpp`, `console_app.cpp`.

| | |
| --- | --- |
| **Why this person** | The **only** member who can compile *and* run both a POSIX and a Win32 build, so the only one who can prove the `Terminal` abstraction abstracts anything — the classic failure is a contract that quietly assumes one platform. Integrator and build owner by capability. Also the **freeze owner (§4.5)**, which is why the v3.0 change (T0.6) is his to ratify and land. |
| **Why parameterization goes here** | The owner of the build owns the launch path, so the owner of the build should own the flags the launch path passes. `Interpreter` and `Parameters` are one owner because `set_text`/`set_speed` mutate the struct and `help` prints the table; splitting them creates a two-person edit on one struct. |
| **Why the line editor is bundled** | Raw mode disables echo, so the app echoes keystrokes itself — that echo path *is* the interpreter's input path, and in v3 it is also where the §3.7 ASCII rule is enforced at entry. |
| **Deliverables** | `CMakeLists.txt`, `CMakePresets.json`, `.github/workflows/ci.yml`, `CONTRACTS.md`, `scripts/check_layers.sh` (map), `parameters.cpp`, `cli.cpp`, `interpreter.cpp`, `line_editor.cpp`, `main.cpp`, `console_app.cpp`, `README.txt`; **T0.6**; PPT *Command recognition* + *Command interpreter implementation* |
| **Budget** | v2 ~11 h → **v3 ~9 h**. Δ: the config parser (2.5 h) becomes a ~0.5 h three-flag CLI; no `ascii_art`/`--plain` plumbing; + T0.6 (~1 h) |

### 4.3 W3 — Nathan (Windows-only)

**Primary:** Win32 console backend `src/platform/terminal_win32.cpp`.
**Supporting:** the Windows rehearsal kit **and the graded-run verification** (T3.4).

| | |
| --- | --- |
| **Why this person** | Win32 console APIs are his only executable surface, and **he runs the graded quiz on Windows** (professor answer #4) — so the runbook owner must own the backend under it. Backend + runbook + rehearsal in one head removes the worst quiz-day failure: a runbook step that does not match the binary's behaviour on that machine. |
| **Specifically on his plate** | `_kbhit()`/`_getch()` polling with `0`/`224` arrow prefixes; `WaitForSingleObject` on the console input handle for the timeout (`_kbhit` cannot block); VT **output** explicitly enabled and a **hard startup error** if it cannot be (v3 replacement for `--diag`, D5); VT **input** deliberately **not** enabled; `GetConsoleScreenBufferInfo` for size + resize; **clearing `ENABLE_QUICK_EDIT_MODE`**; verifying under both `cmd.exe` and Windows Terminal |
| **Deliverables** | `terminal_win32.cpp`; `scripts/rehearse_windows.bat`; the T3.4 record in `docs/clion-run-config.md`; `docs/measurements/windows-{refresh,polling}.md`; the graded takes |
| **Budget** | **v3 ~8 h**, essentially unchanged. Δ: no `--diag` verification step (VT failure is now a startup error); the runbook loses the case-file copy; **T3.4 absorbs T0.5's live gate** |

### 4.4 W4 — Kim (macOS + Windows)

**Primary (logic):** the plain-text render engine — scroll math + the frame builder (`renderer.cpp`).
**Secondary:** the measurement sweep (synthesis) and the PPT/video production.

| | |
| --- | --- |
| **Why this person** | Two OS families but **neither Linux** ⇒ safest on platform-independent code that behaves identically on both machines: scroll arithmetic and string assembly. It is also the piece the video depends on visually. |
| **Why measurement goes here** | The PPT must give recommended refresh/polling values *and* the tearing/delay limits for this hardware. Only W4 can produce two baselines personally, and the renderer is what creates the tearing being measured. One head owns *what we measured*, *why it tears* and *how the fix works*. |
| **Deliverables** | `renderer.cpp`, `tests/unit/test_scroll.cpp`, `tests/unit/test_renderer.cpp`; the D14 sweep procedure; `docs/measurements/*` synthesis; the PPT; video direction |
| **Budget** | v2 ~11 h → **v3 ~7 h**. Δ: the glyph table (3 h) and `FrameBuffer` diffing (3 h) are **deleted**; frame assembly replaces diffing (~1 h) |

### 4.5 RACI, writer rules, freeze protocol

| Artifact | W1 Lorens | W2 Byron | W3 Nathan | W4 Kim |
| --- | --- | --- | --- | --- |
| `include/csopesy/*.hpp` contracts | C | **A/R** (freeze owner) | C | C |
| `parameters.*`, `cli.*` | C | **R** | C | C |
| `platform/terminal_posix.cpp` | **R** | C | — | C (macOS review) |
| `platform/terminal_win32.cpp` | — | C | **R** | — |
| `CMakeLists`, CI, `scripts/*`, `CONTRACTS.md` | C | **R** | C | C |
| `interpreter.*`, `line_editor.*`, `main.cpp`, `console_app.cpp` | C | **R** | C | C |
| `renderer.*` | C | C | C | **R** |
| `process.*`, `scheduler.*`, `shutdown.hpp` impl | **R** | C | **R** (Win32) | C |
| `docs/threading-model.md` + the §3.8 threading contract | **R** | **A/R** (contract freeze owner) | C | C (test-double review) |
| `config_io.*`, `glyphs.*`, `frame_buffer.*` | **deleted — no owner** (D1/D2/D6) | | | |
| Windows rehearsal kit + T3.4 graded-run verification | — | C | **R** | C |
| measurement sweep | R (Linux) | C | R (Windows) | **A/R** (synthesis) |
| PPT | **R** (2 sections) | **R** (2 sections) | C | **A/R** (2 sections + assembly) |
| Video | C | C | **R** (drives the Windows takes) | **A/R** (direction, packaging) |

R = responsible, A = accountable, C = consulted. **One writer per file at any time.**

**The v3.0 change is subject to the §4.5 protocol in full (D17).** The sequence is *propose in chat → W2 + the
affected owners agree → bump the `CONTRACTS.md` marker → announce*, and it is **task T0.6** (§5). The affected
owners for this particular change, and what each is being asked to accept:

| Owner | What the change does to their code |
| --- | --- |
| **W2 Byron** (A/R, freeze owner) | The three deleted headers, the new `cli.hpp`, and the `CONTRACTS.md` marker going to v3.0 |
| **W1 Lorens** | `scheduler.hpp`: the constructor loses its `Clock` parameter, and `measure_`/`pendingEventMs_`/`eventOwed_`/`fb_`/`fbRows_`/`fbCols_` disappear. Also the §3.5 shared-state rule now has `noTty` as its named exception |
| **W3 Nathan** | `terminal_win32.cpp` is unaffected, but the VT-failure path changes: a hard startup error replaces `--diag`'s report (D5), and his T3.4 gate loses the `--diag` sanity check |
| **W4 Kim** | `renderer.hpp` loses `drawFrame(FrameBuffer&, …)`/`artWidthFor` and gains `buildFrame(...)→std::string` + `kBandRow`; `Parameters` loses `asciiArt`/`marqueeRow`; the ASCII text contract (D15) is what makes his width math sound |

## 5. Task backlog

Every task that *can* carry a test is TDD: the test exists and **fails first**, then the implementation, then it
passes, then the commit. The stated exceptions are the **platform-surface tasks** — **T1.1**, **T3.1** and
**T3.4** (which now carries T0.5's live gate) — whose real test is the hand-run checklist on the owner's
hardware, because no unit test can observe raw-mode restoration, a stray-click freeze, or a live resize.

### 5.0 Dated schedule — submission **Monday 2026-09-28** (revised from 2026-09-23)

End-of-day gates, not aspirations. If a gate is red at midnight, apply the cut line the next morning rather
than carrying debt forward.

| Day | Date | Goal | End-of-day gate | Leads |
| --- | --- | --- | --- | --- |
| D0 | **Tue Sep 22** (today) | Finish v3 §4/§5 and hand the §4.5 proposal to W2 | §3 + §4 + §5 committed; **T0.6's proposal is in W2's hands**; no header touched | Lorens writes, Byron receives |
| D1 | **Wed Sep 23** | **Contracts re-frozen, then implementation starts in parallel** | **T0.6 ratified and landed** (contracts v3.0, guard map, marker + hash table, run configs, deletions); `check_layers` + `ctest` + CI green | Byron drives, all four agree |
| D2 | **Thu Sep 24** | Foundations green behind tests | T4.1 (Kim), T2.1+T2.2 (Byron), T1.1 (Lorens), T3.1 (Nathan) done; each backend runs on its own machine | all |
| D3 | **Fri Sep 25** | The critical path: the threaded scheduler | T4.2/T4.3, T2.3, T1.2 done; **T1.3's deterministic Step 1b tests green** (`threaded_input_thread_and_animation_overlap_in_real_time`, no `sleep_` in the file) | Lorens (critical), Kim, Byron |
| D4 | **Sat Sep 26** | End-to-end app on Linux **and** Windows | T1.3 done; T2.4, T2.5, T3.2 done; the real binary runs on both; the `--no-tty` smoke test green in CI; no `-Wall -Wextra` warnings | all |
| D5 | **Sun Sep 27** | Verification, measurements, README | T3.4 Windows gate + T5.1/T5.2 integration; **A1–A11** pass on the Windows build (A12 is the dev/CI plain-line case, not a graded take); T1.4/T3.3/T5.3 sweep tables committed; T2.6 `README.txt` | Nathan + Kim, Byron verifies |
| D6 | **Mon Sep 28** | Freeze, record, submit | T6.3 tag + SHA-256 in `docs/frozen-artifact.md`; §7 runbook dry run; T6.1 PPT assembled; T6.2 takes recorded with the MP4 embedded, ≤1 GB, ≤720p; deliverables uploaded + a backup copy | Nathan runs, Kim directs, Byron submits |

**Cut line** (apply in order; never cut a spec requirement). v2's cut line is largely *pre-applied* by v3 — the
glyph table, `FrameBuffer` diffing, `--diag`, `--measure`, `marquee_row` and the config layer are already gone.
What remains:

1. **T4.3's tight-terminal priority polish** — ship the simple version first (`bandWidthFor` floors at 1, rows
   clipped to `cols`), and only add the full band/prompt-never-dropped ordering if it is free.
2. **T5.3's optional columns** — idle-CPU-per-thread numbers and the macOS second-opinion pass.
3. **PPT verbosity** — trim prose, never a required section.

**Never cut:** the six commands; terminal restore on every exit path; the §3.5 ASCII text contract (the scroll
math's width model depends on it); the two-thread design (professor-mandated, answer #3); A1–A12; the video
constraints; `README.txt`'s entry-file statement.

---

### Phase 0 — contracts and a green pipeline (Byron leads; Lorens/Nathan/Kim review)

**Status (2026-09-22):** T0.1–T0.4 **complete**, with evidence in v2 §T0.1–§T0.4 (their task bodies are
unchanged and remain normative). T0.5 **partial**: the scriptable, committed half is done — `CMakePresets.json`
and both run configurations landed in `78dc8a3` — while the CLion-side live gate is **moved to T3.4** by v3,
because gpt-v12 is right that no part of it can be observed before features exist (and its `--config` steps are
deleted anyway). **T0.6 is new in v3 and gates every Phase 1 task.**

Re-verified locally on 2026-09-17: `bash scripts/check_layers.sh` → `check_layers: OK (26 files scanned)`;
`ctest --preset debug` → **1/1 passed**; `git hash-object include/csopesy/*.hpp` → **13/13 match** `CONTRACTS.md`.
CI: runs `35118223659` / `35118445611` (T0.3), `35119216051` (T0.4), `35120070872` (T0.5) — 3/3 jobs green on
each. Commits: `fd0bb63` (T0.1) · `38818ad` (T0.2) · `7f541c2` (T0.3) · `3d5d0e5` (T0.4) · `78dc8a3` (T0.5,
scriptable half). **Phase 0's green CI does not prove the program works** — every `src/*.cpp` is still a `TODO`
stub, and the `unit` target reports `OK 0 tests` on purpose.

#### T0.6 — Re-freeze the contracts as **v3.0** (W2 A/R; W1/W3/W4 consulted) — **the gate for Phase 1**

**Files:** `include/csopesy/{parameters,renderer,scheduler}.hpp` (rewritten), `include/csopesy/cli.hpp` (new),
`include/csopesy/{config_io,glyphs,frame_buffer}.hpp` (deleted), `scripts/check_layers.sh`,
`CONTRACTS.md`, `.idea/runConfigurations/*.xml`, `config/csopesy.ini` (deleted), `CMakeLists.txt`'s source and
test lists, and the doc-comment references in `src/**` that name the old tasks.

- [x] **Step 1 — propose (no file may change yet).** Sent to W2 and the affected owners as §10's paste-ready
text. **W2 Byron agreed 2026-09-22.**
- [x] **Step 2 — record the agreement.** Recorded as **D18** in `PLAN_V3_PROGRESS.md` §4, with its provenance gap
stated there (W3's and W4's individual acknowledgment is not in-repo).
- [x] **Step 3 — apply, in §3.12's order.** Headers → `layer_of_header()` map → `CONTRACTS.md` marker + the
regenerated `git hash-object` table → both run configs' `PROGRAM_PARAMS` cleared and `config/csopesy.ini` deleted
→ the six source/test deletions → the CMake target lists.
- [x] **Step 4 — the guard count moved exactly as predicted: `check_layers: OK (22 files scanned)`.**
26 − 3 deleted headers − 3 deleted `.cpp` + `cli.hpp` + `cli.cpp` = 22, re-run on the finished tree
2026-09-22. A different number would have meant a file was missed on either side.
- [x] **Step 5 — build and freeze verified locally.** `cmake --preset debug` → `cmake --build --preset debug` →
`ctest --preset debug` reports *100% tests passed, 0 tests failed out of 1 (unit)*. **11/11** hashes match
`git hash-object include/csopesy/*.hpp`, and the **7 headers nobody needed to touch still carry their exact
v2.6 blob hashes** — checkable evidence that this is the delta claimed. **The CI half is unobserved** until the
push (see *still open* below).
- [ ] **Step 6 — announce. NOT DONE: this one is the operator's to send.** The group message stating the marker
is v3.0, the three deleted headers, and the four contract removals (`asciiArt`, `marqueeRow`, `measurePath`,
`Clock`) — so nobody re-adds one from memory. Paste-ready text: `PLAN_V3_PROGRESS.md` §10.
- [x] **Step 7 — committed** as **`d6379df`**
`chore(contracts)!: v3.0 — no config layer, plain-text band, CLI-only flags (R3, R7, R8)` (30 files, the preset
export deliberately kept out of it as `cc74d5d`).
  **Evidence recorded:** the guard's count line, the `ctest` summary, the hash check.

> **Still open from T0.6 — two items, neither blocks Phase 1.** (a) **Step 6's group announcement**: a human
action, and its text is ready in `PLAN_V3_PROGRESS.md` §10. (b) **The CI run on all three OSes**: it requires
pushing `d6379df`, since CI is triggered by the push rather than by the commit. Local verification is green; CI
is simply unobserved until then, and `AGENTS.md` §8 item 2 stays open until it is.

### Phase 1A — W4 (Kim): the plain-text marquee (pure logic, no platform)

Task ids keep their v2 numbers; `T4.1` and `T4.3` are **re-scoped**, and v2's `T4.4`/`T4.5` are absorbed into
`T4.2` because the band is now one row of text and the header region is part of the same frame builder.

| Task | Deliverable | Test that must fail first | Commit subject |
| --- | --- | --- | --- |
| **T4.1** — scroll math + `sliceRow` (absorbs v2 T4.2; v2 T4.1's glyph table is **deleted**) | `scrollOffset`, `sliceRow` per §3.9 | length is **exactly** `bandWidth` for short/equal/long/empty text; `offset == 0` ⇒ all spaces; `offset == bandWidth` ⇒ `text[0]` in column 0; wrap at `textWidth + bandWidth`; never indexes out of range | `feat(marquee): plain-text scroll math and sliceRow (R3, R7)` |
| **T4.2** — frame layout (absorbs v2 T4.4 band composition + T4.5 header region) | `Renderer::buildFrame`, `bandWidthFor`, `kBandRow` per §3.9 | every chrome row present for a normal size; band row is row 3; `Welcome to CSOPESY!` with the capital W; no line exceeds `cols` | `feat(marquee): plain-text frame layout (R2, R3)` |
| **T4.3** — frame invariants + tight terminals (replaces v2's `FrameBuffer` task) | the §3.9 invariants + the band/prompt-never-dropped priority | band and prompt rows survive `rows`/`cols` extremes; a size change alone changes the next frame's width (no invalidation flag exists); the whole frame is **one** string | `feat(marquee): frame invariants and tight-terminal priority (R3)` |

**Do not** reintroduce a glyph table, a `FrameBuffer`, an `ascii_art` flag, or a configurable band row — all
four are deleted by D2/D3/D6, and v2 §T4.1's two hardcoded assertion values (`"C S 0"` = 29 columns,
`rows[0]` for `"AB"`) are obsolete with the glyph engine gone.

### Phase 1B — W1 (Lorens): POSIX backend + process/scheduler

| Task | Deliverable | Test / hand-check | Notes |
| --- | --- | --- | --- |
| **T1.1** — `PosixTerminal` | `terminal_posix.cpp`: raw mode, live size, restore, signals | **hand-run checklist** (platform-surface carve-out): `exit`, `Ctrl+C`, `SIGTERM` all restore the terminal; `stty -a` after each; resize repaints | v2 §T1.1's body is normative and unchanged |
| **T1.2** — `MarqueeProcess` PCB | `process.cpp` | `start()`/`stop()` return `false` when already in that state; `start()` clears `hasRendered` | v2 §T1.2, unchanged |
| **T1.3** — the threaded scheduler | `scheduler.cpp`: two threads, one mutex, one cv, one writer | the Step 1b deterministic tests, including `threaded_input_thread_and_animation_overlap_in_real_time`; `grep -n 'sleep_' tests/unit/test_scheduler.cpp` prints **nothing** | **The critical path.** v2 §T1.3's body is normative with the §3.8 delta list applied: `term_.nowMs()` instead of the injected `Clock`, `buildFrame` + one `write()` instead of the `FrameBuffer`, and no `--measure`/`pendingEventMs_`/`eventOwed_`. The three lock rules, `stop_`-as-the-only-exit-signal, `~Scheduler` joining, and no `detach()` anywhere are unchanged |
| **T1.4** — the Linux measurement sweep | `docs/measurements/linux-{refresh,polling}.md` per **D14** | no test — an observation table, with the hardware, terminal, and the observed tearing/typing-delay thresholds | W1 records Linux; W4 synthesizes (T5.3). Sweep `refreshMs` via `--refresh-ms=`/`set_speed` and `pollingMs` via `--poll-ms=` |

### Phase 1C — W3 (Nathan): Win32 backend, rehearsal kit, graded-run verification

| Task | Deliverable | Test / hand-check | Notes |
| --- | --- | --- | --- |
| **T3.1** — `Win32Terminal` | `terminal_win32.cpp` | **hand-run checklist**: VT output enabled or a **hard startup error**; QuickEdit cleared (click during animation, no freeze); arrows via `0`/`224`; resize; both `cmd.exe` and Windows Terminal | v2 §T3.1's body is normative. Δ: the VT-failure report is a startup error, not a `--diag` field (D5) |
| **T3.2** — rehearsal kit | `scripts/rehearse_windows.bat` | rehearsal only, **never graded evidence** | Δ: no `quiz_case_<n>.ini` copy step exists any more (D1) |
| **T3.4** — graded run-configuration verification on Windows | the record in `docs/clion-run-config.md` | **hand-run**: `csopesy-dev` builds and runs; `csopesy-quiz` launches the **frozen** `frozen/csopesy.exe` with **no** Build step and **no** arguments, and its SHA-256 is unchanged after the press | This task **absorbs T0.5's live gate** and closes handoff §5.3 (does CLion actually load the committed `CustomBuildApplication`?). It is now the video path's precondition |
| **T3.3** — the Windows measurement sweep | `docs/measurements/windows-{refresh,polling}.md` | no test — the D14 observation table on the graded machine | W3 records; W4 synthesizes |

### Phase 1D — W2 (Byron): parameters, CLI, interpreter, entry point, README

| Task | Deliverable | Test that must fail first | Notes |
| --- | --- | --- | --- |
| **T2.1** — `Parameters` | `parameters.cpp`: clamping + the ASCII text rule | `setRefresh`/`setPolling` clamp and report; `setText` returns `Empty` for blank, `NonAscii` for any byte outside `0x20`–`0x7E`, and `Ok` otherwise with internal space runs preserved | §3.5. **Do not** re-add `asciiArt`/`marqueeRow`/`measurePath` |
| **T2.2** — the CLI parser (was "config parser") | `cli.cpp`: `parseCli`, `CliResult` | unknown flag warns and continues; malformed/absent value warns and keeps the default; out-of-range is clamped **and reported**; `--refresh-ms 50` (two tokens) warns; a repeated flag's last value wins; `--config=…` warns like any unknown flag | §3.6. The v2 rule survives intact: **a typo must never cost the quiz** |
| **T2.3** — line editor | `line_editor.cpp` | printable ASCII appends; Backspace deletes; Enter submits; arrows/Tab/control bytes ignored; `visibleSlice` shows the tail so the cursor stays visible | v2 §T2.3, plus §3.11's ASCII-on-entry delta |
| **T2.4** — interpreter + response table | `interpreter.cpp` | every §3.7 row, including the new non-ASCII rejection; `HELP` is unrecognized; `5abc` is a `Usage:` error; `-5` clamps and reports | v2 §T2.4's body is normative; only the `set_text` outcomes grew (three, not two) |
| **T2.5** — `main.cpp` + `ConsoleApp` | the graded entry path + `--no-tty` plain line mode | the real-binary **smoke test** (`tests/smoke/`) driving stdin lines in plain line mode; exit code 0; terminal restored | v2 §T2.5's body is normative; `main` now calls `parseCli` instead of loading a file. The plain-line path starts **no worker thread** |
| **T2.6** — `README.txt` + frozen artifact | member names, run instructions, the **entry file** statement | not a test — a pre-flight read-through against §1.3 | v2 §T2.6, unchanged. The run instructions now describe three flags and **no config file** |

### Phase 2 — integration and cross-OS verification

| Task | Deliverable | Evidence |
| --- | --- | --- |
| **T5.1** — integration on Windows (the primary target) | **A1–A11** passing on the Windows build (A12 belongs to the CI smoke test, not a graded take) | the runbook transcript / recordings |
| **T5.2** — cross-OS matrix | Linux + Windows verified; macOS as a second opinion | `ctest` on each; the CI run URLs |
| **T5.3** — tearing and typing-delay synthesis | the PPT's table: recommended `refreshMs`/`pollingMs` per machine, plus the observed thresholds | the three `docs/measurements/*` tables; **the honest limitation is stated**: observed thresholds, not per-keystroke latencies (D4) |

### Phase 3 — documentation, PPT, freeze

| Task | Deliverable | Gate |
| --- | --- | --- |
| **T6.1** — PPT sections (owners per §4.5) | six sections incl. the four machine baselines; the tearing claim uses §3.9's wording | every handout-required section present; the video embedded (T6.2) |
| **T6.2** — video production | ≤1 GB, 480p–720p, uncut, **Run/Debug press visible**, no source access once running, MP4 embedded in the PPTX | W4 directs, W3 drives the Windows takes |
| **T6.3** — freeze | tag `quiz-frozen`, the binary copied to `frozen/` (**never committed**), SHA-256 recorded in `docs/frozen-artifact.md`, §7 runbook dry run | the SHA-256 is unchanged after a `csopesy-quiz` Run press (T3.4) |

---

## 6. Verification and definition of done

### 6.1 Definition of done (per workstream)

1. The task's test existed, **failed before** the implementation, and passes after — **or**, for the
   platform-surface tasks named in §5 (**T1.1**, **T3.1**, **T3.4**), the task's hand-run checklist was executed
   on the owner's hardware and its result recorded. Demanding a red unit test for `tcsetattr` or
   `SetConsoleMode` would produce theatre, not evidence.
2. **Concurrency tests are deterministic, not merely green once.** `tests/unit/test_scheduler.cpp` contains **no**
   `std::this_thread::sleep_for`/`sleep_until` — T1.3 Step 4 greps for it and must print nothing. A real-time
   sleep is a flake waiting to happen.
3. CI is green on all three OS families.
4. No contract header changed without the §4.5 protocol. **In v3 the only planned header change is T0.6, and it
   is gated on a recorded ratification** — that is the whole reason T0.6 has a Step 2.
5. The change is demonstrated on the owner's own hardware (screenshot/transcript).
6. The commit message names the requirement (R1–R9) it advances.
7. **The concurrency claim is backed by concurrency evidence, not by green unit tests.** T1.3's
   `threaded_input_thread_and_animation_overlap_in_real_time` proves overlap; the deterministic suite proves
   race-freedom and state serializability and does **not** prove real-time overlap. The PR says which one it
   relied on, and A8 is the acceptance-level evidence.

### 6.2 Mandatory acceptance tests ("varying inputs" is the full-credit criterion)

The handout requires **six** commands, so the first six cases are the requirement itself — `help` (A1) and
`exit` (A6) are graded commands exactly like the four that change the marquee. A7–A12 are the robustness cases
that the handout's *"varying inputs"* wording grades.

| # | Case | Varying inputs | Expected |
| --- | --- | --- | --- |
| A1 | `help` | at a clean prompt; while animating | the six-line table, identical both times, listing all six commands |
| A2 | `start_marquee` | when stopped; when already running; immediately after `stop_marquee` | `Marquee started.` / `Marquee is already running.` / stopped then started |
| A3 | `stop_marquee` | when running; when never started | `Marquee stopped.` / `Marquee is not running.` |
| A4 | `set_text` | single word; multiple words; punctuation; digits; 200+ chars; lowercase; extra internal spaces; while stopped; **empty**; **non-ASCII** (`café`, `日本語`, a lone high byte) | echoed text equals the argument after §3.7's rule and internal runs are preserved; empty ⇒ `Usage:` and unchanged; **non-ASCII ⇒ the §3.7 rejection line, text unchanged, no mojibake, band width unchanged** |
| A5 | `set_speed` | `1`, `16`, `100`, `1000`, `10000`, `0`, `-5`, `10001`, missing arg, `abc`, `5abc`, `1.5`, a huge number | a well-formed out-of-range value (incl. `-5`) is clamped with an explicit message; a non-number is `Usage:`; never a crash; the rate visibly changes |
| A6 | `exit` | while animating; with text in the buffer | the goodbye line, terminal restored (no `stty sane` needed), exit code 0, shell prompt returns, **exits promptly** (no join hang) and `ps -T` / Task Manager shows no leftover thread |
| A7 | Unknown / empty input | `foo`, `HELP`, `start marquee`, whitespace only, `set_speed 100 extra` | `not recognized` / `Usage:` messages, no crash, prompt intact (`HELP` is unrecognized: matching is case-sensitive) |
| A8 | **Coexistence (stress) — the real-concurrency case** | type a full command **one key at a time** during animation; resize mid-animation; type during a `1 ms` refresh | typing stays responsive; **several marquee updates are observed between the first keystroke and `Enter`** — record the transcript and **state the observed count**, because this is the one case the deterministic suite cannot substitute for; the band is never half-drawn and no frame interleaves or garbles another; the layout follows the new size after a resize |
| A9 | **Parameter-path parity** (narrowed in v3) | the same effect through a **CLI flag** and through the **runtime command**, on the **frozen** binary: `--refresh-ms=50` vs `set_speed 50` | identical observable behavior (same band rate). `pollingMs` has only the flag; `text` has only the command. **v2's config-file path is gone (D1), so this hedge is two paths, not three** — a deliberate narrowing, recorded rather than glossed |
| A10 | Extremes | `refresh_ms = 1` and `10000`; `polling_ms = 1` and `1000` | no input loss, no crash, stays responsive. Idle CPU and Ctrl+C latency are **observations** for the PPT, not pass/fail criteria |
| A11 | **Tiny terminal** (new in v3) | resize to about 20×5 and back | the band row and the prompt row both survive; no line wraps; no crash; the layout recovers on resize back |
| A12 | **Plain line mode** (new in v3; dev/CI only, D13) | drive stdin lines: `help`, `set_text`, `set_speed`, `start_marquee`, `stop_marquee`, `exit` | plain responses, no ANSI, no frames, **no worker thread started**, exit 0. This is the CI smoke test's case; it is **not** part of the graded run |

### 6.3 What "proof" means here

- **Unit tests:** `ctest` output pasted into the PR — and the task's test must have **failed before** the change.
- **Threading:** the T1.3 `ctest` run, plus an optional local TSan/ASan transcript if the toolchain has it. A
  clean TSan run is what "no obvious data races" is allowed to mean here — it is evidence, not a guarantee;
  the §3.8 ownership table is the actual argument.
- **Concurrency is a claim distinct from race-freedom:** the real-overlap test **plus** the A8 transcript. A
  green deterministic suite is never offered as proof that two activities overlapped in time.
- **Platform behavior:** a terminal transcript/screenshot from the owner's machine. For T1.1, T3.1 and T3.4
  that transcript **is** the task's test, which is why those tasks ship a checklist instead of a failing test.
- **Measurements:** the D14 manual sweep tables under `docs/measurements/`, with the honest limitation stated —
  observed thresholds and recommended values, **not** per-keystroke latency figures (there is no telemetry in
  v3 by design).
- **Quiz readiness:** a full dry run of §7 against the frozen SHA-256.

**Simulation is not verification.** Checking that the *specified* algorithm is self-consistent — arithmetic,
thresholds, deadlines — proves nothing about C++, compilation, or a real terminal. A task is done when its own
test fails first and then passes under `ctest` (or, for the platform-surface tasks, when its checklist passes on
the owner's hardware), and the behaviour is observed on the owner's hardware.

---

## 7. Quiz-day runbook (time-pressure)

```text
T-0:00  Repo root. Confirm the frozen binary is the tagged build:
        Windows: certutil -hashfile frozen\csopesy.exe SHA256
        POSIX:   sha256sum frozen/csopesy
        Compare against docs/frozen-artifact.md. A mismatch is a STOP: do not record that take.
T-0:02  MEASUREMENT TAKE ONLY (D14): set the csopesy-quiz run configuration's Program arguments to
        --poll-ms=N --refresh-ms=M.  A graded BEHAVIOR take leaves them EMPTY.
        There is no case file to copy any more (D1) -- the case is set entirely by the commands below.
T-0:03  Start the recorder (uncut, 480p-720p) BEFORE launching.
T-0:05  PRESS RUN/DEBUG IN THE IDE on `csopesy-quiz` -- the handout requires the video to show this.
        `csopesy-quiz` is a CustomBuildApplication whose Executable is frozen\csopesy.exe and whose
        Before-launch has NO Build step, so the press cannot recompile anything (proven at T3.4).
T-0:10  Execute the case's commands exactly as written.  Any of the SIX is legitimate, `help` and `exit`
        included -- the handout requires all six, not only the four that change the marquee.
        Do not improvise; do not open a second window; do not touch source.
T-0:40  `exit`, stop the recorder, note the take number and case id.  Never delete a take.
T-1:00  Repeat for the next case, keeping the terminal geometry identical so the PPT's tables match the video.
```

**Fallback ladder if a case misbehaves mid-quiz:** (1) retry the same command; (2) reach the same state through
the flag path (`--refresh-ms=`) and keep recording — that is the documented partial-credit route (A9);
(3) if the terminal is wedged, `Ctrl+C` (POSIX) / close the window, restart **without recompiling**, and
continue recording; on POSIX `stty sane` recovers a terminal whose restore was skipped by `SIGKILL`.

**There is no launcher-based evidence path.** `scripts/rehearse_windows.bat` is for practice and for recovering
from a wedged terminal; it never appears in a submitted take. And **no source is opened, edited or rebuilt at
any point during the quiz** — that is the handout's assessment rule, and §7 is written so that nothing about the
runbook requires violating it.

**Pre-flight (T6.3):** the hash matches; `csopesy-quiz` has no Build step; its Program arguments are empty for
graded takes; the recorder is within 480p–720p; `git status` is clean and no source is open in the IDE;
`frozen/` is present on disk and correctly **absent** from git.

---

## 8. Risks and mitigations

Risk numbers are kept from v2.6 so older references still resolve. **Closed** risks are listed after the table
with the reason they closed — they are not silently deleted, because a reader who remembers a numbering would
otherwise wonder whether it was forgotten.

| # | Risk | Impact | Mitigation | Owner |
| --- | --- | --- | --- | --- |
| 1 | Raw mode not restored after a crash ⇒ unusable terminal | quiz-blocking | flag-only signal handler + normal-scope `restore()` + `atexit` backstop; tested with `exit`, `Ctrl+C`, `SIGTERM`; §7's recovery step | W1 |
| 2 | Windows QuickEdit freezes the app on a stray click | quiz-blocking | clear `ENABLE_QUICK_EDIT_MODE` in `enterRawMode`; verify by clicking during animation (T3.1) | W3 |
| 3 | A console without VT support renders escape codes literally | no points for the UI | enable VT **output** explicitly and **fail loudly at startup** if it cannot be enabled — the v3 replacement for `--diag`'s report (D5); prefer Windows Terminal | W3 |
| 4 | macOS-only POSIX divergence discovered late | lost macOS baseline only | non-blocking: Linux first, macOS as a P2 second opinion | W4 |
| 5 | Compile break on a platform nobody is sitting at | lost hours | the 3-OS CI matrix (T0.3) + frozen contracts (T0.4) | W2 |
| 6 | Header churn after Phase 0 | three members blocked | one-writer-per-file; the `CONTRACTS.md` marker; **in v3 the single planned change is T0.6 and it is gated on ratification** | W2 |
| 7 | Busy-spin at a low `pollingMs` burns a core and adds jitter | measurement noise | both threads wait on bounded timed primitives, never on a spin; never `sleep(0)`, never `_kbhit` in a loop | W1/W3 |
| 11 | Video rejected for format reasons | submission invalidated | the §T6.2 pre-flight; §7 forbids launcher-based takes | W4 |
| 12 | The graded binary drifts from source (someone rebuilds, or a Build step relinks) | invalidates the "no recompile" claim | `csopesy-quiz` has **no** Build step and **no** arguments; the `quiz-frozen` tag + SHA-256; T3.4 proves the hash is unchanged after a Run press | W3/W2 |
| 13 | CLion's Run window may not give a real console | quiz-blocking | T3.4's per-config verification (it absorbs T0.5's live gate) + T0.5b's fallback + the record in `docs/clion-run-config.md` | W2 owns the record |
| 14 | CLion builds with a different toolchain than the frozen binary | invalidates the frozen-build claim | T0.5's toolchain pin (scriptable half done) + `csopesy-quiz` runs `frozen/` **without** a build step, so no toolchain is involved at Run time | W2/W3 |
| 16 | Schedule slip | missed submission | §5.0's end-of-day gates; the cut line; a spec requirement is never cut. The move to **2026-09-28** gave back the days consumed by the v2.6 → v3 re-plan | Byron |
| 17 | Enabling VT **input** alongside `_getch` turns keys into escape sequences | broken input on Windows | VT input deliberately **not** enabled; arrows come from the `0`/`224` prefix | W3 |
| 18 | `_kbhit()` used as a wait ⇒ 100 % core, corrupted measurements | measurement noise | `WaitForSingleObject(consoleInput, timeoutMs)` then `_kbhit`/`_getch` | W3 |
| 20 | The sweep's numbers are read as **per-keystroke** latency | a false claim in a graded report | D4's wording is binding: the sweep reports *observed thresholds and recommended values*, never per-keystroke latency; T5.3 states the limitation where the numbers are published | W4 |
| 21 | Deadlock or lock inversion between the two threads | hangs the graded run | exactly **one** mutex and **one** cv; `tick()` is never called holding `mu_`; terminal writes outside `mu_`; RAII guards in bounded scopes; no nested locks exist, so there is no order to get wrong | W1/W2 |
| 22 | A worker thread outlives the objects it references | crash at exit or mid-quiz | the §3.10 order — `requestStop()` → `join()` → *then* `restore()`; `~Scheduler` joins; **no `detach()` anywhere**; the `unit` ctest `TIMEOUT 60` turns a missed join into a red test | W1 |
| 23 | POSIX link fails on `pthread` | build dead on 3 of 4 machines | `find_package(Threads REQUIRED)` + a `PUBLIC` link on `csopesy_core` (T0.2); it fails at *link* time only | W2 |
| 24 | Threading silently costs CPU (two idle waiters instead of one) | worse PPT numbers | `pollingMs` is the only timer in either thread; T1.4/T3.3/T5.3 report wakeups for both; the PPT states the doubling as the price of the architecture | W1/W4 |
| 25 | The threaded implementation drifts from the assignment's observable behaviour | marks lost on behaviour that already worked | the pure-step tests and A1–A12 stay green throughout; change the architecture, not the contract | W1/W2 |
| 26 | `postEvent()` still waits behind one frame's **render** | perceived typing delay at A4's 200+ character extreme | §3.8 states the narrow true guarantee ("the unbounded console write can never stall the input thread") instead of "the input thread never waits"; size reads and writes stay outside `mu_`; A4/A12 observe the worst case | W1/W4 |
| 28 | A second reader of `Interpreter::quit_` reappears | a cross-thread read needing a correctness argument this project should not have to make | the §3.8 ownership table gives `quit_` its own row; restoring `TickResult::quit` also breaks the v3.0 frozen marker | W1 |
| **29** | **NEW:** someone relaxes the ASCII text contract (D15) without changing the width model | mojibake **and** a band whose width no longer matches its content — `sliceRow`'s invariant silently false | §3.5's invariant + A4's non-ASCII case + §3.7 rule 7 + the do-not-cut list in §5.0 | W4 |
| **30** | **NEW:** a case's text is non-ASCII and the program refuses it | a refused case that has nothing to do with the marquee | the refusal is explicit and harmless (message, state unchanged, prompt intact); the professor confirmed ASCII input is fine; recorded in §9 | W2 |

**Closed in v3, with reasons:** **8** (glyph coverage — there is no glyph table, D2); **19** (a dropped resize —
every frame is rebuilt from a live-size read, so the defect class is structurally absent, §3.8); **27** (`--measure`
read as per-keystroke — there is no `--measure`, D4); **9** ("scheduler expected to be threads") and **10**
("parameters = edit files vs commands") and **15** ("the professor never answers") — all three closed by the
answers recorded in §9, with risk 10's residual carried into A9's two-path wording.

---

## 9. The professor's answers (replacing v2's open questions)

v2 §9 was five questions with fallback assumptions. **All five were answered on 2026-09-22**, so v3 records
answers instead of hedges, and each answer is traced to what it changed.

| # | Question | Answer | Changes |
| --- | --- | --- | --- |
| 1 | Does "modify the parameters" mean editing a **parameter file** on the frozen build, or typing parameters **as commands**? | **No `.ini` file is needed** — parameters are changed through the supported inputs/commands, without modifying or recompiling the source | D1: the config layer, `--config`, `quiz_case_<n>.ini` and §7's copy step are gone. A9 narrowed to two paths |
| 2 | ASCII-art graphics, or is plain text scroll sufficient? | **Plain text scroll is sufficient** (the handout allows either) | D2: the 5×5 glyph engine, `ascii_art` and `--plain` are gone; W4's budget drops 11 h → 7 h |
| 3 | Is two threads — one for the marquee/scheduler, one for the command interpreter — the expected shape? | **Yes**, the professor instructed exactly that | D8: the two-thread design is **kept and is now mandated**; `gpt-v9`/`gpt-v10`'s attempt to cut it is rejected; A8 and §3.8 answer *process representation* + *scheduler implementation* for the PPT |
| 4 | Is the quiz run on your machine or ours, and does the platform matter? | **The professor's Windows machine** | §2: Windows is the graded platform; the graded run config targets Windows; CI keeps the other two compiling |
| 5 | Where exactly should the marquee band sit? | **Near the top, clearly visible; the exact row need not be configurable** | D3: `marquee_row` is gone; `kBandRow = 3` is a renderer constant |

**Confirmed afterwards (2026-09-22, via the operator):** ASCII-only text input is fine (**D15**); `--no-tty` is a
development/CI mechanism rather than a quiz parameter (**D13**); the refresh/polling sweep is performed on the
**frozen** binary by changing only the run configuration's arguments (**D14**).

**Provenance — two gaps, both recorded rather than papered over.** (1) The answers above exist in this repo only
as `gpt-v11.md`'s paraphrase; the raw reply is not committed (`docs/replies/` is gitignored). (2) The revised
**2026-09-28** deadline is operator-stated, with no in-repo artifact. Until the raw texts are pasted into
`docs/replies/`, v3 cites both as *"as recorded in `gpt-v11.md` / operator-stated, 2026-09-22"*. Contract text
cited *from* the worksheet is quoted **verbatim** — `"1. Text marquee or ASCII graphics marquee"` and
`"You should only modify the parameters and no longer recompile the project when taking the quiz."` are both
verified against the PDF with `pdftotext -layout`.

**No open questions remain.** What remains are *assumptions*, listed so nobody mistakes them for verified facts:
the graded machine is Windows; plain text satisfies the marquee requirement; ASCII-only input is acceptable;
the deadline is 2026-09-28.

---

## 10. Plan self-review

### 10.1 Spec coverage check

| Handout item | Where it is satisfied |
| --- | --- |
| OS emulator with command input + display output | §3.3–§3.11, T2.5 |
| Main menu console (welcome, developers, version date, prompt) | §1.4, §3.9 layout, T4.2, T2.5 |
| Text marquee **or** ASCII graphics marquee | §3.9 (the text branch, handout p.2), T4.1–T4.3 |
| Commands that change the marquee's behavior | §3.7 exact strings, T2.4 |
| **All six** commands, `help` and `exit` included | §3.7 + A1–A6 + §7's runbook line that all six are legitimate |
| Parameters modifiable **without recompiling** | §3.6 flags + §3.7 commands, T2.2, A9, T6.3 |
| PPT: the six required topics incl. refresh-vs-polling balance | §4.5 RACI, T6.1, §1.2's measurement requirement |
| Video: Run/Debug press, uncut, 480p–720p, MP4 in the PPTX | §1.3, §7's runbook, T6.2 |
| `README.txt` with the entry file | T2.6 |
| Concurrency (professor's answer #3) | §3.8 (two threads, one mutex, one cv, ownership table), T1.3, A8 |
| Recommended refresh/polling values + the tearing/typing-delay limits | §1.2 + D14's workflow, T1.4/T3.3/T5.3 |

### 10.2 Placeholder scan

`grep -n 'TBD\|TODO\|FIXME\|XXX'` on this document returns **two** hits, both factual statements that
`src/*.cpp` is still `TODO` stubs — not placeholders. There are no `TBD`, no `FIXME` and no orphan
cross-references. **T0.6's seven boxes are all checked except Step 6** — the ratification, the header change, the
predicted guard count, the build/freeze verification and the commit each carry their evidence inline at §T0.6 and
in `PLAN_V3_PROGRESS.md` §4/§7. Step 6 (the group announcement) and the CI run are the two items left open on
purpose: both need a human or a push, neither blocks Phase 1, and the paste-ready announcement is in
`PLAN_V3_PROGRESS.md` §10. Every other task in §5 is written as work to perform, not as a step to tick off in
this file.

### 10.3 Name/type consistency check

- The header set is **11** everywhere it appears (§3.0.1's table, §3.12, T0.6), and the guard-count prediction in
  T0.6 Step 4 (26 → **22**) was verified against the tree on 2026-09-22: 13 headers + 13 `.cpp` today, minus 3
  deleted headers, minus 3 deleted `.cpp`, plus `cli.hpp`, plus `cli.cpp`.
- Every type and function named in §5 exists in §3: `buildFrame`, `bandWidthFor`, `kBandRow`, `sliceRow`,
  `scrollOffset`, `TextResult`, `parseCli`, `CliResult`, `noTty` (each occurs in both sections).
- Task ids keep their v2 numbers where the task survives (T1.1–T1.4, T2.1–T2.6, T3.1–T3.4, T5.x, T6.x); the
  re-scoped ones (T4.1–T4.3) and the absorbed ones (T4.4/T4.5 → T4.2) are stated in place, so a reader coming
  from v2 or the handoff is not misled.

### 10.4 Deliberate deviations from this project's own process rules

1. **§3 is a delta contract** (D16), not a transcription — it writes out every *changed* contract and cites v2.6
   for the unchanged frozen text. This softens D12's "self-contained" into "self-contained in decisions, tasks,
   acceptance and risks". It is safe **only** while v2.6 is immutable; if v2 is ever edited, §3's references go
   stale, which is stated at the top of §3.
2. **Task *bodies* for unchanged tasks stay in v2** (`T0.1`–`T0.5b`, `T1.1`, `T2.4`, `T2.5`, `T3.1`), which v3
   names explicitly as normative. Copying ~1,400 lines of already-adjudicated instructions would create two
   copies to keep in sync.
3. **A9 narrows v2's three-path partial-credit hedge to two paths.** The professor's answer #1 removed the file
   path as a requirement. Recorded as a narrowing, with the alternative (keeping a config layer purely as
   insurance) explicitly declined.
4. **T0.5's live gate moves to T3.4** rather than staying in Phase 0, because no part of it can be observed
   before features exist and its `--config` steps are deleted anyway.
5. **The `help`/`exit` clarification** (gpt-v14) is applied as §6.2's preamble and §7's runbook line, so no
   document in this repo can be read as treating four commands as the requirement.

### 10.5 Explicitly out of scope

P2 extras (the pty replay harness, further platform polish); a thread per marquee process (explicitly not
planned at any scope); reviving the config layer, the glyph table, `FrameBuffer` diffing, `--diag` or
`--measure`; and beyond-CI macOS verification. **Never**: anything the professor specified.

### 10.6 Revision log (v2.6 → v3.0)

| Area | v2.6 | v3.0 | Authority |
| --- | --- | --- | --- |
| Config | `.ini` + `--config` + `quiz_case_<n>.ini` + 4-layer precedence | **removed**; 3-layer precedence (command > flag > default) | answer #1, D1 |
| Marquee | 5×5 glyph engine + `ascii_art` + `--plain` | **plain text, one row**; ASCII-only input contract | answer #2, handout p.2, D2/D15 |
| Band position | `marquee_row` (config/CLI knob) | `kBandRow = 3`, fixed | answer #5, D3 |
| Measurement | `--measure=FILE` + CSV + scripts + coalescing rules | **manual sweep** with a frozen-binary workflow; the PPT requirement kept | D4, D14 |
| Diagnostics | `--diag` | **removed**; VT failure is a hard startup error | D5 |
| Rendering | `FrameBuffer` + row diffing + invalidation | `buildFrame(...)→std::string`, one `write()` per frame | D6 |
| Clock | `Terminal::nowMs()` **and** an injected `Clock` | `Terminal::nowMs()` only | D7 |
| Flags | `--config`, `--plain`, `--art`, `--row`, `--measure`, `--diag`, … | **three**: `--no-tty` (dev/CI), `--refresh-ms`, `--poll-ms` | D10, D13 |
| Headers | 13, marker v2.6 | **11** (+`cli.hpp`; −`config_io`, −`glyphs`, −`frame_buffer`), marker **v3.0** | D1/D2/D6, §3.12 |
| Threads | two threads, one mutex, one cv | **unchanged** — now professor-mandated | answer #3, D8 |
| Infrastructure | FSD + guard, freeze, 3-OS CI, harness | **unchanged** | D9 |
| A9 | config + flag + command | flag + command | D1, §6.2 |
| A11/A12 | — | tiny terminal; plain-line mode | §6.2 |
| Risks | 28 | 24 live + 6 closed with reasons + 2 new | §8 |
| Deadline | Wed 2026-09-23 | **Mon 2026-09-28** | D17 |

### 10.7 Review-round dispositions (`gpt-v9`…`gpt-v14`)

| Review | Disposition |
| --- | --- |
| `gpt-v9` | The config/art/`--measure` cuts match the professor's answers → **accepted**; the threading cut and the FSD/CI/contract cuts → **rejected** |
| `gpt-v10` | Same direction, wider; only the injected-clock and `FrameBuffer`/glyph items adopted, the clock only in the form that **keeps one clock source** |
| `gpt-v11` | **Accepted — it is v3's scope** (written after the professor's answers) |
| `gpt-v12` | **Accepted verbatim** — write v3 first; T0.5's live gate moves behind the features it observes |
| `gpt-v13` | **All three points accepted** (D13, D14, D15), plus the ASCII confirmation from the professor |
| `gpt-v14` | **Accepted** — S4 proceeds without waiting for ratification (it is documentation), while the header change stays gated; its `help`/`exit` watch item is applied in §6.2 and §7 |

The full point-by-point adjudication, in the style of the seven earlier rounds, is **`REVIEW_ADJUDICATION.md`
round 8** — scheduled in S5, after `ctest` is green, because it is internal hygiene and does not gate anyone's
code. Until it lands, this table is the disposition of record.

---

## What still has to land before this plan may be implemented

| Stage | Sections | Status |
| --- | --- | --- |
| **S1** | §0, §1, §2, §3.0 | ✅ |
| **S1.5** | `gpt-v13` absorbed: D13–D16, the frozen-binary sweep workflow (§1.2), the dev/CI-vs-quiz flag split (§2.3), ASCII instead of normalization | ✅ |
| **S2 (plan text)** | §3.1–§3.12 — the delta contract | ✅ |
| **S2.5** | Deadline corrected to **2026-09-28**; D17 recorded (no header change ahead of ratification) | ✅ |
| **S3** | §4 (workstreams + RACI) + §5 (task backlog, T0.6, the Sep-28 schedule) | ✅ |
| **S4** | §6 (DoD + A1–A12), §7 (runbook), §8 (risks), §9 (the answers recorded), §10 (self-review + the revision log). `gpt-v14` absorbed | ✅ this document |
| **S2 (bytes)** | The header edits, the guard map, `CONTRACTS.md` v3.0, the run configs, `config/` — per §3.12 / **T0.6** | ✅ **landed 2026-09-22 as `d6379df`** (30 files). Header set 13 → 11; guard `OK (22 files scanned)`; `ctest` 1/1; 11/11 hashes match. T0.6 Steps 6 (announce) and the CI run are the only parts still open |
| **S5** | `AGENTS.md` (§2 commands, §5 clock + threading invariants, §7 behaviour contracts), v2's superseded banner, `REVIEW_ADJUDICATION.md` round 8 for `gpt-v9`…`gpt-v14` | ⬜ P3 — after `ctest` is green |
