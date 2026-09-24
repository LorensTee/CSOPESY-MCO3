# PLAN_V3_PROGRESS.md — the v3 migration log and resume anchor

**Purpose.** This file exists so that a **lost chat, a crashed session, or a different agent** can resume the
v2.6 → v3 plan migration without re-deriving anything. It is the *working* record; `docs/IMPLEMENTATION_PLAN_v3.md`
is the *deliverable*. When the two disagree, this file says what is actually done and the plan says what is
intended — **update both in the same session**.

**Rule for this file:** update it at the **end of every working session**, before you stop. A session that
changes nothing still gets a dated line in §7. Never mark a stage done without the artifact it produced.

**Established:** 2026-09-22 · **Last updated:** 2026-09-24 (**T6.3 freeze produced** — `frozen/csopesy.exe` (Release) from `4c5e8cf`, SHA-256 `a9cbc09e…1fd4f15f`, tagged `quiz-frozen`, record `docs/frozen-artifact.md`; only the T3.4 CLion Run-press gate remains. **Phase 2: T5.1 + T5.2 done** — T5.1 A1–A11 on a real Win32 console **38/38**, A8 real-overlap recorded (`docs/t5.1-windows-acceptance.md`); T5.2 `ctest` **2/2 on Windows, Linux and macOS** ([CI run 36007416747](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/36007416747), record `docs/t5.2-cross-os-ctest.md`); local gate `ctest` 2/2 (unit 117 + smoke), guard `OK (22 files scanned)`, 11/11 frozen hashes. Prior: **W4: Phase 1A `T4.1`–`T4.3` landed** — `scrollOffset`/`sliceRow` + `Renderer::buildFrame` (plain-text band, chrome, message rows, bottom-anchored prompt, tight-terminal priority), 27 new tests, red-before/green-after; **no frozen header touched** (11/11 hashes), guard `OK (22 files scanned)`, `ctest` 2/2 (unit **117** + smoke). **Phase 1 is now code-complete in all four workstreams.** Prior: W2 Phase 1D `T2.1`–`T2.6` + the same-day follow-up (`feed(Eof)` quit fix `24c41e9`, README Ninja `ff602f4`); W3 `T3.1`/`T3.2`; W1 `T1.1`–`T1.3`.) 
**Deadline: Monday 2026-09-28** (revised from 2026-09-23 — six days, not one). **The §4.5 change-control protocol
is honored in full: no header change lands ahead of ratification.** Stage sequence in §9.

---

## 1. Resume here

| | |
| --- | --- |
| **Deadline** | **Monday 2026-09-28** (revised from 2026-09-23 on 2026-09-22 — operator-stated; see §6 provenance). Six days. Urgency removed, so **D17** applies: no workaround for §4.5. |
| **Current stage** | **T0.6 — DONE and fully closed. Phase 1 is CODE-COMPLETE: T1.1–T1.3 (W1), T2.1–T2.6 (W2), T3.1–T3.2 (W3) and T4.1–T4.3 (W4) are all implemented.** Contracts are **v3.0**, committed as **`d6379df`** (2026-09-22; guard `OK (22 files scanned)`, `ctest` 1/1, **11/11** hashes match). **T1.1 (`src/platform/terminal_posix.cpp`, `PosixTerminal`) is implemented, pty-verified, and pushed with CI green on all three OSes** ([run 35728920428](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/35728920428)). **T1.2 (`MarqueeProcess` PCB, `src/entities/process.cpp`) is implemented, unit-tested, and pushed as `ed18908` — CI is green on all three OSes** ([run 35732133066](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/35732133066): ubuntu 20 s, macOS 18 s, Windows 1 m 14 s) — 7 tests, red-before/green-after, guard `OK (22 files scanned)`, 11/11 hashes unchanged (§7). **T1.3 (the threaded scheduler, `src/app/scheduler.cpp`) is implemented, red-before/green-after, and locally verified** — 16 tests, 53 assertions red against the stub then green, `ctest` 1/1, 150 consecutive clean runs, `grep -n 'sleep_' tests/unit/test_scheduler.cpp` prints **nothing**, guard `OK (22 files scanned)`, 11/11 hashes unchanged (§6, §7). Its tests are deliberately **scheduler-only**: assertions needing another owner's implementation are deferred and listed in §6, not faked. Landed as **`95b0b61`** and pushed; **CI [run 35735238660](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/35735238660) is green on all three OSes** (ubuntu-latest 53 s, macos-latest 49 s, windows-latest 47 s) — the first CI run in which the `unit` target drives a real worker thread, which is the 3-OS half of `AGENTS.md` §8 item 2. **T3.1 (`536f865`) + T3.2 (`cddba69`) are pushed; CI [run 35866073135](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/35866073135) is green on all three OSes** — T3.1's real-app checks (animation while typing, both consoles, QuickEdit, Ctrl+C) defer to T3.4, so the implementation is done but its live verification is not. **W4's `T4.1`–`T4.3` (`src/features/marquee/renderer.cpp`) are implemented, red-before/green-after, and locally verified** — 27 new tests (scroll math + `sliceRow` width invariant; frame layout, band at `kBandRow`, capital-W welcome, clipping; tight-terminal band/prompt priority, message rows, full rebuild), **180 assertions red against the stub then `OK 117 tests`**, `ctest` 2/2 (unit 117 + smoke), guard `OK (22 files scanned)`, 11/11 hashes unchanged, warning-free clean rebuild (§7). **Phase 1 is now code-complete in all four workstreams.** |
| **Next action** | **W1's `T1.3` is CLOSED** (landed `95b0b61`, CI green on all three OSes). **`T1.4` (the Linux measurement sweep) is not actionable yet**: D14 wants observations from the **frozen** binary (`T6.3`), never from a rebuilt debug binary — do not fake it with a unit-test-shaped substitute. The §6 deferred T1.3 assertions are now **unblocked**: `T4.2` (`Renderer::buildFrame`) and `T2.4` (`Interpreter::feed`) both landed 2026-09-24, so the "only the marquee thread ever writes" / "one whole frame per `write()`" / "echo redraw is not gated by `refreshMs`" cases can now be added to W1's `tests/unit/test_scheduler.cpp`. **W3's `T3.1`/`T3.2` are CLOSED** (pushed `536f865` + `cddba69`, CI green); W3's next work is `T3.4` — the W2/W4 stubs it was waiting on have now landed (`T2.x` and `T4.1`–`T4.3`), so the live CLion gate is actionable; only `frozen/csopesy.exe` (`T6.3`) is still outstanding. Task bodies and ids are in `IMPLEMENTATION_PLAN_v3.md` §5 — ids keep their v2 numbers, and §3.8's delta list is what applied to the scheduler (`term_.nowMs()` not `Clock`, `buildFrame` + one `write()`, no `--measure` fields). |
| **T0.6 Step 6 — CLOSED** | **The group announcement (§11) was sent** — operator-stated 2026-09-22 (a human action, recorded here rather than as an in-repo artifact). Together with the green CI run [35724109198](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/35724109198) on all three OSes (ubuntu 17 s, macOS 26 s, Windows 45 s), **every T0.6 step is now closed** and `AGENTS.md` §8 item 2 is satisfied. |
| **Blocked on** | **Nothing.** The §4.5 gate is satisfied and recorded as **D18** (W2 Byron, the freeze owner, 2026-09-22). If W3 or W4 objects to the change, `d6379df` is one revertable commit; when it landed every `src/*.cpp` was still a `TODO` stub, so nothing consumed it yet (T1.1 is the first consumer, 2026-09-22). |
| **W2 Phase 1D — implementation complete 2026-09-24** | ✅ **`T2.1`–`T2.5` complete; `T2.6`'s README complete. The frozen-artifact portion of `T2.6` is intentionally deferred to `T6.3`. TDD red-before/green-after per task.** `parameters.cpp` (clamp + `ClampReport`; `setText` trim/internal-run → Ok/Empty/NonAscii), `cli.cpp` (three flags; unknown/malformed warn+continue, out-of-range clamps+reports, last-wins), `line_editor.cpp` (`feed` echo/Backspace/Enter-Eof/ignored arrows + `visibleSlice` tail window), `interpreter.cpp` (case-sensitive dispatch table, exact §3.7 lines, `[-+]?[0-9]+` full match, three `set_text` outcomes), `main.cpp` + `console_app.cpp` (RAII raw-mode guard, initial frame before `start()`, input/command loop, `requestStop`→`join`→goodbye order, `--no-tty` plain line mode with **no worker**) + the portable `ctest smoke` real-binary test, `README.txt`. **`T2.4` clears the blocker on W1's deferred T1.3 command-effect assertions; `T2.5` removes the W2 application blocker from `T1.4` and `T3.4` — both remain gated on the finished application and, for `T3.4`, on `frozen/csopesy.exe` (`T6.3`).** The composition root is **locals in `run()`**, not new private members, so `console_app.hpp` keeps its v2.6 hash (risk 6 not reopened). `T2.6`'s frozen-artifact half deliberately moves to `T6.3`. Commits `14eee96`,`135a950`,`a684816`,`f4a9411`,`f0c6fab`,`71eafb8`,`89d5fe1`, plus same-day review follow-ups `24c41e9` (Eof quit fix) and `ff602f4` (README Ninja prerequisite + §9 comment cleanup). |
| **Phase 2 — T5.1 (Windows) DONE 2026-09-24** | ✅ **A1–A11 pass on the Windows development build** (`97ce13a`): **38/38** automated checks driven on a real Win32 console (`CREATE_NEW_CONSOLE` → `AttachConsole`, keys via `WriteConsoleInputW`, screen read via `ReadConsoleOutputCharacterW`, live `SetConsoleScreenBufferSize`/`SetConsoleWindowInfo` resize). A8 real-overlap recorded — **29 distinct band positions over 29 one-key-at-a-time keystrokes** at `refresh-ms=40` (and **13/13** at `set_speed 1`), resize mid-animation followed, command echoed; A9 parity `--refresh-ms=50` vs `set_speed 50` = 16 vs 16 positions. A4 non-ASCII (rule 7) confirmed on the `--no-tty` raw-stdin path; the interactive line editor drops high bytes by design — the acceptance/spec tension is recorded, not patched. Gate: warning-free build, `ctest` 2/2 (unit **117** + smoke), guard `OK (22 files scanned)`, **11/11** frozen hashes. Evidence: `docs/t5.1-windows-acceptance.md`. **T5.2's Windows half is now evidenced; global T5.2 still needs the Linux `ctest`.** T3.4/T3.3 remain blocked on `frozen/csopesy.exe` (`T6.3`). |
| **Phase 2 — T5.2 (cross-OS) DONE 2026-09-24** | ✅ **Linux + Windows verified, macOS as the second opinion** — the plan's evidence for T5.2 is *`ctest` on each* + the CI run URLs. Commit `dbfc8b9` (docs-only T5.1 record) pushed; **CI [run 36007416747](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/36007416747) success on `ubuntu-latest`, `windows-latest`, `macos-latest`** — Layer guard `OK (22 files scanned)` and `ctest` **2/2** in every job (ubuntu unit 1.87 s/smoke 0.00 s; windows unit 7.01 s/smoke 0.06 s; macOS unit 2.31 s/smoke 0.03 s; unit binary `OK 117 tests`). Windows is additionally backed by the T5.1 real-console A1–A11 pass; macOS by W4's byte-exact pty pass. **Honest scope:** the Linux column is CI build + `ctest` only — no human A1–A11 on a Linux terminal is claimed (`PosixTerminal` was pty-verified at T1.1). Evidence: `docs/t5.2-cross-os-ctest.md`. |
| **T6.3 — freeze PRODUCED 2026-09-24** | ✅ **The graded artifact now exists.** A clean **Release** build (`cmake --preset release`, `ctest --preset release` 2/2) was copied to `frozen/csopesy.exe` — gitignored, never committed — and tagged **`quiz-frozen`** at `4c5e8cf`. Source: `97ce13a` (last code commit; the two commits above are docs-only). **SHA-256 `a9cbc09e7ffbefda9f2e3714845995e0320fd349ac47fe7f531d15ec1fd4f15f`**, 186 436 bytes, 0 warnings under `-Wall -Wextra` (GCC 14.2.0 MinGW-W64 UCRT). §7 pre-flight: `frozen/` ignored, `csopesy-quiz` has no Build step + `PROGRAM_PARAMS=""`, the binary exits 0 on `--no-tty`. **Remaining gate (T3.4, hand-run):** press Run on `csopesy-quiz` and confirm the SHA-256 is unchanged; that press also resolves the documented `EMULATE_TERMINAL`/`USE_EXTERNAL_CONSOLE` choice (flipping it does not change the binary). Record: `docs/frozen-artifact.md`. **This unblocks T3.3/T1.4 (sweeps), T3.4 and T5.3.** |
| **Do not do** | Do **not** perform v2's T0.5 manual checklist (CLion GUI, terminal gate, `--diag` sanity check). `gpt-v12.md` is right: its steps reference `--config=config/csopesy.ini`, which v3 deleted, and the surviving audit moved to **T3.4** — a hand-run checklist that needs features to observe. |
| **Then** | S6 (implement Phase 1) → S5 (`AGENTS.md` follow-ups, the v2 superseded banner, adjudication round 8). The full sequence is §9. |

## 2. How to resume in a fresh chat (read in this order)

1. `AGENTS.md` — repo rules (layers, frozen contracts, threading invariants, definition of done).
2. **This file** — where the migration stands and what not to redo.
3. `docs/IMPLEMENTATION_PLAN_v3.md` — the target (complete, and **v3.0 is applied**; read the banner).
4. `docs/replies/gpt-v9.md` … `gpt-v12.md` — the review input that triggered v3 (§3 below).
5. `docs/REVIEW_ADJUDICATION.md` — why v2.6 looks the way it does, **especially** the points v9/v10 try to re-open.
6. `docs/IMPLEMENTATION_PLAN_v2.md` — still the only complete frozen plan until v3 is finished. Never implement
   from it without checking §3 of this file first.

## 3. Why v3 exists — the four replies, and what each one actually settles

| File | What it argues | Disposition |
| --- | --- | --- |
| `gpt-v9.md` | `.ini` is our engineering choice, not the professor's requirement; the plan is over-engineered; cut `.ini`, CLI duplication, `quiz_case_*.ini`, `--diag`, `--measure`, `FrameBuffer`, glyphs, `marquee_row`, config-only knobs; **questions the two threads** | **Partly accepted, partly rejected.** The config/art cuts match the professor's answers → accepted. The threading cut → **rejected** (professor answer #3 requires two threads). The FSD/CI/contract cuts → **rejected**; already adjudicated twice, and Phase 0 verified them. |
| `gpt-v10.md` | Same, wider: also remove the injected clock, `SchedulerSnapshot`, `wakeTick_`, the concurrency-test machinery, macOS, FSD, `check_layers.sh`, `CONTRACTS.md`, the SHA-256 freeze, the harness; smaller PCB | **Mostly rejected.** Only the injected-clock and `FrameBuffer`/glyph items are adopted, and the clock only in the form that *keeps one clock source* (`Terminal::nowMs()`), not by adding `std::chrono` calls to scheduler logic. Everything else survives: see §5 "rejected, do not re-open". |
| `gpt-v11.md` | Written *after* the professor's answers: keep the two threads, keep Windows as the graded platform, keep the scheduler + interpreter, plain text marquee is enough; remove `.ini`, `quiz_case_*.ini`, `marquee_row`, the ASCII glyph system, and the machinery that existed only to support them | **Accepted — this is v3's scope**, subject to the operator's two carve-outs (§4 D3, D4). |
| `gpt-v12.md` | Don't run v2's manual T0.5 checks first; write v3 first; carry forward only the genuinely verified Phase 0 facts; after v3, the surviving manual check is the **Windows CLion Run/Debug path** | **Accepted verbatim.** T0.5 is restructured in v3: the scriptable half stays done, the live gate moves to Phase 2 where it can actually be observed. |
| `gpt-v13.md` | Reviews v3 itself: (1) `--no-tty` is a dev/CI mechanism, not a quiz parameter — separate the two surfaces; (2) the polling/refresh sweep needs an explicit **frozen-binary workflow** (change Program Arguments, press Run again, never rebuild); (3) don't invent non-ASCII normalization semantics — make the text contract **explicitly ASCII input** | **All three accepted.** D13, D14, D15. Point 3 was confirmed by the operator against the professor (*"Just ASCII is fine"*), which turns risk 29 from *undefined* into *defined and testable*. |

## 4. Decisions taken (dated, with authority and ratifier)

| # | Decision | Authority | Ratifier |
| --- | --- | --- | --- |
| D1 | **Remove the `.ini` config system** (`config/csopesy.ini`, `config_io.hpp/.cpp`, `test_config.cpp`, `--config`, `quiz_case_<n>.ini` convention, the copy-over runbook step) | Professor answer #1 (as recorded in `gpt-v11.md`) | W2 Byron (owns `config_io.*`, §4.5) |
| D2 | **Plain text scrolling marquee only** — delete the 5×5 glyph table, `ascii_art`, `--plain`/`--art`, `glyphs.hpp/.cpp`, `test_glyphs.cpp`. The handout says *"Text marquee **or** ASCII graphics marquee"* (verified verbatim in the PDF, p.2) so plain text satisfies the requirement | Professor answer #2 + handout p.2 | W4 Kim (owns glyphs, §4.5) |
| D3 | **Drop `marquee_row`**; the band is a fixed row near the top (row 3). | Professor answer #5 | W4 Kim |
| D4 | **Drop the automated measurement machinery** (`--measure`, `Parameters::measurePath`, `Scheduler::appendMeasure`, `pendingEventMs_`, `eventOwed_`, `scripts/measure.*`, the CSV). **Keep the PPT measurement requirement** — the refresh/polling recommended values and the tearing/typing-delay limits — as a **documented manual sweep** (§T1.4 rewritten). *Operator carve-out, 2026-09-22.* | Operator decision on `gpt-v10.md`'s cut | W4 Kim (measurement owner) |
| D5 | **Drop `--diag`.** Its two real jobs survive as (a) a **loud startup failure** if VT output cannot be enabled, and (b) the hand-run CLion gate in Phase 2. | Operator decision (chose the "also drop the v2 extras" option) | W2/W3 |
| D6 | **Drop `FrameBuffer` row-diffing.** `Renderer::buildFrame(...)` returns one complete frame string; the scheduler issues **one `write()` + one `flush()`**. Keeps the plan's narrow tearing claim (*no application-level interleaving*, not atomic terminal repaint) and deletes the "invalidation is not submission" defect class outright. | Operator decision + `gpt-v10.md` §5 | W4 Kim |
| D7 | **Drop the injected `Clock` functor.** `Terminal::nowMs()` — already in the frozen `Terminal` contract — is the single clock source, and `FakeTerminal::nowMs()` is how tests drive time. This removes v2's duplicate clock source and keeps `AGENTS.md` §5's invariant true; **no `std::chrono` call enters scheduler logic.** | Operator decision on `gpt-v10.md`, narrowed | W1 Lorens (scheduler) + W2 |
| D8 | **Keep the two-thread architecture** (one mutex, one condition variable, one terminal writer, `stop_`-only shutdown, `interrupt`/`quit_` owned by the input thread). | Professor answer #3; `gpt-v9`/`gpt-v10`'s cut **rejected** | W1 Lorens |
| D9 | **Keep the engineering infrastructure**: FSD layers + `scripts/check_layers.sh`, the `CONTRACTS.md` freeze protocol, the 3-OS CI matrix, macOS as a non-blocking second opinion, the zero-dependency harness, `--no-tty` + the real-binary smoke test. | Operator decision; `REVIEW_ADJUDICATION.md` §3 pt 15, §6.4, §10.4, §11.6 | W2 Byron |
| D10 | **The quiz drives all six handout commands, and the parameter surface is those six plus `--refresh-ms=N` and `--poll-ms=N`** — *amended 2026-09-22 (D13), clarified 2026-09-22 per `gpt-v14.md`*. `help` and `exit` are **required quiz commands too** — they are commands rather than *parameter-setting* commands, but nothing here makes them optional (A1/A6, §7). The four that set values (`set_text`, `set_speed`, `start_marquee`, `stop_marquee`) plus the two numeric flags are what a test case's values travel through. No config file, no case files. The graded run config passes **no arguments** by default; the two numeric flags are set only for a *measurement take* (D14). | Operator decision (option 2) + professor answer #1 | W2 Byron |
| D11 | Keep `docs/IMPLEMENTATION_PLAN_v2.md` in place, **marked superseded**, not deleted or archived. | Operator decision | W1 Lorens |
| D12 | v3 is a **self-contained** replacement plan, not a delta patch against v2. *Amended 2026-09-22 (D16):* self-contained in its decisions, tasks, acceptance cases and risks; §3 is a **delta contract** that writes out every changed contract in full and incorporates the unchanged frozen text by version-pinned reference. | Operator decision ("place the changes into a new implementation-v3 markdown") | W1 Lorens |
| D13 | **`--no-tty` is a dev/CI flag only.** It exists so the real-binary smoke test is portable across the three CI images. It is **not** part of the quiz parameter surface and never appears in the graded run (the graded press runs on a real console). `--refresh-ms` / `--poll-ms` are the quiz-facing flags. | `gpt-v13.md` pt 1 | W2 Byron |
| D14 | **The refresh/polling sweep has a concrete frozen-binary workflow** (no source edit, no rebuild): for each take, set `--poll-ms=N --refresh-ms=M` in the run configuration's *Program arguments*, press Run/Debug on `csopesy-quiz`, observe, record, then change **only the arguments** and repeat. Written into §T1.4 and §7 of v3. | `gpt-v13.md` pt 2 | W4 Kim (measurement) + W3 Nathan (Windows takes) |
| D15 | **The text contract is explicitly ASCII input.** After §3.7's trim + internal-run preservation, every byte of a `set_text` argument must be printable ASCII `0x20`–`0x7E`; otherwise the command is **rejected with a message and the marquee text is unchanged**. **No normalization is invented** — the fix for byte-slicing is the input contract, not a mangling step, so the user's requested text is never silently altered. Operator: *"Just ASCII is fine according to my professor."* | `gpt-v13.md` pt 3 + operator confirmation (professor) | W4 Kim (PP: renderer) + W2 |
| D16 | **v3 §3 is a delta contract, not a transcription.** Changed contracts (`parameters.hpp`, `cli.hpp`, `renderer.hpp`, `scheduler.hpp`'s deltas) are written out in full; unchanged frozen text (`terminal.hpp`, `keys.hpp`, `shutdown.hpp`, `process.hpp`, the §3.8 threading contract, §3.11) is incorporated **by reference to v2.6, which is frozen and never edited**. Reason: duplicating ~500 lines of seven-round-adjudicated prose is a silent-drift risk, and the freeze makes the reference stable. | This session | W1 Lorens |
| D17 | **The deadline moved to Monday 2026-09-28, so the §4.5 protocol is honored in full and no header changes before ratification.** The earlier recommendation to apply the v3.0 headers "ahead of ratification" was justified *only* by T-1 schedule pressure; that justification is gone and the option is **withdrawn**. Sequence: correct the stale dates → finish §5 → ratify with W2 + affected owners → apply the headers → implement → verification/PPT/runbook. | Operator instruction, 2026-09-22 | Operator; §4.5 sign-off by W2 Byron |
| D18 | **Contracts re-frozen as v3.0 — ratified and LANDED.** The header set went 13 → 11 (`cli.hpp` added; `config_io.hpp`, `glyphs.hpp`, `frame_buffer.hpp` deleted) and the four contract removals (`asciiArt`, `marqueeRow`, `measurePath`, the injected `Clock`) plus `TextResult` were applied in §3.12's order, as commit **`d6379df`**. Guard `OK (22 files scanned)` (the predicted count), `ctest` 1/1, 11/11 hashes matching — including the 7 untouched headers still carrying their v2.6 blob hashes. | v3 §3.12 + task **T0.6** Step 2; **W2 Byron (the §4.5 freeze owner, A/R) agreed 2026-09-22** — *"he agrees with all of it"*, relayed by the operator | W2 Byron (ratifier). Operator/W1 concurs as the driver. **Provenance gap, recorded not glossed:** W3's and W4's individual acknowledgment is **not in this repo**. §4.5's gate is the freeze owner's sign-off, and the blast radius is contained — every `src/*.cpp` is still a `TODO` stub, so a single revert of `d6379df` restores v2.6 cleanly if either objects. |

## 5. Rejected, do not re-open without new primary evidence

`gpt-v9.md` and `gpt-v10.md` recommend cutting these. Each was already adjudicated, and the operator kept it.
Recording the reasons here so a future session does not "simplify" them again.

| Thing | Kept because |
| --- | --- |
| Two threads / mutex / condition variable | The professor **asked for it** (answer #3). Not a reviewer's call. |
| FSD layers + `scripts/check_layers.sh` | `AGENTS.md` §3 mandates it; T0.1's evidence exists; the guard is 100 lines. The reviewer's objection is to the *cost of enforcement*, not to the layer facts. |
| `CONTRACTS.md` freeze + hash table | T0.4's evidence; it caught a mutation test. Deleting it removes the only defence against mid-phase header churn on four machines. |
| 3-OS CI matrix | T0.3's evidence (two green runs). It is the only thing that compiles `terminal_win32.cpp` for the members who cannot. |
| macOS verification | Cost is one CI job that already exists; it is explicitly non-blocking (W4, P2). |
| Zero-dep test harness (`tests/support/check.hpp`) | `AGENTS.md` §6: no framework, no network, works on all three CI images. |
| `--no-tty` plain line mode | It is what makes the real-binary smoke test portable across the three CI OSes (adjudication R4). |
| `SchedulerSnapshot`, `wakeTick_`, the concurrency tests | Adjudicated and kept (adjudication §10.3, §11). `--diag`'s removal does not change `snapshot()`'s other justification: it is the only race-free read of live scheduler state for tests. |
| `MarqueeProcess` as a small PCB | 17 lines, already minimal; "process representation" is a graded PPT topic. |
| The instruction-level verbatim text of the unchanged contracts (v2.6 §3.3/§3.8/§3.10/§3.11) | D16: they are frozen and v2 is immutable, so referencing them cannot drift. Rewriting them by hand can. |

## 6. Findings worth keeping (verified this session, 2026-09-22)

- **Handout quotes verified verbatim** from `docs/MO3 - Marquee_Operator.pdf` with `pdftotext -layout`:
  *"1. Text marquee or ASCII graphics marquee"* (p.2) and *"You should only modify the parameters and no longer
  recompile the project when taking the quiz."* (p.2, ASSESSMENT METHOD). D2 rests on the first.
- **`csopesy-quiz`'s type id is no longer unverified-by-inspection.** The committed
  `.idea/runConfigurations/csopesy-quiz.xml` contains `type="CustomBuildApplication"` — the id handoff §5.3
  could not confirm because CLion's distribution is compressed. What remains unverified is only whether CLion
  **loads** it (a hand check on a real CLion, moved to Phase 2).
- **`config/quiz_case_<n>.ini` never existed in the repo** — the convention lived only in v2 §3.6/§7. Removing
  it costs no files, only prose.
- **`src/**` was all `TODO` stubs at this point** (230 lines; headers 303 lines) — T1.1 replaced `terminal_posix.cpp`'s stub on 2026-09-22 (§7). This was the cheapest possible
  moment to change the frozen contracts — nothing depends on them yet. This is the strongest argument for
  doing v3 **now** rather than trimming v2.6 in place.
- **Provenance gap, open:** the professor's five answers exist in this repo only as `gpt-v11.md`'s paraphrase.
  The raw message is not committed (and `docs/replies/` is gitignored, so it would not be public anyway).
  **Action:** the operator should paste the raw professor reply into `docs/replies/professor-answers.md` and
  note the date. Until then v3 cites them as *"professor's answers, as recorded in `gpt-v11.md` (2026-09-22)"*.
- **`--diag`'s removal has a real cost** and it is being paid knowingly: risk 3's "VT could not be enabled"
  becomes a hard startup error rather than a reported field, and the runbook's pre-flight sanity check loses
  its one non-visual step. Recorded so nobody later thinks a diagnostic was forgotten.
- **The ASCII decision retires risk 29's ambiguity.** v3's first draft invented a *normalization* step
  (non-printable bytes → `?`), which would silently alter the text the operator asked for. With the professor's
  "just ASCII is fine" (via the operator, 2026-09-22), the contract instead **rejects** out-of-range input with
  a message. Same safety for the scroll math (one byte = one column), no invented semantics, and it is testable.
- **T1.2's `hasRendered` transition is split by ownership, not by oversight.** Frozen `process.hpp` says `hasRendered` *"false => the next tick draws immediately (fresh OR restarted)"*, so the PCB owns the field and `start()` clearing it, while the **false → true** transition is the *scheduler's* render path (§3.8). v2 §T1.2 Step 1 lists "becomes `true` after the first render" in the same test list, but no unit test can reach it before `Scheduler::tick` exists — T1.2's 7 tests pin the half the PCB owns (`start()` clears it; `stop()` alone does not) and the rest arrives with T1.3. Recorded so a reviewer diffing T1.2's suite against v2 §T1.2 does not read the absent assertion as a dropped requirement, and so nobody fakes a render inside `process.cpp` to "complete" it.
- **`sliceRow`'s width invariant is only *sound* because of D15.** With arbitrary UTF-8 accepted, slicing by
  bytes can split a character and the "one byte = one column" premise fails. ASCII-only is therefore a
  correctness contract, not a validation nicety — do not relax it without changing the renderer's width model.
- **The deadline moved to Monday 2026-09-28 (operator-stated, 2026-09-22).** This removes the only argument for
  bypassing §4.5 (D17) and moves the plan's prose back onto the critical path. Every 2026-09-23 date in these two
  documents was corrected in the same pass. **Stale dates deliberately left alone:**
  `docs/IMPLEMENTATION_PLAN_v2.md` line 117 and its §5.0 schedule still say Wed 2026-09-23, and `AGENTS.md` line 4
  did too. v2 is **frozen text** (D11/D16 depend on that), so its dates are superseded-by-banner at S5 rather
  than edited; `AGENTS.md` is not frozen and its date was corrected in place on 2026-09-22.
- **Deadline provenance is operator-supplied, not an in-repo artifact.** The new date comes from the operator
  (`gpt-v14`-style instruction, 2026-09-22). No class announcement, email or handout revision is committed, and
  `docs/replies/` is gitignored. **Recorded as a provenance gap alongside the professor's five answers (§6
  above):** if the professor's extension notice can be pasted into `docs/replies/`, cite it here; until then the
  plan states the date as *"operator-stated 2026-09-22"* so a future reader can weigh it.
- **`help` and `exit` are required quiz commands, not optional extras** (`gpt-v14`'s watch item, applied). D10's
  first wording said "the four required commands + two flags", which is wrong as a *requirement* statement: the
  handout requires **six**, and only four of them are *parameter-setting*. v3 §6.2 now leads with the mapping
  (A1 = `help`, A2/A3 = start/stop, A4 = `set_text`, A5 = `set_speed`, A6 = `exit`) and §7's runbook says
  outright that any of the six is legitimate in a take. Recorded here because a wording slip in a decision log is
  exactly how a requirement gets quietly dropped later.
- **The v3 plan is now self-sufficient for implementation.** §0–§10 are complete: no "TBD" anywhere. After
  T0.6 (below) the only unchecked box left in `IMPLEMENTATION_PLAN_v3.md` is T0.6 **Step 6**, which a human must
  send — **it was sent on 2026-09-22 (§11), so T0.6 is now fully closed.** Unchanged task *bodies* still live in v2 and are named normative in place (v3 §10.4 deviation 2), so
  nobody has to guess which document to read for `T1.1`, `T2.5`, `T3.1`, `T3.4` or `T0.1`–`T0.5b`.
- **The predicted guard count was confirmed, not adjusted.** v3 §10.3 predicted `OK (22 files scanned)` for T0.6
  **before** the change was made (26 − 3 headers − 3 `.cpp` + `cli.hpp` + `cli.cpp`), and the finished tree prints
  exactly 22. A count that is merely *observed* and then written down proves nothing; a number predicted in
  advance and then matched is a real check that no file was missed on either side.
- **The v3.0 delta is verifiable from the freeze table alone.** **7 of the 11 headers still carry their exact
  v2.6 `git hash-object` values** (`console_app`, `interpreter`, `keys`, `line_editor`, `process`, `shutdown`,
  `terminal`). A reader who distrusts the commit message can diff the two tables and see that the revision is
  precisely the four headers it claims — the hash table turns "trust me" into "check me". This is the strongest
  argument for keeping `CONTRACTS.md` that this session produced.
- **A tooling false positive, permanently fixed — and it was not a contract problem.** Analyzer/LSP runs on
  `tests/unit/*.cpp` reported `check.hpp` as unresolved (`'check.hpp' file not found`), because the real
  `-I tests/support` comes from a per-target `target_include_directories` that a blind heuristic cannot see. With
  `build/debug/compile_commands.json` present the same four files report **0** diagnostics. The `debug` configure
  preset now exports it (**`cc74d5d`**), so the false blocker stops recurring for every future session and every
  teammate. Recorded because the tempting "fix" was to edit the test files, which was never the actual problem.
- **The false positive re-confirmed on a T1.3 file, and resisted a second time.** The same heuristic flagged
  `tests/support/fake_terminal.hpp` (`'csopesy/terminal.hpp' file not found`, `only virtual member functions can
  be marked 'override'`) with "quick fixes" that would have deleted the include and stripped every `override` —
  i.e. made the file uncompilable. Two authoritative checks say it is clean: `-Wall -Wextra -Werror` on the TU
  that includes it, and `lens_diagnostics source=lsp` over the three changed files (**0 diagnostics**, 3/3
  clean). Provenance of the false positive is the per-target `-I tests/support` / `-Iinclude` that the runtime
  heuristic does not read. **Do not act on it.**
- **T1.3's test suite is deliberately scheduler-only, and the deferred assertions are enumerated rather than
  faked.** `Renderer::buildFrame` (T4.2, W4) and `Interpreter::feed` (T2.4, W2) were still stubs at T1.3 time —
  both landed 2026-09-24 — so then **no `Terminal::write()` ever happened** (`tick` keeps the
  v2-normative `if (!frame.empty())` guard, and in v3 `buildFrame` is contracted to return a whole frame, so
  the guard is production-irrelevant). Everything observable through scheduler-owned state is pinned now:
  `cycles`, `hasRendered`/`lastRenderMs`, `snapshot()`, `pollTimeoutMs()`, `TickResult`, `stop_` + `join`, and
  the `size()`-call tick barrier. **Owed to T4.2/T2.4 — now unblocked (both landed 2026-09-24); do not re-derive
  these from the v2/v3 prose:**
  (a) `frameCount()`/`writesCopy()`/`writersCopy()` assertions — "only the marquee thread ever writes" and
  "every `write()` carries exactly one whole frame" — need T4.2 to emit a real frame;
  (b) "echo redraw is not gated by `refreshMs`" is only observable via a write, so it needs T4.2;
  (c) every command-effect test typed through `postEvent` (`stop_marquee` → `Stopped`, `set_text` → `params.text`,
  `set_speed` → next deadline, `exit` → `quitRequested`) needs T2.4.
- **The plan's own `waitForTickStarted(n)` pattern is racy, and T1.3 found it by running the suite 40×.**
  `tick()` calls `Terminal::size()` *before* it takes `mu_` (§3.8 rule 3), so "tick n started" does **not** mean
  tick n rendered — asserting about tick n's own effect at that moment races it. The correct barrier is
  **completion**, and it is free: the worker steps strictly in sequence, so **tick n+1 starting proves tick n's
  body returned**. Both v2 §T1.3's literal snippets (`waitForTickStarted(1)` then `CHECK_EQ(cycles, 1)`) have
  this latent race. Observed **1 failure in 40 runs**; after switching to the relative
  `t = tickCount(); waitForTickStarted(t + 2)` form, **150/150 runs pass**. The `t + 2` (not `t + 1`) is also
  load-bearing: the stop/store is sequenced before the `tickCount()` read, so tick `t+1` is the first one
  guaranteed to load the new clock, and `t+2` is what proves it finished.
- **The handoff prompt's "size change repaints without waiting for refresh" contradicts v3 §3.8, and §3.8 wins.**
  v3 §3.8's delta table removes `fbRows_`/`fbCols_` and states *"Resize needs no state any more"*; detecting a
  resize without remembering the last size is impossible, so "repaint on size change" and "no size state" cannot
  both hold. Operator decision 2026-09-22: **follow §3.8** — a resize is picked up by the **next rendered**
  frame (`buildFrame` is handed the `Terminal::size()` read in `tick()`), so the layout is correct within
  `refreshMs`, not within `pollingMs`. That one test is deliberately **not** in the suite, and no size field was
  added. (`A11`'s tiny-terminal case still holds: it is a layout case, not a repaint-latency case.)
- **T1.3's `set_speed` test drives the field directly, on purpose.** `Parameters::setRefresh` (T2.1, W2) is still
  a stub returning an empty `ClampReport` without applying the value, so a pure-step test that called it would
  have been testing W2's task, not the scheduler's deadline math. With no worker running there is no sharing, so
  `params.refreshMs = 50` is race-free and isolates exactly the scheduler property under test. The
  command-path half (`set_speed 50` → `setRefresh` → the next deadline) is covered by T2.1/T2.4 — see the
  deferred list above.
- **T1.3 Step 4's grep only passes if the file's prose avoids the token it greps for.** `grep -n 'sleep_'`
  matched the header comment that *declared* the file free of `sleep_for`/`sleep_until`. The requirement is that
  the grep prints **nothing**, so the comment now says "no real-time sleep primitive" instead. Worth knowing:
  this is a literal-text check, and a well-meaning explanatory comment can fail it.
- **Sanitizers are not runnable on this machine (plan T1.3 Step 5 is explicitly local/optional).** `-fsanitize=thread`
  fails to link (`cannot find /usr/lib64/libtsan.so.2.0.0`) and so do `address` and `undefined`
  (`libasan.so.8.0.0`, `libubsan.so.1.0.0`) — the system GCC 16.2.1 has no sanitizer runtimes installed. The
  only copies on disk are inside Flatpak runtimes (`org.gnome.Platform` etc.), which are a different libc and
  must not be mixed in. **No TSan/ASan evidence is claimed for T1.3**, and per the plan nothing was wired into
  CI. The determinism evidence is the 150/150 clean runs plus the design's single-owner rules.
- **`postEvent`'s wake form was reconciled to `notify_all` — the two sources of truth disagreed about it.** v2
  §3.8's normative code block writes `cv_.notify_one()` for `postEvent`, while v3 §3.8's delta row says
  `cv_.notify_all()`. A delta row is *by definition* the changed v3 behaviour, so the delta row wins and the
  implementation now matches it. The change is behaviourally free (with exactly one waiter, `notify_all` and
  `notify_one` are indistinguishable) and it makes `postEvent` consistent with `requestStop`. Recorded so a later
  "cleanup" back to `notify_one` does not silently re-open the mismatch — **`wake()` stays `notify_one`**, because
  no delta list changes it.
- **The `FakeTerminal` lock-order comment was factually wrong, and v2 §3.8 is where the wrong claim came from.**
  It read *"the worker takes `mu_` and then `m_` (tick -> size -> write)"*. It does not: `Terminal::size()` is
  read **before** `mu_` is taken (§3.8 rule 3) and `write()`/`flush()` happen **after** it is released (rule 2),
  so the two locks are never held simultaneously and there is no `mu_ -> m_` order to state. This outlives the
  comment: the identical sentence sits in v2 §3.8's `FakeTerminal` snippet, so anyone copying that snippet
  inherits a false invariant. The comment now describes what the code does; **no lock rule changed.**
- **Three stale claims were fixed in the non-frozen docs (the v3 plan and `AGENTS.md`).**
  `IMPLEMENTATION_PLAN_v3.md` still called itself `v3.0-draft`, and its §0 table still said *"Until S2 lands, the
  headers on disk are **v2.6**, not v3"* and *"§5 (task backlog) — arrives in S3"* — all three falsified by T0.6
  (`d6379df`). `AGENTS.md` §1 said the acceptance cases are **A1–A10** while v3 §6.2 has **A1–A12**. Fixed in
  place, because neither file is frozen. **Deliberately not touched:** v2's own `A1–A10` prose (lines 2949/2981/
  3048) and its stale 2026-09-23 dates — v2 is frozen text (D11/D16), superseded by banner rather than by edit.
- **A review's two style claims were checked and did not hold; recorded so nobody "fixes" them later.** The claim
  that source comments violate `AGENTS.md` §9 by referencing `§3.8` and task ids is wrong — that is established
  house style here (`tests/unit/test_process.cpp` opens with "T1.2"; `src/platform/terminal_posix.cpp` cites
  "§3.4, §T1.1"), and §9 forbids *plan history, review history, agent reasoning, D-numbers and rejected designs*,
  not references to the normative spec. The claim that the comments narrate "the v2 plan" is also wrong:
  `grep -n v2` finds **no** mention in `scheduler.cpp`, `test_scheduler.cpp` or `fake_terminal.hpp`. What the
  review got right, and what was fixed: the test header narrated *process* ("T1.3 Step 4 greps this file",
  "§6.1 DoD 2", "so they are owed, not forgotten") — exactly the plan-process prose §9 forbids.
- **A second instance of the barrier race — found by CI, at the HEAD of the test, after the 150-run loop missed
  it.** The cleanup commit `d3e3794` (docs only) failed on `windows-latest` with `FAIL
  tests/unit/test_scheduler.cpp:211  h.sched.snapshot().cycles == 4`, while the identical *code* (`572eddc`) had
  been green on Windows minutes earlier — the definition of a flake. It is the same defect class as the recorded
  `waitForTickStarted` finding above, but in the one place that was not converted: the **head** of
  `threaded_marquee_keeps_animating_across_refresh_deadlines` still used `waitForTickStarted(1, …)`, which is a
  *start* barrier — "tick 1 has begun", nothing completed.
  **Root cause, reproduced on demand rather than by timing luck:** `tick()` calls `size()` (the barrier) and only
  *then* reads `term_.nowMs()` under `mu_` (§3.8 rule 3), so a clock store landing in that gap is read by tick 1.
  The first frame is immediate (`!hasRendered`), so tick 1 stamps `lastRenderMs = 100`, moving every deadline to
  200/300/400 — iteration 1's wake at clock 100 is then *not* due (`100 - 100 < 100`), one deadline frame is lost,
  and the suite sees **3** where the test asserts 4. Demonstrated deterministically with a scratch-only
  `FakeTerminal` subclass whose `size()` stores the clock on its first call, forcing the store into exactly the
  window the start barrier permits: `cycles == 4` failed and `lastRenderMs` was 100, not 0.
  **Fix:** the head is now a completion barrier (`waitForTickStarted(2, …)`) plus the precondition it actually
  depends on, checked explicitly — `cycles == 1` **and** `lastRenderMs == 0` — so a regression fails at the
  precondition instead of confusingly in the final count. Verified non-vacuous: under the forced racing double,
  `lastRenderMs == 0` fails. **200/200 clean runs** after the fix.
  **Generalised rule:** any assertion that depends on a *rendered* frame needs the **completion** barrier
  (`waitForTickStarted(n + 1)`, or `tickCount()` then `+ 2`); `waitForTickStarted(1, …)` is sound only where the
  test asserts nothing about a render — it is deliberately kept in `request_stop_…`, where demanding a completion
  barrier would instead force a real 1 s poll wait.
- **The inline analyzer was also observed being wrong in the opposite direction: a false negative.** While
  writing the scratch reproduction it reported `✓ C/C++ clean` on a file gcc refused to compile (`passing 'const
  std::atomic<long long>' as 'this' argument discards qualifiers`). The same heuristic that invents 22 errors on a
  clean file missed a real one on the next edit — the strongest available argument that the real build plus
  `lens_diagnostics source=lsp` are the only authorities, and that its "quick fixes" must never be applied.

## 7. Session log (newest last)

| Date | Stage | What changed | Artifact |
| --- | --- | --- | --- |
| 2026-09-22 | — | Read `gpt-v9`…`gpt-v12` in order; read `handoff-v1.md`, v2 §1/§3.5–3.11/§4/§6–§8/§9, `REVIEW_ADJUDICATION.md` (all 7 rounds' headers + every `.ini`/`--measure`/`--diag`/glyph hit), `CONTRACTS.md`, `check_layers.sh`, both run configs, `ci.yml`, `CMakePresets.json`, the handout text. Operator answered 4 scoping questions (D3, D4, D10, D11, D12). | — |
| 2026-09-22 | **S1** | Wrote this file; wrote `IMPLEMENTATION_PLAN_v3.md` §0–§2 + §3.0 (change inventory + proposed contract deltas). No header, source, config or CI file touched yet. | `PLAN_V3_PROGRESS.md`, `IMPLEMENTATION_PLAN_v3.md` |
| 2026-09-22 | **S1.5** | Absorbed `gpt-v13.md`: split the dev/CI flag surface from the quiz surface (D13), wrote the frozen-binary measurement workflow (D14), replaced invented ASCII normalization with an explicit ASCII-input contract confirmed against the professor (D15), adopted the delta-contract structure for §3 (D16, amending D12), and re-prioritized the stages against the deadline as it was then understood, T-1 (§9, **superseded by S2.5**). No header, source, config or CI file touched yet. | `PLAN_V3_PROGRESS.md`, `IMPLEMENTATION_PLAN_v3.md` |
| 2026-09-22 | **S2 (plan text)** | Wrote `IMPLEMENTATION_PLAN_v3.md` §3.1–§3.12: the delta contract (`Parameters` v3, `cli.hpp`, the command table + the ASCII rule, the plain-text renderer + layout + the tight-terminal priority rule, the scheduler delta list, and the v3.0 freeze checklist). Headers still v2.6. | `IMPLEMENTATION_PLAN_v3.md` |
| 2026-09-22 | **S4** | `gpt-v14` absorbed. Wrote `IMPLEMENTATION_PLAN_v3.md` §6 (DoD + **A1–A12**, incl. the new tiny-terminal and plain-line cases and the six-command mapping), §7 (runbook: no case file, no `--diag`, Windows-first, all six commands legitimate, no-rebuild pre-flight), §8 (24 live risks + 6 closed with reasons + new 29/30), §9 (the five answers recorded, with the two provenance gaps), §10 (self-review, name-consistency check, the v2.6→v3.0 revision log, review dispositions). Fixed D10's "four required commands" wording — the handout requires **six**. | `IMPLEMENTATION_PLAN_v3.md`, `PLAN_V3_PROGRESS.md` |
| 2026-09-22 | **S4.1** | Fixed the **last** ASCII contradiction: `IMPLEMENTATION_PLAN_v3.md` §1.4 still said §3.7 *"normalizes non-printable and non-ASCII bytes"*, which contradicted D15 (*rejects*). Caught by `gpt-v15`; now reads *rejects*. Then committed the three documentation changes as **`f2e2147`** (`docs(plan): v3 plan — plain-text marquee, CLI-only parameters, Sep-28 deadline`) and verified the working tree is clean. **No header, source, config, test or run-config file is touched by that commit.** | `AGENTS.md`, `IMPLEMENTATION_PLAN_v3.md`, `PLAN_V3_PROGRESS.md` |
| 2026-09-22 | **S2 (bytes) = T0.6** | **W2 Byron (the §4.5 freeze owner) ratified the v3.0 contract change**, so T0.6's seven steps were executed in §3.12's order: 4 header rewrites + `cli.hpp`, 3 header deletions, the guard's `layer_of_header()` map, `CONTRACTS.md`'s v3.0 marker + regenerated hash table, both run configs' `PROGRAM_PARAMS` cleared, `config/csopesy.ini` deleted, the 6 source/test deletions, both CMake target lists, and the stub doc-comments that named the deleted work. Verified: guard `check_layers: OK (22 files scanned)` — the prediction in v3 §10.3, made before the change, **confirmed exactly**; `ctest` 1/1; **11/11** hashes matching, with the 7 untouched headers still on their v2.6 blobs. Landed as **`d6379df`** (30 files) and pushed, with the `compile_commands.json` preset export deliberately kept out as **`cc74d5d`**. Recorded as **D18**. **CI is green on all three OSes** (ubuntu 17 s, macOS 26 s, Windows 45 s) in [run 35724109198](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/35724109198), so `AGENTS.md` §8 item 2 is satisfied. **Still open: T0.6 Step 6 only — the group announcement (§11).** | `include/csopesy/*.hpp`, `scripts/check_layers.sh`, `CONTRACTS.md`, `CMakeLists.txt`, `.idea/runConfigurations/*.xml`, `src/**`, `tests/unit/**`, `AGENTS.md`, `IMPLEMENTATION_PLAN_v3.md`, `clion-run-config.md` |
| 2026-09-22 | **S6 / P1 — T1.1** | **Comment-quality cleanup pass** over the non-frozen files (`src/**`, `tests/**`, `scripts/check_layers.sh`, `ci.yml`): removed plan/review-history narration, D-number tags and long block comments, kept every `TODO(task)` marker and every real invariant; added **`AGENTS.md` §9 (Comment style)** and corrected §4's stale `(v2.6, 2026-09-16)` freeze marker to v3.0. Proved comment-only by a strip-and-compare pass over every changed source file, plus `ctest` 1/1 and `check_layers: OK (22 files scanned)`; **no frozen header touched** (11/11 hashes still match `CONTRACTS.md`). Then **T1.1 `PosixTerminal`**: raw mode (`ICANON`/`ECHO`/`ISIG`/`OPOST`/`ICRNL` cleared, `VMIN=VTIME=0`), idempotent `restore()`, live `TIOCGWINSZ` sizing with the 24x80 fallback, poll-based `readEvent` (CSI+SS3 arrows, bare-Esc 5 ms deadline, Enter/Backspace/Tab/Char, Ctrl+C and Ctrl+D → `Eof`), CRLF-preserving `write()`, `flush()`, `isTty()`, monotonic `nowMs()`, and flag-only SIGINT/SIGTERM handlers with no `SA_RESTART`. **33/33 pty checks pass**, observed from the parent side of a real pty (exact termios flags, live resize, full restore equality, idempotency, a 0.1 ms EINTR wake against a 2000 ms timeout), plus the non-tty path. **Still open:** the app-level hand check is blocked by `main`'s stub (T2.5). | `src/platform/terminal_posix.cpp`, `AGENTS.md`, `scripts/check_layers.sh`, `.github/workflows/ci.yml`, other non-frozen `src/**` + `tests/**` |
| 2026-09-22 | **S6 / P1 — T1.2** | **T1.2 `MarqueeProcess` PCB** implemented in `src/entities/process.cpp`: `start()` returns `false` when already `Running` and otherwise sets `Running` + clears `hasRendered`; `stop()` returns `false` when already `Stopped` and otherwise sets `Stopped`. New `tests/unit/test_process.cpp` (7 tests: the contract defaults — `pid 1`, `name "marquee"`, `Stopped`, `cycles 0`, `lastRenderMs 0`, `hasRendered false` — both transitions, both no-op `false` returns, `start()` clearing `hasRendered`, and `stop()` *not* clearing it), registered in `CMakeLists.txt`. **TDD evidence: 14 assertions failed against the stub** (`FAILED  14 assertion(s)`, `0% tests passed`), then `OK 7 tests` / `ctest` 100% 1/1 after the implementation; clean rebuild is warning-free under `-Wall -Wextra`. Guard `OK (22 files scanned)`; **11/11 frozen hashes still match `CONTRACTS.md`** — no header touched. The `hasRendered` false → true transition is **deliberately not** in this task: `Scheduler` owns it (§3.8), so it lands with T1.3 (§6). Landed as **`ed18908`** and pushed; **CI [run 35732133066](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/35732133066) is green on all three OSes** (ubuntu-latest 20 s, macos-latest 18 s, windows-latest 1 m 14 s), so `AGENTS.md` §8 item 2 holds for this task too. | `src/entities/process.cpp`, `tests/unit/test_process.cpp`, `CMakeLists.txt`, `PLAN_V3_PROGRESS.md` |
| 2026-09-22 | **S6 / P1 — T1.3** | **T1.3 the threaded scheduler** implemented in `src/app/scheduler.cpp` (the only non-test `src` change): `start`/`run` (bounded `cv_.wait_for`, `stop_` the sole exit signal), the input-thread API (`postEvent`/`wake`/`requestStop`/`join`/`joinable`), the preserved pure step `tick(ev)`, `pollTimeoutMs`/`snapshot`, and the three lock rules — `size()` read before `mu_`, the frame assembled under `mu_` and written after it, `tick()` never called holding `mu_`. `buildFrame` + one `write()` + one `flush()` replaces the v2 `FrameBuffer`; `term_.nowMs()` replaces the injected `Clock` (`<chrono>` is only the wait duration type); no `--measure`/`pendingEventMs_`/`eventOwed_` reappeared. New `tests/support/fake_terminal.hpp` (thread-aware double: `size()`-count tick barrier, cv waits, `outCopy`/`frameCount`/`writesCopy`/`writersCopy`/`push`) and `tests/unit/test_scheduler.cpp` — 16 tests. **TDD evidence: 53 assertions failed against the stub** (`FAILED  53 assertion(s)`), then **`OK 23 tests`** / `ctest` 100% 1/1. `grep -n 'sleep_' tests/unit/test_scheduler.cpp` prints **nothing**; guard `OK (22 files scanned)`; **11/11 frozen hashes still match `CONTRACTS.md`**; clean rebuild warning-free under `-Wall -Wextra`. **A latent flake in the plan's own barrier pattern was found and fixed** (1 failure in 40 runs → **150/150 clean**) — see §6. `docs/threading-model.md` written (T1.3 Step 6, the PPT's source of truth). **The test suite is deliberately scheduler-only**; the assertions owed to T4.2/T2.4 are enumerated in §6. Sanitizers unavailable on this machine (§6). Landed as **`95b0b61`** and pushed; **CI [run 35735238660](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/35735238660) green on all three OSes** (ubuntu-latest 53 s, macos-latest 49 s, windows-latest 47 s — `100% tests passed` on each, so the threaded suite really executed on Windows and macOS, not just compiled). | `src/app/scheduler.cpp`, `tests/support/fake_terminal.hpp`, `tests/unit/test_scheduler.cpp`, `docs/threading-model.md`, `PLAN_V3_PROGRESS.md` |
| 2026-09-22 | **S6 / P1 — T1.1 CI** | Pushed the cleanup pass (`1b600c7`) and T1.1 (`212db5b`). **CI [run 35728749335](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/35728749335) failed on `macos-latest` only** — `::sigemptyset(...)` does not compile there because macOS defines `sigemptyset` as a macro (`(*(set) = 0, 0)`), while ubuntu and windows passed. That is exactly the platform split the 3-OS matrix exists to catch (D9), and it is the first defect T1.1's Linux-only hand check could not have found. Fixed as **`242825b`** (unqualified `sigemptyset`, with a comment so a future cleanup does not re-add `::`); re-run [35728920428](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/35728920428) is **green on all three OSes** (ubuntu, windows, macos). | `src/platform/terminal_posix.cpp` |
| 2026-09-22 | **S6 / P1 — cleanup** | **Review-driven cleanup; the only executable change is one wake form.** `postEvent` now uses `cv_.notify_all()` to match v3 §3.8's delta row (v2 §3.8's code block says `notify_one`; the delta row is the changed behaviour, and with one waiter the two are indistinguishable — §6). Fixed the factually wrong `FakeTerminal` lock-order comment and trimmed plan-process narration out of `test_scheduler.cpp`'s header per `AGENTS.md` §9. Fixed three stale doc claims: the v3 plan's `v3.0-draft` marker, its §0 *"Until S2 lands … v2.6"* cell and its *"§5 arrives in S3"* cell, and `AGENTS.md`'s acceptance range (**A1–A12**). Verified: `-Wall -Wextra -Werror` clean on the edited TU, `ctest` 1/1 (`OK 23 tests`), `grep -n 'sleep_' tests/unit/test_scheduler.cpp` prints **nothing**, guard `OK (22 files scanned)`, **11/11 frozen hashes still match `CONTRACTS.md`** — no header touched. **Findings that must not be re-derived are in §6.** Landed as **`572eddc`** and pushed; **CI [run 35737493061](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/35737493061) is green on all three OSes** (ubuntu-latest 29 s, macos-latest 34 s, windows-latest 43 s — `100% tests passed` and `check_layers: OK (22 files scanned)` on each, so the threaded suite really executed on Windows and macOS, not just compiled). | `src/app/scheduler.cpp`, `tests/support/fake_terminal.hpp`, `tests/unit/test_scheduler.cpp`, `docs/IMPLEMENTATION_PLAN_v3.md`, `AGENTS.md`, `PLAN_V3_PROGRESS.md` |
| 2026-09-22 | **S6 / P1 — T1.3 flake fix** | **Second instance of the barrier race — found by CI, at the head of the frame-counting test, after the 150-run loop missed it.** Docs-only `d3e3794` failed on `windows-latest` (`FAIL tests/unit/test_scheduler.cpp:211 cycles == 4`) while the identical code (`572eddc`) had been green on Windows minutes earlier. Root cause: the head waited on `waitForTickStarted(1, …)`, a *start* barrier, so the loop's first clock store could land between tick 1's `size()` and its `nowMs()` read (§3.8 rule 3) and stamp the immediate frame at 100 — every deadline one period late, one frame lost (3, not 4). Reproduced on demand with a scratch-only racing double (not committed); fix is a completion barrier (`waitForTickStarted(2, …)`) plus the checked precondition `cycles == 1` and `lastRenderMs == 0`, verified non-vacuous and **200/200 clean**. No production code changed (`tick()` untouched); findings and the generalised rule in §6. Landed as **`d6190dc`** and pushed; **CI [run 35739537588](https://github.com/LorensTee/CSOPESY-MCO3/actions/run/35739537588) is green on all three OSes** (ubuntu-latest ~18 s, macos-latest ~43 s, windows-latest ~51 s). | `tests/unit/test_scheduler.cpp`, `PLAN_V3_PROGRESS.md` |
| 2026-09-23 | **S6 / P1 — T3.1 + T3.2 (W3)** | **T3.1 `Win32Terminal`** implemented in `src/platform/terminal_win32.cpp` (a prior session was cut off by a provider cap *after* the code and hand-check existed but *before* any commit): both console modes saved/restored (QuickEdit + line/echo/processed/mouse/window cleared, VT **output** on, VT **input** off), `WaitForSingleObject` then `_kbhit`/`_getch` with `0`/`224` arrow prefixes, `GetConsoleScreenBufferInfo` live sizing, `GetTickCount64` clock, flag-only `SetConsoleCtrlHandler`, loud `fatalStartup` when VT cannot be enabled (D5's replacement for `--diag`), plus an `atexit` restore backstop. The hand-check record is `docs/t3.1-win32-handcheck.md`: **49/49** private-conhost checks, **47/47** under ConPTY (2 inherently unobservable, recorded as such), a real **`FOCUS_EVENT` busy-spin defect found and fixed** (`drainFilteredRecords()` — risk 18), VT proven by reading the screen buffer back, loud-failure path proven over a pipe. This session re-verified the untouched tree: 20/20 targets, `ctest` 1/1, guard `OK (22 files scanned)`, **11/11** hashes match, no frozen header touched. **T3.2 `scripts/rehearse_windows.bat`** written and exercised on both paths (found → echoes + launches with args, exit 0; missing → lists the 5 probed paths, exit 1); it never builds, never touches source, no `.ini`/`--config` (D1); it prefers `frozen\csopesy.exe` once T6.3 produces it. Corrected the hand-check's premature *"T3.2 landed with T3.1"* line. **Environment (not a code defect):** Norton still denies **creating** `csopesy.exe` (any case) in `build/debug` and `build/release` — the current exclusion does not cover those directories — so the suite was built/run in `cmake-build-debug` instead; **fix the exclusion repo-wide before T6.3 freezes the artifact**. **Blocked:** T3.4 (graded-run gate) and T3.3 (measurements) need the finished application + `frozen/csopesy.exe` — `main.cpp`, `console_app.cpp`, `interpreter.cpp`, `line_editor.cpp`, `renderer.cpp` are other owners' `TODO` stubs; nothing faked. **Pushed 2026-09-23** as `536f865` (T3.1) + `cddba69` (T3.2, `main` HEAD); **CI [run 35866073135](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/35866073135) is green on all three OSes** (push event, head `cddba69`: ubuntu 19 s, macOS 26 s, Windows 46 s — guard `OK (22 files scanned)`, `ctest` 1/1 in each job). | `src/platform/terminal_win32.cpp`, `scripts/rehearse_windows.bat`, `docs/t3.1-win32-handcheck.md`, `PLAN_V3_PROGRESS.md` |
| 2026-09-24 | **S6 / P1 — T2.1–T2.6 (W2)** | **W2's Phase 1D implemented, TDD red-before/green-after per task.** T2.1 `parameters.cpp` (clamp + `ClampReport`; `setText` trim/keep-internal-runs → `Ok`/`Empty`/`NonAscii`, state unchanged on refusal); T2.2 `cli.cpp` (`--no-tty`/`--refresh-ms=N`/`--poll-ms=N`; unknown/malformed warn+continue, out-of-range clamps+reports, repeated flag last-wins; `5abc`/huge literals handled); T2.3 `line_editor.cpp` (`feed` echoes printable ASCII, Backspace, Enter/Eof submit, arrows/Tab/control ignored; `visibleSlice` tail window); T2.4 `interpreter.cpp` (case-sensitive dispatch table, exact §3.7 help/Usage/goodbye/not-recognized lines, `[-+]?[0-9]+` full match so `5abc`/`1.5` are Usage while `-5`/`10001`/huge clamp, three `set_text` outcomes incl. the non-ASCII rejection); T2.5 `main.cpp` + `console_app.cpp` (flags→terminal→handlers→RAII raw-mode guard→`ConsoleApp`; initial frame drawn before `start()`, input/command loop re-checks quit/shutdown after every `readEvent`, `requestStop`→`join`→single goodbye, `--no-tty`/non-tty plain line mode starts **no worker**) + the portable `ctest smoke` real-binary test; T2.6 `README.txt`. **No frozen header touched — 11/11 `git hash-object` blobs still match `CONTRACTS.md`**; guard `OK (22 files scanned)`; `ctest` **2/2** (unit 88 tests + smoke); warning-free `-Wall -Wextra`. Added **`.clangd`** (`CompilationDatabase: build/debug`) to stop the recurring false *"file not found"* on `csopesy/*.hpp` — the runtime heuristic cannot read CMake's per-target `-I` (the documented false positive in §6); the header was never actually broken, proven by `-Werror` and `lens_diagnostics source=lsp`. Hand-verified the real binary: A12-shaped plain-line transcript, flag clamping/warnings on stderr, and an interactive pty run that printed **one** goodbye and exited 0 with no join hang. **`T2.4` clears the blocker on W1's deferred T1.3 command-effect assertions; `T2.5` removes the W2 application blocker from `T1.4` (Linux sweep) and `T3.4` (Windows gate) — both become actionable only once the remaining prerequisites exist (the finished application; `frozen/csopesy.exe` for `T3.4`).** Remaining Phase 1: W4's `renderer.cpp` (`T4.1`–`T4.3`). **`T2.6`'s frozen-artifact half deferred to `T6.3`:** the interactive renderer is still a stub, so freezing a binary now would freeze an incomplete program and invalidate the no-recompile claim. | `src/entities/parameters.cpp`, `src/entities/cli.cpp`, `src/features/commands/line_editor.cpp`, `src/features/commands/interpreter.cpp`, `src/app/console_app.cpp`, `src/main.cpp`, `tests/unit/test_parameters.cpp`, `tests/unit/test_cli.cpp`, `tests/unit/test_interpreter.cpp`, `tests/smoke/*`, `CMakeLists.txt`, `README.txt`, `.clangd`, `PLAN_V3_PROGRESS.md` |
| 2026-09-24 | **S6 / P1 — W2 review follow-up** | **Validity/standards fixes, all inside W2's jurisdiction.** (1) **`feed(Eof)` now requests quit** (TDD: 2 assertions red → green). Raw mode has ISIG off, so Ctrl+C, Ctrl+D and a closed stdin all arrive as `KeyType::Eof`; the old `feed` only submitted the line, leaving §3.10's non-signal exit paths (`Ctrl+C`, `EOF`) dead — a real pty probe showed `exit`/`SIGTERM`/`kill -INT` exiting rc=0 but Ctrl+C/Ctrl+D doing nothing. Re-probe: **all six exit paths** (`exit`, Ctrl+C, Ctrl+D, Ctrl+C-mid-text, SIGTERM, kill -INT) now rc=0 with exactly one goodbye. No frozen header touched (`interpreter.hpp`'s `feed` signature/semantics comment untouched). (2) **README names Ninja** as a build prerequisite — the presets pin the Ninja generator, so "no extra dependencies beyond a compiler + CMake" was wrong for a fresh machine. (3) Removed a stray decision-id tag from `test_parameters.cpp`'s comments per `AGENTS.md` §9. Gate after: `ctest` 2/2 (**unit 90** + smoke), zero warnings, guard `OK (22 files scanned)`, **11/11 hashes** match. **Watch for T5.1:** once W4's renderer lands, `exit` may show the goodbye twice (§3.7 message row + §3.10 post-join plain line) — both sanctioned by the plan; eyeball and record which is intended. Landed as **`24c41e9`** + **`ff602f4`** (push + CI link pending). | `src/features/commands/line_editor.cpp`, `tests/unit/test_interpreter.cpp`, `tests/unit/test_parameters.cpp`, `README.txt`, `PLAN_V3_PROGRESS.md` |
| 2026-09-24 | **S6 / P1 — T4.1–T4.3 (W4)** | **W4's Phase 1A renderer implemented — Phase 1 is now code-complete.** `src/features/marquee/renderer.cpp`: `scrollOffset` (period `textWidth + bandWidth`, non-negative modulus, non-positive period ⇒ 0) and `sliceRow` (`windowLeft = offset - bandWidth`, always exactly `bandWidth` bytes) for **T4.1**; `Renderer::buildFrame` + `bandWidthFor` for **T4.2** — cursor-home + exactly `rows` CRLF-separated rows each padded/clipped to exactly `cols` (raw mode clears `ONLCR`/`DISABLE_NEWLINE_AUTO_RETURN`), chrome (`Welcome to CSOPESY!` capital W, blank, band, blank, `Group developer:` + names, blank, `Version date:` with no trailing space when blank), band at `kBandRow == 3`, prompt row last with the buffer's tail window; **T4.3** — the message is rendered one line per `\n` directly above the bottom-anchored prompt (dropping it would make `help` invisible in raw mode; the §3.9 row table does not name its row), and chrome is dropped bottom-up in the plan's order (`Version date:`, names, `Group developer:`, welcome) while the band (falling back to the row above the block when `rows < 4`) and the prompt are never dropped. `visibleSlice` is **not** called: it lives in `features/commands` and features may not cross-import, so the tail window is local. 27 new tests (`test_scroll.cpp` 11, `test_renderer.cpp` 16). **TDD evidence: 180 assertions red against the stub** (`FAILED  180 assertion(s)`, exit 1; the first red run aborted on a `substr` and the helper was made abort-proof so a stub is a clean red), then **`OK 117 tests`** / `ctest` 2/2 (unit 117 + smoke). Clean `rm -rf build/debug` rebuild warning-free under `-Wall -Wextra`; guard `OK (22 files scanned)`; **11/11 frozen hashes still match `CONTRACTS.md`** — no contract header touched. v2-deleted-feature sweep clean (only frozen-header prose mentioning that those things are gone). A scratch print of a 16×60 menu frame and a 7-line help frame matches §3.9; **known cosmetic note:** every row incl. the prompt is padded to `cols`, so the cursor rests at the right margin — revisit only if the live T3.4 demo makes it matter. **Watch for T5.1:** alongside W2's noted `exit`-goodbye question. | `src/features/marquee/renderer.cpp`, `tests/unit/test_scroll.cpp`, `tests/unit/test_renderer.cpp`, `docs/W4_KIM_PROGRESS.md`, `PLAN_V3_PROGRESS.md` |
| 2026-09-24 | **docs consistency** | Post-W4 review pass: corrected the W4 session date (2026-09-25 → 2026-09-24, matching the commit and CI timestamps) and removed the last stale Phase-1 status text (§1 "Current stage"/"Next action", §6's T4.2/T2.4 "still a stub" finding, §8's S6 row, §9 step 5). No source, test or frozen file touched. | `PLAN_V3_PROGRESS.md`, `docs/W4_KIM_PROGRESS.md` |
| 2026-09-24 | **S4 / P2 — T5.1 (Windows)** | **A1–A11 acceptance run on the Windows development build (`97ce13a`), the primary target.** A scratch ctypes harness (temp-only, not committed) spawned `csopesy.exe` with `CREATE_NEW_CONSOLE`, attached to the child's real console, injected keys with `WriteConsoleInputW`, read frames with `ReadConsoleOutputCharacterW`, and resized live — so `GetConsoleMode`/`_kbhit`/`_getch`, `SetConsoleMode`, VT output and live sizing were all exercised on a genuine console, not a pipe. **38/38 checks pass**: A1 help (both states), A2/A3 transitions, A4 text (single/multi/punct/digits/internal runs/lowercase/220 chars/empty), A5 speed (1/16/100/1000/10000/0/−5/10001/missing/`abc`/`5abc`/`1.5`/huge), A6 exit-while-animating (exit 0), A7 unknown/empty, A8 coexistence, A9 flag/command parity, A10 extremes, A11 tiny 20×5 + recovery. **A8 evidence:** 29 distinct band positions over 29 one-key-at-a-time keystrokes at `refresh-ms=40` with a resize mid-animation (layout followed, command echoed); 13/13 at `set_speed 1`; the mid-typing transcript is in the record. **A4 non-ASCII:** rule 7 is implemented and unit-tested and fires on `--no-tty` raw stdin (`set_text café` → the rejection line); the interactive path cannot produce it because the line editor appends printable ASCII only — the same acceptance/spec tension W4 found, recorded for A4's owner, **no code changed**. Gate: warning-free build, `ctest` 2/2 (unit **117** + smoke), guard `OK (22 files scanned)`, **11/11** frozen hashes. Note: the checkout arrived with future mtimes (Ninja "manifest still dirty after 100 tries"); fixed by a content-preserving `touch` + clean reconfigure — a machine/checkout artefact, not a repo defect. **Next:** the Linux half of T5.2 (W1's `ctest` evidence); T3.4/T3.3 wait on `T6.3`. | `docs/t5.1-windows-acceptance.md`, `PLAN_V3_PROGRESS.md` |
| 2026-09-24 | **S4 / P2 — T5.2 (cross-OS)** | **`ctest` verified on all three OS families — T5.2 closed for Linux + Windows (macOS second opinion).** Commit `dbfc8b9` (docs-only T5.1 record; no source/test/frozen change) pushed; **CI [run 36007416747](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/36007416747) completed success on `ubuntu-latest`, `windows-latest`, `macos-latest`** — each job's Layer guard `OK (22 files scanned)`, Configure/Build/Test all green, `ctest` **`100% tests passed … out of 2`** (ubuntu unit 1.87 s/smoke 0.00 s; windows unit 7.01 s/smoke 0.06 s; macOS unit 2.31 s/smoke 0.03 s). The local Windows build repeats `ctest --preset debug` **2/2** (unit 2.09 s, smoke 0.07 s; unit binary `OK 117 tests`) with **11/11** frozen hashes. The macOS job is the second opinion (the threaded `unit` suite runs a real worker on Apple Silicon), backed by W4's byte-exact pty pass; the Windows job is backed by the T5.1 real-console A1–A11 run. **Honest scope:** the Linux column is CI build + `ctest` only — no human A1–A11 on a Linux terminal is claimed, and `PosixTerminal` was already pty-verified at T1.1. Evidence: `docs/t5.2-cross-os-ctest.md`. **T3.4/T3.3/T1.4/T5.3 stay blocked on `frozen/csopesy.exe` (`T6.3`).** | `docs/t5.2-cross-os-ctest.md`, `PLAN_V3_PROGRESS.md` |
| 2026-09-24 | **S4 / P3 — T6.3 (freeze)** | **The frozen artifact produced — the blocker on every remaining verification task is cleared.** A clean **Release** preset build (`rm -rf build/release`, `cmake --preset release`, `cmake --build --preset release`, 0 warnings under `-Wall -Wextra`; GCC 14.2.0 MinGW-W64 UCRT, CMake 3.30.4, Ninja 1.12.1) passed `ctest --preset release` **2/2** (unit 117 + smoke), then was copied to `frozen/csopesy.exe` — **gitignored** (`.gitignore:5`), never committed — and tagged **`quiz-frozen`** at `4c5e8cf`. **SHA-256 `a9cbc09e7ffbefda9f2e3714845995e0320fd349ac47fe7f531d15ec1fd4f15f`** (186 436 bytes). §7 pre-flight verified: `frozen/` correctly absent from git; `csopesy-quiz` is a `CustomBuildApplication` with **no** Build step, `PROGRAM_PARAMS=""`, `EXECUTABLE=…/frozen/csopesy.exe`; the frozen binary runs `help` (six lines) and `exit` (one goodbye, rc=0), also with `--refresh-ms=25 --poll-ms=5`. **Only the T3.4 hand-run remains** (CLion Run press: SHA-256 unchanged, and the documented `EMULATE_TERMINAL`/`USE_EXTERNAL_CONSOLE` tension gets settled — flipping it leaves the binary/hash untouched). Record `docs/frozen-artifact.md`. **Unblocks T3.3 (Windows sweep), T1.4 (Linux sweep), T3.4 and T5.3.** | `docs/frozen-artifact.md`, `PLAN_V3_PROGRESS.md` |

## 8. Stage plan (S1–S6)

| Stage | Deliverable | Status |
| --- | --- | --- |
| **S1** | This log + v3 §0–§2 + §3.0 change inventory | ✅ done 2026-09-22 |
| **S1.5** | gpt-v13 absorbed: D13–D16 + the §9 stage sequence | ✅ done 2026-09-22 |
| **S2 (plan text)** | v3 §3.1–§3.12 — the delta contract | ✅ done 2026-09-22 |
| **S2 (bytes)** | the header edits + `cli.hpp` + `check_layers.sh` map + `CONTRACTS.md` v3.0 + both run configs' `PROGRAM_PARAMS` cleared + `config/csopesy.ini` deleted | ✅ **landed 2026-09-22 as `d6379df`** (task **T0.6**, 30 files). Header set 13 → 11; guard `OK (22 files scanned)`; `ctest` 1/1; 11/11 hashes match; CI green on all three OSes (run 35724109198). Only Step 6 (announce) stays open |
| **S3** | v3 §4 (workstreams/RACI) + §5 (task backlog incl. T0.6, Phase 0 status carried with evidence, Sep-28 schedule) | ✅ done 2026-09-22 |
| **S4** | v3 §6 (DoD + A1–A12) + §7 (runbook) + §8 (risks) + §9 (the answers) + §10 (self-review, revision log, review dispositions) | ✅ done 2026-09-22 |
| **S5** | `AGENTS.md` (§2 commands, §5 clock/threading invariants, §7 behaviour contracts) + superseded banner on v2 + `REVIEW_ADJUDICATION.md` round 8 for gpt-v9…v14 | ⬜ P3 — after `ctest` is green |
| **S6** | Implement Phase 1 against the re-frozen v3.0 contracts (not a documentation stage) | ✅ **code-complete 2026-09-24 — T1.1–T1.3 (W1), T2.1–T2.6 (W2), T3.1–T3.2 (W3), T4.1–T4.3 (W4) all implemented and locally verified; `ctest` 2/2 (unit 117 + smoke), guard `OK (22 files scanned)`, 11/11 frozen hashes unchanged.** W4's renderer landed as `cc4e879`/`e3ae9e8`, **CI green on all three OSes** ([run 35978176723](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/35978176723)). **T6.3 froze `frozen/csopesy.exe` on 2026-09-24 (`docs/frozen-artifact.md`), so the remaining items are the live/hand-run gate (`T3.4`) and the sweeps (`T1.4`/`T3.3`), not an artifact dependency.** The only stage that scores points |

---

## 9. Stage sequence

**Order (agreed 2026-09-22, D17).** Steps 1–4 are done; 5–6 remain.

```text
1. Correct the deadline → Mon 2026-09-28            ✅ S2.5
2. Finish the task backlog (§5, incl. T0.6)        ✅ S3
3. Ratify the v3.0 contract change with W2 + the affected owners   ✅ D18 (W2, 2026-09-22)
   (the proposal text was v3 §3.12 + task T0.6; §10 below is the announcement still to send)
4. Apply the v3.0 header/contract change          → S2 (bytes) = T0.6   ✅ `d6379df`
5. Implement Phase 1                              → S6   ✅ all four workstreams done 2026-09-24
6. Verification, PPT, runbook, video              → S4 (P2) then S5 (P3)
```

**Why step 3 comes before step 4 and nothing else moves it:** the header set changes (11 headers, 3 deletions,
1 addition), `include/csopesy/*.hpp` is W2's A/R, and §4.5's whole purpose is that a mid-phase header change
invalidates other members' builds. With six days the schedule cost of waiting is small and the cost of an
unratified change is a teammate's broken build.

| Priority | Work | Gate |
| --- | --- | --- |
| **P0** | Steps 3–4: the §4.5 ratification, then T0.6 (contracts v3.0 + guard map + `CONTRACTS.md` + run configs + deletions) | ✅ **met in full 2026-09-22** — `check_layers: OK (22 files scanned)` (the predicted count, confirmed), `ctest` 100% / 1 test, every hash in the new table matching, and **CI green on all three OSes** ([run 35724109198](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/35724109198)) |
| **P0** | Step 5: Phase 1 (T4.1–T4.3, T1.1–T1.3, T3.1, T2.1–T2.5) | the threaded scheduler's deterministic tests, then the real binary on Windows and Linux |
| **P1** | Step 6: T3.4 Windows gate, T5.1–T5.3, T1.4/T3.3 measurements, T2.6 `README.txt` | **A1–A11 on the Windows build ✅ T5.1 done 2026-09-24** (`docs/t5.1-windows-acceptance.md`); **T5.2 ✅ done 2026-09-24** — `ctest` 2/2 on Windows/Linux/macOS ([CI 36007416747](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/36007416747), `docs/t5.2-cross-os-ctest.md`); **T6.3 ✅ artifact frozen 2026-09-24** (`frozen/csopesy.exe`, `docs/frozen-artifact.md`) so the sweeps (T1.4/T3.3), T3.4 and T5.3 are now actionable |
| **P2** | S4: v3 §6–§10 (DoD, acceptance, runbook, risks) | runbook dry run against the frozen SHA-256 |
| **P3** | S5: `AGENTS.md`, v2's superseded banner, adjudication round 8 | internal hygiene; do it **after** `ctest` is green |

**Never cut, at any priority:** the six commands, terminal restore on every exit path, the §3.5 ASCII
text contract (the scroll math's width model depends on it), the two-thread design (professor-mandated,
answer #3), the video constraints, and `README.txt`'s entry-file statement.

---

## 10. Paste-ready §4.5 proposal (send to W2 Byron + the affected owners)

This is the message to send for §9 step 3. It is deliberately short: the recipients are being asked to approve a
**specification**, not to review code, and no implementation exists to disrupt (every `src/*.cpp` is still a
`TODO` stub). The authoritative detail is v3 §3.12, task T0.6 and the §4.5 per-owner table; this is the summary
they read first.

> **Subject: contracts v3.0 — can you confirm you agree?**
>
> Short version: after the professor answered our five questions, we are simplifying the frozen contracts and
> cutting the parts the handout never required. This is a change to `include/csopesy/*.hpp`, so per §4.5 it
> needs your OK as freeze owner before anything is applied.
>
> **Removed:** the `.ini`/config layer (`config_io.hpp`, `--config`, `quiz_case_<n>.ini`); the 5×5 glyph
> engine and `ascii_art` (`glyphs.hpp`); `marquee_row`; `--diag`; the `--measure` telemetry; the
> `FrameBuffer` diffing (`frame_buffer.hpp`); and the extra injected `Clock`.
>
> **Kept:** the two threads, one mutex and one condition variable the professor asked for; the FSD layers and
> `check_layers.sh`; the `CONTRACTS.md` freeze itself; the 3-OS CI matrix; and `--no-tty` for the CI smoke test.
>
> **Added:** a small `cli.hpp` with three flags — `--no-tty` (dev/CI only), `--refresh-ms=N`, `--poll-ms=N` —
> and `Parameters::setText` returning `Ok | Empty | NonAscii`, because the text contract is now explicitly
> ASCII (out-of-range input is rejected with a message, never mangled).
>
> **Effect on you:** the header set goes 13 → 11 and the marker goes v2.6 → v3.0, with the hash table
> regenerated in the same commit.
>
> Detail: `docs/IMPLEMENTATION_PLAN_v3.md` §3.12 (the exact file list, in order), §4.5 (what each owner is
> accepting) and T0.6 (the seven steps). If you agree, reply and I will record it and land T0.6; if any part is
> wrong, say which — nothing is applied before that.

**Done.** D18 is recorded in §4 above and T0.6 landed as `d6379df`; this proposal is kept as the record of what
was asked and answered. **§11 was sent to the group on 2026-09-22** (T0.6 Step 6) and is kept as the record.

---

## 11. Paste-ready T0.6 Step 6 announcement (send to the group)

Step 6 is the one T0.6 step a human has to send; it is left unchecked in the plan for that reason. **Sent to the
group on 2026-09-22 (operator-stated).** Kept as the record of what was sent:

> **Contracts are now v3.0** — landed as `d6379df` on 2026-09-22. What it means for your next commit:
>
> **The header set is 13 → 11.** Added `include/csopesy/cli.hpp`. Deleted `config_io.hpp`, `glyphs.hpp` and
> `frame_buffer.hpp`, along with their `.cpp` files, `test_config.cpp`, `test_glyphs.cpp` and
> `config/csopesy.ini`.
>
> **Four contract removals — do not re-add one from memory:** `Parameters::asciiArt`, `Parameters::marqueeRow`,
> `Parameters::measurePath`, and the injected `Clock` (use `term_.nowMs()` — it is the single clock source).
> Two signature changes: `Renderer::drawFrame(FrameBuffer&, …)` became `Renderer::buildFrame(…) -> std::string`
> (one complete frame, one `write()`), and `Parameters::setText` now returns
> `TextResult{Ok, Empty, NonAscii}` instead of `bool`.
>
> **Unchanged:** the two threads, one mutex + one condition variable, the FSD layers and `check_layers.sh`, the
> `CONTRACTS.md` freeze, and the 3-OS CI.
>
> **Before your first commit, run:** `cmake --preset debug && cmake --build --preset debug && ctest --preset debug`
> (expect 1/1) and `bash scripts/check_layers.sh` (expect `OK (22 files scanned)`). If your build breaks on a
> deleted header, you are reading v2.6 — `CONTRACTS.md`'s hash table is the authority on what is current.
