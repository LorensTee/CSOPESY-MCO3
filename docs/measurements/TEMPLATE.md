# TEMPLATE — one D14 take table

Copy this shape into a per-machine file. Fill a row only from a frozen-binary take (see `README.md`).

**Artifact under test (T6.3):** `frozen/csopesy.exe` — SHA-256: **PENDING (T6.3)**
**Machine / OS:** PENDING
**Terminal / console:** PENDING
**Recorded by / date:** PENDING

## Refresh sweep (`--refresh-ms=`, or the `set_speed` runtime equivalent)

Observe the band and the echo at each value. "Tearing" means visible tearing or flicker; "typing delay" means
the keystroke echo visibly lags the keypress.

| Take | `refreshMs` | Fluid? | Tearing / flicker? | Typing delay? | Notes |
| --- | --- | --- | --- | --- | --- |
| 1 | 1 | PENDING | PENDING | PENDING | |
| 2 | 16 | PENDING | PENDING | PENDING | |
| 3 | 50 | PENDING | PENDING | PENDING | |
| 4 | 100 | PENDING | PENDING | PENDING | |
| 5 | 250 | PENDING | PENDING | PENDING | |
| 6 | 1000 | PENDING | PENDING | PENDING | |
| 7 | 10000 | PENDING | PENDING | PENDING | |

**Recommended `refreshMs` for this machine:** PENDING
**Observed lower/upper thresholds** (where tearing starts / where motion stops looking fluid): PENDING

## Polling sweep (`--poll-ms=`)

`pollingMs` is the maximum idle wait when nothing is ready; it does not delay keystrokes (each keystroke
notifies the condition variable). Record idle CPU and the Ctrl+C latency.

| Take | `pollingMs` | Idle CPU (process) | Idle CPU (per thread) | Ctrl+C latency | Key-echo latency | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 1 | PENDING | PENDING | PENDING | PENDING | |
| 2 | 5 | PENDING | PENDING | PENDING | PENDING | |
| 3 | 10 | PENDING | PENDING | PENDING | PENDING | |
| 4 | 50 | PENDING | PENDING | PENDING | PENDING | |
| 5 | 1000 | PENDING | PENDING | PENDING | PENDING | |

**Recommended `pollingMs` for this machine:** PENDING
**Observed wakeup effect (two waiters):** PENDING

## Honest limitation

State it wherever these numbers are published: these are **observed thresholds from a manual sweep**, not
per-keystroke latency measurements. v3 removed the in-process telemetry (D4), so the sweep reports what a
person saw and the values they recommend — never a latency figure it cannot measure.

## Raw transcript

Paste the terminal transcript/screenshot reference for this machine here. PENDING (T6.3)
