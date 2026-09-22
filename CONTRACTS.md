# CONTRACTS.md — the interface freeze marker

**contracts v3.0 — 2026-09-22**

Everything under `include/csopesy/` is an **interface contract**: the declarations the other layers
compile against, plus the semantics those declarations promise. After this marker a contract changes
only through the protocol at the bottom of this file — never by a silent edit, because a mid-phase header
change invalidates a teammate's build on a machine you are not holding.

**Owner (§4.5):** W2 (Byron) is **A/R** for `include/csopesy/*.hpp` and for this file; W1, W3 and W4 are C.
**Source of truth:** `docs/IMPLEMENTATION_PLAN_v3.md` §3 (the delta contract), §3.8 (threading), §4.5 (protocol),
§3.12 + **T0.6** (what this v3.0 change consisted of).
**History:** `docs/REVIEW_ADJUDICATION.md` records why each earlier revision looks the way it does (rounds 1–7
cover v1 → v2.6); `docs/PLAN_V3_PROGRESS.md` §4 records the v3.0 decisions D1–D18 with their authorities.

## What this freeze covers

**Frozen:** every public declaration in the headers below — signatures, struct fields, constants — and the
documented semantics attached to them (ownership, locking, one-writer rules, error behaviour).

**Not frozen, because a header says so explicitly:** the private members listed in the next section. Those
are implementation detail; adding them is not a contract change and does not reopen this freeze.

## The frozen headers (11)

| Header | § | What it pins |
| --- | --- | --- |
| `keys.hpp` | 3.3 | `KeyType` / `KeyEvent` — the normalized key vocabulary, i.e. the platform→interpreter boundary |
| `terminal.hpp` | 3.3 | **The only platform-dependent contract.** `enterRawMode`/`restore`/`size`/`readEvent`/`write`/`flush`/`isTty`/`nowMs` + `Terminal::create()`. **Since v3.0 `nowMs()` is the *only* clock source** — the injected `Clock` is gone (D7) |
| `shutdown.hpp` | 3.10 | `installShutdownHandlers()` / `shutdownRequested()`; defined once in the selected `src/platform/*`; the flag is read-and-clear, and only by the input thread |
| `parameters.hpp` | 3.5 | The single `Parameters` struct (7 fields), its ranges (`kRefreshMin`/`kRefreshMax`/`kPollMin`/`kPollMax`), the three-layer precedence defaults → CLI → command, and `TextResult` — the ASCII text contract (D15) |
| `cli.hpp` | 3.6 | **New in v3.0** (replaces `config_io.hpp`): `parseCli` + `CliResult`. Exactly three flags; bad input warns and is never fatal |
| `process.hpp` | 3.8 | `MarqueeProcess` (the PCB): `Stopped`/`Running`, `cycles`, `hasRendered`, `start()` / `stop()` |
| `scheduler.hpp` | 3.8 | The two-thread contract: one mutex, one condition variable, one owner per resource. `tick()` (the pure step), `run()`, `postEvent()`, `wake()`, `requestStop()`, `join()`, `snapshot()`; types `TickResult`, `SchedulerSnapshot` |
| `renderer.hpp` | 3.9 | Pure scroll math (`scrollOffset`, `sliceRow`), `Renderer::buildFrame` (one complete frame as a single string), `bandWidthFor`, and the fixed `kBandRow` |
| `line_editor.hpp` | 3.11, §T2.3 | Declares **nothing**, on purpose — see below |
| `interpreter.hpp` | 3.11 | The editing state (`line_`, `message_`, `quit_`), command dispatch (`feed`, `executeLine`, `quitRequested`), and `visibleSlice` — the prompt-row scroll window |
| `console_app.hpp` | 3.1, §T2.5 | The composition root: `ConsoleApp(Terminal&, Parameters&)` and `int run()`. Owns the §3.10 shutdown sequence |

**Deleted in v3.0 (D1/D2/D6):** `config_io.hpp`, `glyphs.hpp`, `frame_buffer.hpp`. Their replacements are
`cli.hpp`, nothing (the band is one row of text), and one string per frame inside `renderer.hpp`.

## Deliberately deferred (unchanged from v2.6)

- **`line_editor.hpp` stays empty of declarations.** §3.11 keeps the editing state on `Interpreter` and
  declares the line editor's only named helper, `visibleSlice`, in `interpreter.hpp`, under the rule
  *"Interpreter itself takes no lock; the caller owns it"*. A declaration here would put a second home on one
  piece of state. `src/features/commands/line_editor.cpp` implements those rules and declares nothing.
- **`console_app.hpp`** — the private composition-root members (`Renderer`, `Interpreter`, `MarqueeProcess`,
  `Scheduler`) arrive with **T2.5**. A private-member addition does not reopen this freeze.

## Re-frozen in v3.0

The change was proposed and ratified under §4.5 (**D18: W2 Byron agreed, 2026-09-22**), and the four headers
that actually changed were re-frozen first:

- **`cli.hpp` — new.** `CliResult{Parameters params; std::vector<std::string> warnings;}` + `parseCli(args)`,
  where `args` excludes `argv[0]`.
- **`parameters.hpp`** — `asciiArt`, `marqueeRow` and `measurePath` **removed**; `TextResult{Ok,Empty,NonAscii}`
  added and `setText` now returns it. Ten fields become seven.
- **`renderer.hpp`** — `FrameBuffer` include, `drawFrame(FrameBuffer&, …)`, `artWidthFor` and `composeBand`
  removed; `buildFrame(…)->std::string`, `bandWidthFor` and `kBandRow` added.
- **`scheduler.hpp`** — the injected `Clock` (and `now_`), the `FrameBuffer`/`fbRows_`/`fbCols_` fields, and
  `measure_`/`appendMeasure()`/`pendingEventMs_`/`eventOwed_`/`noteEventLocked()` **removed**; the constructor
  loses its `Clock` parameter.

**A property worth keeping:** the **7 headers nobody needed to touch still carry their exact v2.6 blob
hashes** (see the table below). That is checkable evidence that this revision is the *delta claimed* and not a
quiet rewrite of something else — exactly the kind of proof the freeze exists to make possible.

**Recorded deviation:** `interpreter.hpp` includes `<string_view>` and `csopesy/keys.hpp`, which the plan's
§3.11 snippet omits, so the header compiles standalone. Two added includes; no signature changed. Noted in the
header itself at T0.1, and unchanged since.

## Change protocol (§4.5, verbatim)

> Contracts change only by: propose in chat → W2 + the affected owner agree → bump the `CONTRACTS.md` marker
> → announce. No silent header edits after Phase 0: a mid-phase header change invalidates other members'
> builds on machines they cannot fix.

And, from the same table: **one writer per file at any time.**

## Freeze check (content identity)

`git hash-object` hashes the *committed blob*, so a CRLF checkout or a different line-ending setting cannot
produce a false mismatch:

| Header | Blob hash | v3.0 status |
| --- | --- | --- |
| `cli.hpp` | `4d2f3f25bf004f13edb1b57e83dd004f966c20f5` | **new** |
| `console_app.hpp` | `af282ac885161be648f2c1e038936cce1e66d584` | unchanged (= v2.6) |
| `interpreter.hpp` | `bb7d888a59e9a801fa57e380d408fae5f98365ed` | unchanged (= v2.6) |
| `keys.hpp` | `1f6eeea8a1930ba9b15ea02fa62f29d65e106fa4` | unchanged (= v2.6) |
| `line_editor.hpp` | `5f229d78338e7116a13b95c19c8fa98da62746d5` | unchanged (= v2.6) |
| `parameters.hpp` | `d190e4f5a56f01140ef774a872ba4d287cb892a0` | **changed** |
| `process.hpp` | `104ed6f89fa51fdacaa635fdb4d9c757dceb2343` | unchanged (= v2.6) |
| `renderer.hpp` | `3b0bd9663f5127ecd4a354354dec5181aa8220fe` | **changed** |
| `scheduler.hpp` | `6c006cdcaddfe5c9573959d149cb794f457a980a` | **changed** |
| `shutdown.hpp` | `02bb0a2c722be76278b16a0fbf3fa3f2b00f750b` | unchanged (= v2.6) |
| `terminal.hpp` | `e9031d85b59aad0c52d9978e70be69dd2db4128e` | unchanged (= v2.6) |

Re-check with `git hash-object include/csopesy/*.hpp`. A mismatch means a contract header changed: either it
went through the protocol above — in which case regenerate this table and bump the marker *in the same commit* —
or it did not, in which case revert it. A tool that rewrites a contract header is making a contract change;
re-running a formatter does not exempt it.

These headers are also the layer guard's input: `scripts/check_layers.sh` fails the build on any upward or
cross-slice include among them (§3.1), on every push, on all three OSes (`.github/workflows/ci.yml`). v3.0
updated `layer_of_header()` in the same commit: `cli.hpp` → `entities`; `config_io.hpp`, `glyphs.hpp` and
`frame_buffer.hpp` removed from the map.
