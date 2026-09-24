# docs/measurements/ — the D14 sweep tables (T1.4, T3.3, T5.3)

Synthesis owner: **W4 (Kim)** — final table interpretation, `T5.3`.
Per-machine recording owners: **W1 (Lorens) → Linux (T1.4)**, **W3 (Nathan) → Windows (T3.3)**,
**W4 (Kim) → macOS**.

## Hard rule — the frozen binary only

The v3 plan measures the **frozen** artifact (`T6.3`, D14). Nothing in this directory is filled from a unit
test, a `FakeTerminal`, a synthetic calculation, a debug-build timing, an estimate, or an invented number. A
take is valid only when it was produced by pressing Run on the frozen binary and changing **only** the run
configuration's Program arguments between takes — never a rebuild, never a re-copied executable.

Until `T6.3` exists, every table in this directory says **PENDING (T6.3)**. An honest empty cell is worth more
than a plausible number.

## The frozen-binary workflow (D14, verbatim shape)

```text
For each take (one row of the table):
  1. In the IDE, set the run configuration's Program arguments to  --poll-ms=N --refresh-ms=M
     (csopesy-quiz: Executable = the frozen artifact, Before-launch has NO Build step).
  2. Press Run/Debug.  The frozen binary starts with those two values and nothing else changed.
  3. Observe: is the band fluid? does it tear or flicker? does typing echo lag?  Record the observation
     and the terminal/hardware it was made on.
  4. Change ONLY the arguments for the next take.
  5. Never edit source, never rebuild, never re-copy the executable.
```

Record the artifact's SHA-256 once per machine (from `docs/frozen-artifact.md`, `T6.3`) and confirm it is
unchanged at the end of the sweep. That is the evidence that every row describes the same build.

## Files

| File | Owner | Status |
| --- | --- | --- |
| `linux-refresh.md` / `linux-polling.md` | W1 Lorens (T1.4) | PENDING (T6.3) |
| `windows-refresh.md` / `windows-polling.md` | W3 Nathan (T3.3) | PENDING (T6.3) |
| `macos-refresh.md` / `macos-polling.md` | W4 Kim | PENDING (T6.3) |
| `TEMPLATE.md` | — | the row shape to copy |
| this file | W4 Kim | `T5.3` synthesis + the honest limitation |

## Synthesis rules (T5.3)

* Publish **observed thresholds and recommended values** — for the machine the take was made on.
* **State the honest limitation where the numbers appear:** these are *observed* thresholds from a manual
  sweep, **not** per-keystroke latency measurements. v3 has no in-process telemetry by design (D4).
* Report idle CPU **per thread** as well as per process: the architecture runs two waiters (input + marquee),
  and the doubling is the price of the professor-mandated two-thread design (plan risk 24).
* `pollingMs` is the only timer in either thread; a take that varies it should note the wakeup effect.
* Do not average across machines into one "recommended" number — the plan asks for a baseline **per machine**.

## Status

* **2026-09-24:** structure created by W4 as `T5.3` scaffolding. No values recorded; all tables PENDING `T6.3`.
