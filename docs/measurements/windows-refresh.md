# windows-refresh.md — the **refresh** sweep on windows

* **Recorder (owner):** W3 Nathan — T3.3 (Windows sweep)
* **Status:** **COMPLETE (T3.3, 2026-09-27)** — sweep run against the frozen binary only (D14).
* **Artifact under test:** `frozen/csopesy.exe` — SHA-256
  `ec86bb06a3e5614d3924c5afce01cab67f8e442e0f3e0f8867905e49303e90b0` (source `c825138`, T6.3 re-freeze)

The sweep ran the frozen binary in a real Win32 console created with `CREATE_NEW_CONSOLE`, changing only
the program arguments `--refresh-ms=M` between takes (`--poll-ms=10`). No rebuild and no re-copied
executable between takes; the SHA-256 was re-checked unchanged at the end.

**Machine / OS:** Windows 11 `[Version 10.0.26200.8875]`, x86_64
**Terminal / console:** real Win32 console (the external-console backend the graded `csopesy-quiz` uses)
**Recorded by / date:** W3 (this session), 2026-09-27

## refresh table

`Fluid?` / `Tearing?` / `Typing delay?` are answered from observations of the console screen buffer; the
band was set to a 200-character ASCII text so a moving band is always visible.

| Take | `refreshMs` | Fluid? | Tearing / flicker? | Typing delay? | Notes |
| --- | --- | --- | --- | --- | --- |
| 1 | 1 | Yes — fastest this console sustains | No | No (~1.8 ms) | ~64 band changes/s; console-write bound, not `refreshMs` bound |
| 2 | 16 | Yes | No | No (~1.4 ms) | ~40 band changes/s |
| 3 | 50 | Yes | No | No (~1.2 ms) | ~16 band changes/s |
| 4 | 100 | Yes, with slight stepping | No | No (~1.4 ms) | ~9 band changes/s (shipped default) |
| 5 | 250 | Choppy, clearly stepped | No | No (~1.7 ms) | ~4 band changes/s |
| 6 | 1000 | One step per second | No | No (~1.6 ms) | 1 band change/s |
| 7 | 10000 | Motion effectively frozen | No | No (~1.6 ms) | 1 band change in a 12 s window |

**Recommended `refreshMs` for this machine:** **16** — the band is fluid and no longer console-write bound;
`50` and `100` remain usable if fewer redraws are wanted.
**Observed lower/upper thresholds** (where tearing starts / where motion stops looking fluid): **tearing
never observed at any value** — each frame is one complete `write()` and only the band row changes, so there
is no partial-frame tear. Motion stops looking fluid above ~100 ms (≤ ~9 fps); at 250 ms and above the
stepping is obvious. Keystroke echo did **not** track `refreshMs` at any value (all ~1–2 ms), which is the
expected `dirty_`/condition-variable behaviour.

## Honest limitation

Observed thresholds and recommended values only — **not** per-keystroke latency figures (D4). The screen
buffer is what was sampled, not a human's eyeball; `T3.4` supplies the live CLion external-console check.

## Raw transcript

The instrument is the T5.1 real-console harness (temp-only, not committed): spawn with `CREATE_NEW_CONSOLE`,
`AttachConsole(pid)`, read rows with `ReadConsoleOutputCharacterW`, sample the band row every ~6 ms and count
distinct positions. Representative output rows:

```text
refreshMs=1     window=2.51s band_changes=161 observed_fps=64.21 frame_consistent=true echo_ms=1.76
refreshMs=16    window=2.50s band_changes=100 observed_fps=39.96 frame_consistent=true echo_ms=1.43
refreshMs=50    window=2.51s band_changes= 40 observed_fps=15.95 frame_consistent=true echo_ms=1.15
refreshMs=100   window=2.50s band_changes= 23 observed_fps= 9.19 frame_consistent=true echo_ms=1.37
refreshMs=250   window=2.51s band_changes= 10 observed_fps= 3.99 frame_consistent=true echo_ms=1.68
refreshMs=1000  window=5.00s band_changes=  5 observed_fps= 1.00 frame_consistent=true echo_ms=1.61
refreshMs=10000 window=12.01s band_changes= 1 observed_fps= 0.08 frame_consistent=true echo_ms=1.58
```
