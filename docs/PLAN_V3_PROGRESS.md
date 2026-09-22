# PLAN_V3_PROGRESS.md — the v3 migration log and resume anchor

**Purpose.** This file exists so that a **lost chat, a crashed session, or a different agent** can resume the
v2.6 → v3 plan migration without re-deriving anything. It is the *working* record; `docs/IMPLEMENTATION_PLAN_v3.md`
is the *deliverable*. When the two disagree, this file says what is actually done and the plan says what is
intended — **update both in the same session**.

**Rule for this file:** update it at the **end of every working session**, before you stop. A session that
changes nothing still gets a dated line in §7. Never mark a stage done without the artifact it produced.

**Established:** 2026-09-22 · **Last updated:** 2026-09-22 (**T0.6 fully closed — announcement sent; Phase 1 started: T1.1 landed**)
**Deadline: Monday 2026-09-28** (revised from 2026-09-23 — six days, not one). **The §4.5 change-control protocol
is honored in full: no header change lands ahead of ratification.** Stage sequence in §9.

---

## 1. Resume here

| | |
| --- | --- |
| **Deadline** | **Monday 2026-09-28** (revised from 2026-09-23 on 2026-09-22 — operator-stated; see §6 provenance). Six days. Urgency removed, so **D17** applies: no workaround for §4.5. |
| **Current stage** | **T0.6 — DONE and fully closed. Phase 1 has started.** Contracts are **v3.0**, committed as **`d6379df`** (2026-09-22; guard `OK (22 files scanned)`, `ctest` 1/1, **11/11** hashes match). **T1.1 (`src/platform/terminal_posix.cpp`, `PosixTerminal`) is implemented and pty-verified**; the remaining Phase 1 tasks are still `TODO` stubs. |
| **Next action** | **Keep going in Phase 1** (§9 step 5). W1: `T1.2` (PCB) → **`T1.3` (the threaded scheduler) — the critical path**; `T1.1` is done. W2's `T2.1`/`T2.2`, W3's `T3.1` and W4's `T4.1`–`T4.3` are unblocked. Task bodies and ids are in `IMPLEMENTATION_PLAN_v3.md` §5 — ids keep their v2 numbers, and §3.8's delta list is what applies to the scheduler. |
| **T0.6 Step 6 — CLOSED** | **The group announcement (§11) was sent** — operator-stated 2026-09-22 (a human action, recorded here rather than as an in-repo artifact). Together with the green CI run [35724109198](https://github.com/LorensTee/CSOPESY-MCO3/actions/runs/35724109198) on all three OSes (ubuntu 17 s, macOS 26 s, Windows 45 s), **every T0.6 step is now closed** and `AGENTS.md` §8 item 2 is satisfied. |
| **Blocked on** | **Nothing.** The §4.5 gate is satisfied and recorded as **D18** (W2 Byron, the freeze owner, 2026-09-22). If W3 or W4 objects to the change, `d6379df` is one revertable commit; when it landed every `src/*.cpp` was still a `TODO` stub, so nothing consumed it yet (T1.1 is the first consumer, 2026-09-22). |
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
| **S6** | Implement Phase 1 against the re-frozen v3.0 contracts (not a documentation stage) | 🟨 **in progress — T1.1 (`PosixTerminal`) implemented and pty-verified 2026-09-22**; T1.2/T1.3 (W1), T2.x, T3.1, T4.x still `TODO`. The only stage that scores points |

---

## 9. Stage sequence

**Order (agreed 2026-09-22, D17).** Steps 1–4 are done; 5–6 remain.

```text
1. Correct the deadline → Mon 2026-09-28            ✅ S2.5
2. Finish the task backlog (§5, incl. T0.6)        ✅ S3
3. Ratify the v3.0 contract change with W2 + the affected owners   ✅ D18 (W2, 2026-09-22)
   (the proposal text was v3 §3.12 + task T0.6; §10 below is the announcement still to send)
4. Apply the v3.0 header/contract change          → S2 (bytes) = T0.6   ✅ `d6379df`
5. Implement Phase 1                              → S6   🟨 T1.1 done; T1.2/T1.3 next
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
| **P1** | Step 6: T3.4 Windows gate, T5.1–T5.3, T1.4/T3.3 measurements, T2.6 `README.txt` | **A1–A11** on the Windows build; measurement tables committed |
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
