# CONTRACTS.md — the interface freeze marker

**contracts v3.1.1 — 2026-09-28**

Everything under `include/csopesy/` is an **interface contract**: the declarations the other layers
compile against, plus the semantics those declarations promise. After this marker a contract changes
only through the protocol at the bottom of this file — never by a silent edit, because a mid-phase header
change invalidates a teammate's build on a machine you are not holding.

**Owner (§4.5):** W2 (Byron) is **A/R** for `include/csopesy/*.hpp` and for this file; W1, W3 and W4 are C.
**Source of truth:** `docs/IMPLEMENTATION_PLAN_v3.md` §3 (the delta contract), §3.8 (threading), §4.5 (protocol),
§3.12 + **T0.6** (what this v3.0 change consisted of).
**History:** `docs/REVIEW_ADJUDICATION.md` records why each earlier revision looks the way it does (rounds 1–7
cover v1 → v2.6); `docs/PLAN_V3_PROGRESS.md` §4 records the v3.x decisions D1–D19 (v3.0: D1–D18; v3.1: D19,
the authorized `config.txt` default layer; v3.1.1: the ASD-STE100 comment rewrite, which changed all 11 header
blobs without changing a declaration).

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
| `parameters.hpp` | 3.5 | The single `Parameters` struct (7 fields), its ranges (`kRefreshMin`/`kRefreshMax`/`kPollMin`/`kPollMax`), the four-layer precedence built-in defaults → `config.txt` → CLI → command, and `TextResult` — the ASCII text contract (D15) |
| `cli.hpp` | 3.6 | `parseCli` + `CliResult` + `loadConfigFile`. Exactly three flags plus the optional `config.txt` default layer; bad input warns and is never fatal |
| `process.hpp` | 3.8 | `MarqueeProcess` (the PCB): `Stopped`/`Running`, `cycles`, `hasRendered`, `start()` / `stop()` |
| `scheduler.hpp` | 3.8 | The two-thread contract: one mutex, one condition variable, one owner per resource. `tick()` (the pure step), `run()`, `postEvent()`, `wake()`, `requestStop()`, `join()`, `snapshot()`; types `TickResult`, `SchedulerSnapshot` |
| `renderer.hpp` | 3.9 | Pure scroll math (`scrollOffset`, `sliceRow`), `Renderer::buildFrame` (one complete frame as a single string), `bandWidthFor`, and the fixed `kBandRow` |
| `line_editor.hpp` | 3.11, §T2.3 | Declares **nothing**, on purpose — see below |
| `interpreter.hpp` | 3.11 | The editing state (`line_`, `message_`, `quit_`), command dispatch (`feed`, `executeLine`, `quitRequested`), and `visibleSlice` — the prompt-row scroll window |
| `console_app.hpp` | 3.1, §T2.5 | The composition root: `ConsoleApp(Terminal&, Parameters&)` and `int run()`. Owns the §3.10 shutdown sequence |

**Deleted in v3.0 (D1/D2/D6):** `config_io.hpp`, `glyphs.hpp`, `frame_buffer.hpp`. Their replacements are
`cli.hpp`, nothing (the band is one row of text), and one string per frame inside `renderer.hpp`.
**Added in v3.1 (D19):** the optional `config.txt` default layer inside `cli.hpp` — see below.

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

## Re-frozen in v3.1

The change was authorized by the operator on 2026-09-28 as an explicit requirements change (**D19**),
superseding D1's "no config file" assumption, and lands with the header change so the marker and code agree:

- **`cli.hpp`** — adds `CliResult loadConfigFile(const std::string& path)` and a `configPath` parameter on
  `parseCli` (default `"config.txt"`). `config.txt` is the layer between the built-in defaults and the CLI
  flags; it supplies defaults only, flags always win, and a missing file is silent. The format is
  `key=value`; the recognized keys are `refresh_ms` and `polling_ms`, mapped to `setRefresh`/`setPolling`, so
  malformed values warn, out-of-range values clamp and report, and unknown keys are ignored — all without
  aborting startup. The three CLI flags are unchanged and `--config` still does not exist.

Only `cli.hpp`'s blob changed; the other 10 headers still match their recorded hashes (the exact evidence that
this is the delta claimed). `parameters.hpp`'s **documented precedence** is now four layers, but its declared
members and methods are unchanged, so no edit was needed there.

## Re-frozen in v3.1.1

Landed 2026-09-28 as a **comment-only** revision: the source comments were rewritten in ASD-STE100 style, and
every internal reference — decision ids such as D19, and `§`-numbered plan sections — was removed from
`include/csopesy/*.hpp` and `src/**`, because `AGENTS.md` §9 keeps that material in `docs/`. Nobody grading the
project should need to read a decision log to understand a header.

A comment edit inside a frozen header is a contract change (`AGENTS.md` §4 and §9), so **all 11 blobs changed**
and the whole table below was regenerated in the same commit. That is a deliberate departure from v3.0/v3.1,
where the evidence was that untouched headers kept their recorded hashes.

**What did not change:** no signature, struct field, constant, default value or documented semantic moved. The
only code-level differences in any header are multi-declarator splits, where `T a, b;` became `T a; T b;` (same
type, same order, same initializers) — `interpreter.hpp` (1), `parameters.hpp` (2), `scheduler.hpp` (4).
`cli.hpp` carries the D19 config API and `parameters.hpp`'s comment now states the four-layer precedence that
`cli.hpp` implements; before this revision that comment still said three layers.

**Marker:** the rewrite is a patch-level revision, so the marker is `v3.1.1`, not `v3.2`.

## Change protocol (§4.5, verbatim)

> Contracts change only by: propose in chat → W2 + the affected owner agree → bump the `CONTRACTS.md` marker
> → announce. No silent header edits after Phase 0: a mid-phase header change invalidates other members'
> builds on machines they cannot fix.

And, from the same table: **one writer per file at any time.**

## Freeze check (content identity)

`git hash-object` hashes the *committed blob*, so a CRLF checkout or a different line-ending setting cannot
produce a false mismatch:

| Header | Blob hash | v3.1.1 status |
| --- | --- | --- |
| `cli.hpp` | `9a3f600c177a453a7913d7aaecd01de6fa5080dd` | **changed (v3.1.1 — STE100 comments + D19 API)** |
| `console_app.hpp` | `3d65fca6d2ed5674023208401fdaf84d806c19ef` | **changed (v3.1.1 — comments)** |
| `interpreter.hpp` | `afc9891d09181e20d1d1fdddcc4c27fe75d7c912` | **changed (v3.1.1 — comments)** |
| `keys.hpp` | `5071f0f1dbde46a793358102b00db7e7d4e2d81a` | **changed (v3.1.1 — comments)** |
| `line_editor.hpp` | `89efcf7d4eaf8bb44d7cd92541adff374037971c` | **changed (v3.1.1 — comments)** |
| `parameters.hpp` | `dc64cd56c06c4002846f03bf87f35f065c1ce35a` | **changed (v3.1.1 — comments + four-layer precedence)** |
| `process.hpp` | `0a924a4c5f54d91dac35606209c74034866f774c` | **changed (v3.1.1 — comments)** |
| `renderer.hpp` | `e8d4f633b9b9529e1747b4947e840b35e6ce2c33` | **changed (v3.1.1 — comments)** |
| `scheduler.hpp` | `37c212aa00b12edf92d203611f8544a815d2c0f8` | **changed (v3.1.1 — comments)** |
| `shutdown.hpp` | `921b5e65bfd063ba65f4ee5ccf39bdaf7f81d4f1` | **changed (v3.1.1 — comments)** |
| `terminal.hpp` | `866f0197296608076c74baf17bba01397aaa7ae4` | **changed (v3.1.1 — comments)** |

The v3.0 and v3.1 tables are superseded by this one; they remain in git history for the "the untouched headers
kept their hashes" evidence recorded in the sections above.

Re-check with `git hash-object include/csopesy/*.hpp`. A mismatch means a contract header changed: either it
went through the protocol above — in which case regenerate this table and bump the marker *in the same commit* —
or it did not, in which case revert it. A tool that rewrites a contract header is making a contract change;
re-running a formatter does not exempt it.

**Update — 2026-09-27, group member names + version date.** `parameters.hpp`'s `developers` default changed from
the handout-mock names to the four actual group members (`Ang, Byron Scott` · `Laborada, Nathan` ·
`Sotingco, Kimbery Wynelle` · `Tee, John Lorens`), and its `versionDate` default changed from blank to
`"2026-09-27"`, so its blob hash above was regenerated. These are data-only changes to field initializers; no
signature, range or method changed, and the other 10 headers still match their recorded hashes.
`tests/unit/test_parameters.cpp` and `tests/unit/test_renderer.cpp` were updated in the same change.

These headers are also the layer guard's input: `scripts/check_layers.sh` fails the build on any upward or
cross-slice include among them (§3.1), on every push, on all three OSes (`.github/workflows/ci.yml`). v3.0
updated `layer_of_header()` in the same commit: `cli.hpp` → `entities`; `config_io.hpp`, `glyphs.hpp` and
`frame_buffer.hpp` removed from the map.
