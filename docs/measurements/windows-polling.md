# windows-polling.md — the **polling** sweep on windows

* **Recorder (owner):** W3 Nathan — T3.3 (Windows sweep)
* **Status:** **COMPLETE (T3.3, 2026-09-27)** — sweep run against the frozen binary only (D14).
* **Artifact under test:** `frozen/csopesy.exe` — SHA-256
  `ec86bb06a3e5614d3924c5afce01cab67f8e442e0f3e0f8867905e49303e90b0` (source `c825138`, T6.3 re-freeze)

The sweep ran the frozen binary in a real Win32 console, changing only `--poll-ms=N` between takes
(`--refresh-ms=100`). "Idle" means the marquee was never started. Process CPU was read with
`GetProcessTimes` over a 30 s window; the input thread blocks in `ReadConsoleInput`, so the timed-wait
worker accounts for essentially all of it.

**Machine / OS:** Windows 11 `[Version 10.0.26200.8875]`, x86_64
**Terminal / console:** real Win32 console (the external-console backend the graded `csopesy-quiz` uses)
**Recorded by / date:** W3 (this session), 2026-09-27

## polling table

`Ctrl+C` latency is the time from injecting Ctrl+C to process exit; `Key-echo` latency is the time from
injecting a printable key to the prompt row showing it.

| Take | `pollingMs` | Idle CPU (process) | Idle CPU (per thread) | Ctrl+C latency | Key-echo latency | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 1 | 0.31% (93.75 ms / 30 s) | worker ≈ 0.31%, input thread 0 (blocked) | ~3.3 ms | ~1.9 ms | OS timer quantum (~15.6 ms) coalesces the wakeups |
| 2 | 5 | 0.42% (125.0 ms / 30 s) | worker ≈ 0.42%, input thread 0 | ~4.3 ms | ~1.6 ms | same coalesced wakeup rate as take 1 |
| 3 | 10 | 0.36% (109.4 ms / 30 s) | worker ≈ 0.36%, input thread 0 | ~3.4 ms | ~1.8 ms | shipped default; same wakeup rate |
| 4 | 50 | 0.05% (15.6 ms / 30 s) | worker ≈ 0.05%, input thread 0 | ~3.2 ms | ~1.3 ms | ~20 wakeups/s |
| 5 | 1000 | 0.00% (below the 15.6 ms quantum) | worker ≈ 0, input thread 0 | ~3.2 ms | ~2.0 ms | ~1 wakeup/s |

**Recommended `pollingMs` for this machine:** **10** (the shipped default) — idle CPU stays under 1% and
Ctrl+C/echo are immediate. `50` or `1000` cut the wakeups further with no keystroke-latency cost.
**Observed wakeup effect (two waiters):** any `pollingMs` below the ~15.6 ms Windows timer quantum (1, 5,
10 ms) wakes at the same effective rate (~64/s), so their CPU is indistinguishable; the effect only appears
once the timeout exceeds the quantum (50 ms → ~20/s, 1000 ms → ~1/s). The input thread is a blocked waiter
and consumes no measurable CPU, so the idle cost is the worker's timed wait, not a doubled two-thread cost.

## Honest limitation

Observed thresholds and recommended values only — **not** per-keystroke latency figures (D4). `GetProcessTimes`
has a 15.625 ms quantum, so values below ~0.05% are at or under the measurement floor.

## Raw transcript

Same T5.1 real-console instrument as `windows-refresh.md`. Representative output rows:

```text
pollingMs=1    idle_cpu= 93.75ms/30s (0.3125%)  ctrl_c=3.28ms  echo=1.94ms
pollingMs=5    idle_cpu=125.00ms/30s (0.4167%)  ctrl_c=4.25ms  echo=1.61ms
pollingMs=10   idle_cpu=109.38ms/30s (0.3646%)  ctrl_c=3.37ms  echo=1.80ms
pollingMs=50   idle_cpu= 15.62ms/30s (0.0521%)  ctrl_c=3.24ms  echo=1.27ms
pollingMs=1000 idle_cpu=  0.00ms/30s (0.0000%)  ctrl_c=3.19ms  echo=1.96ms
```
