# CONTRACTS.md — the interface freeze marker

**contracts v2.6 — 2026-09-16**

Everything under `include/csopesy/` is an **interface contract**: the declarations the other layers
compile against, plus the semantics those declarations promise. After this marker a contract changes
only through the protocol at the bottom of this file — never by a silent edit, because a mid-phase header
change invalidates a teammate's build on a machine you are not holding.

**Owner (§4.5):** W2 (Byron) is **A/R** for `include/csopesy/*.hpp` and for this file; W1, W3 and W4 are C.
**Source of truth:** `docs/IMPLEMENTATION_PLAN_v2.md` §3 (the contracts), §3.8 (threading), §4.5 (protocol).
**History:** `docs/REVIEW_ADJUDICATION.md` records why v2.6 differs from the v1 drafts.

## What this freeze covers

**Frozen:** every public declaration in the headers below — signatures, struct fields, constants — and the
documented semantics attached to them (ownership, locking, one-writer rules, error behaviour).

**Not frozen, because a header says so explicitly:** the private members listed in the next section. Those
are implementation detail; adding them is not a contract change and does not reopen this freeze.

## The frozen headers

| Header | § | What it pins |
| --- | --- | --- |
| `keys.hpp` | 3.3 | `KeyType` / `KeyEvent` — the normalized key vocabulary, i.e. the platform→interpreter boundary |
| `terminal.hpp` | 3.3 | **The only platform-dependent contract.** `enterRawMode`/`restore`/`size`/`readEvent`/`write`/`flush`/`isTty`/`nowMs` + `Terminal::create()`. `nowMs()` is the *only* clock source, injected into `Scheduler` |
| `shutdown.hpp` | 3.10 | `installShutdownHandlers()` / `shutdownRequested()`; defined once in the selected `src/platform/*`; the flag is read-and-clear, and only by the input thread |
| `parameters.hpp` | 3.5 | The single `Parameters` struct, its ranges (`kRefreshMin`/`kRefreshMax`/`kPollMin`/`kPollMax`) and the layered precedence defaults → ini → CLI |
| `config_io.hpp` | 3.6 | `loadConfig` / `loadConfigFromText` / `applyCliArgs` + `ConfigResult`. Bad input warns; it is never fatal |
| `process.hpp` | 3.8 | `MarqueeProcess` (the PCB): `Stopped`/`Running`, `cycles`, `hasRendered`, `start()` / `stop()` |
| `scheduler.hpp` | 3.8 | The two-thread contract: one mutex, one condition variable, one owner per resource. `tick()` (the pure step), `run()`, `postEvent()`, `wake()`, `requestStop()`, `join()`, `snapshot()`; types `TickResult`, `SchedulerSnapshot` |
| `glyphs.hpp` | 3.9 | Glyph cell geometry (`kCellCols`, `kCellRows`, `kArtRows`) and the rasterizer `glyphFor` / `renderBlockText` |
| `renderer.hpp` | 3.9 | Pure scroll math (`scrollOffset`, `sliceRow`, `artWidthFor`) and `Renderer::drawFrame` / `composeBand` |
| `frame_buffer.hpp` | 3.9 | The `FrameBuffer` public API: `resize` / `put` / `putRow` / `renderDiff` / `invalidate` |
| `line_editor.hpp` | 3.11, §T2.3 | Declares **nothing**, on purpose — see below |
| `interpreter.hpp` | 3.11 | The editing state (`line_`, `message_`, `quit_`), command dispatch (`feed`, `executeLine`, `quitRequested`), and `visibleSlice` — the prompt-row scroll window |
| `console_app.hpp` | 3.1, §T2.5 | The composition root: `ConsoleApp(Terminal&, Parameters&)` and `int run()`. Owns the §3.10 shutdown sequence |

## Deliberately deferred (resolved or restated at this freeze)

- **`line_editor.hpp` stays empty of declarations — decided here, at T0.4.** The header deferred this
  choice to the freeze ("*this header stays empty of declarations until T2.3 (and the T0.4 freeze) decide
  otherwise*"), so the decision is now recorded: it stays empty. §3.11 keeps the editing state on
  `Interpreter` and declares the line editor's only named helper, `visibleSlice`, in `interpreter.hpp`,
  under the rule *"Interpreter itself takes no lock; the caller owns it"*. A declaration here would put a
  second home on one piece of state — the exact failure §3.11 exists to prevent.
  `src/features/commands/line_editor.cpp` implements those rules and introduces no new declaration.
- **`frame_buffer.hpp`** — private members arrive with the implementation at **T4.3**; the public API in
  the table above is the contract.
- **`console_app.hpp`** — private composition-root members (`Renderer`, `Interpreter`, `MarqueeProcess`,
  `Scheduler`) arrive at **T2.5**. Recorded as a private-member addition, which explicitly does not reopen
  this freeze.

## Re-frozen in v2.6

These are the headers that actually changed (§T0.4), re-frozen first:

- **`scheduler.hpp`** — the two-thread contract and its v2.5 critical-section rules; the v2.6 narrowing of
  `tick()`'s return to `TickResult{ bool rendered }` because `run()` ignores the result and the worker never
  reads `quit_`. The header also carries the `tick(const KeyEvent*)` test-only overload: production only ever
  steps with `nullptr` via `run()`, and both paths share one locked body by construction.
- **`parameters.hpp`** — `measurePath`, the lock-free exception to the mutex discipline: `main` writes it once
  before any thread exists, and the marquee thread then only reads it (§3.5). Declared in the §3.5 struct; the
  CLI flag that sets it is a §3.6 lever.
- **`interpreter.hpp`** — the `quit_` ownership note: the *input* thread is its only reader and writer, which
  is why `TickResult` lost `quit` in v2.6 (§3.8, §3.11).

**Recorded deviation that does not reopen the freeze:** `interpreter.hpp` includes `<string_view>` and
`csopesy/keys.hpp`, which the §3.11 snippet omits, so that the header compiles standalone. Two added
includes; no signature changed. Noted in the header itself at T0.1.

## Change protocol (§4.5, verbatim)

> Contracts change only by: propose in chat → W2 + the affected owner agree → bump the `CONTRACTS.md` marker
> → announce. No silent header edits after Phase 0: a mid-phase header change invalidates other members'
> builds on machines they cannot fix.

And, from the same table: **one writer per file at any time.**

## Freeze check (content identity)

`git hash-object` hashes the *committed blob*, so a CRLF checkout or a different line-ending setting cannot
produce a false mismatch:

| Header | Blob hash |
| --- | --- |
| `config_io.hpp` | `118d0921ffbdc7e506445438dad81e5c011ebf75` |
| `console_app.hpp` | `af282ac885161be648f2c1e038936cce1e66d584` |
| `frame_buffer.hpp` | `e68bd34d7ac7c145c9edbc07089e3784ee2bc080` |
| `glyphs.hpp` | `47554bed6b39477734572f52a2cebb197fcccc61` |
| `interpreter.hpp` | `bb7d888a59e9a801fa57e380d408fae5f98365ed` |
| `keys.hpp` | `1f6eeea8a1930ba9b15ea02fa62f29d65e106fa4` |
| `line_editor.hpp` | `5f229d78338e7116a13b95c19c8fa98da62746d5` |
| `parameters.hpp` | `7c7104da1b396a7f1105c311eeec877f5f65ad02` |
| `process.hpp` | `104ed6f89fa51fdacaa635fdb4d9c757dceb2343` |
| `renderer.hpp` | `2ad51698242f3dd746df989676c94e30d22e5437` |
| `scheduler.hpp` | `8e72c90a8adf24b8ace008bd44c816322ec429fe` |
| `shutdown.hpp` | `02bb0a2c722be76278b16a0fbf3fa3f2b00f750b` |
| `terminal.hpp` | `e9031d85b59aad0c52d9978e70be69dd2db4128e` |

Re-check with `git hash-object include/csopesy/*.hpp`. A mismatch means a contract header changed: either it
went through the protocol above — in which case regenerate this table and bump the marker *in the same commit* —
or it did not, in which case revert it. A tool that rewrites a contract header is making a contract change;
re-running a formatter does not exempt it.

These headers are also the layer guard's input: `scripts/check_layers.sh` fails the build on any upward or
cross-slice include among them (§3.1), on every push, on all three OSes (`.github/workflows/ci.yml`).
