# Frozen artifact record (T6.3)

The graded binary is **`frozen/csopesy.exe`**, the executable named by the committed `csopesy-quiz`
run configuration. This file records *which source produced it*, *how it was built*, and *its SHA-256*, so
that the §7 runbook's T-0:00 check is a real verification and not a formality.

`frozen/` is **gitignored** (`.gitignore:5`) and the binary is **never committed** — only this record is.

## The artifact

| | |
| --- | --- |
| Tag | `quiz-frozen` |
| Source commit | `c825138c4d06074e809921b066b8e58e132aec4a` (`main`, 2026-09-27; the tip carrying the pre-freeze fixes for issues #1 and #3) |
| Path | `frozen/csopesy.exe` |
| SHA-256 | `ec86bb06a3e5614d3924c5afce01cab67f8e442e0f3e0f8867905e49303e90b0` |
| Size | 187 519 bytes |
| Build type | **Release** (`cmake --preset release`) |

This re-freeze replaces the 2026-09-24 artifact (source `4c5e8cf`, SHA-256 `a9cbc09e…1fd4f15f`). Source
changed after that freeze — `src/features/commands/interpreter.cpp` and `src/features/marquee/renderer.cpp`
both gained the pre-freeze fixes — so the old artifact was stale by the re-freeze rule below and the tag was
moved to the evidence commit in the same change.

### How it was built

```text
cmake --preset release
cmake --build --preset release
ctest --preset release          # 2/2 (unit 128 + smoke)
cp build/release/csopesy.exe frozen/csopesy.exe
```

| | |
| --- | --- |
| OS | Windows 11 `[Version 10.0.26200.8875]`, x86_64 |
| Compiler | GCC 14.2.0 (MinGW-W64 UCRT, Brecht Sanders r2), `-O2` |
| CMake / Ninja | CMake 4.4.3, Ninja 1.12.1 |
| Warnings | **0** under `-Wall -Wextra` |

The Release build was chosen over Debug so the graded run and the D14 measurement sweep run an optimized
binary. The Release `ctest` run above re-confirms the suite on this exact optimization level. The two
pre-freeze fixes are present in the bytes: the frozen binary over `--no-tty` rejects appended text
(`help extra` → `Usage: help`, `exit extra` → `Usage: exit`) and the interactive frame never writes the
bottom-right cell (verified live in T3.4 below).

## §7 runbook pre-flight

| Pre-flight item | Status |
| --- | --- |
| `frozen/` present on disk and correctly **absent** from git | ✅ (`git check-ignore frozen/csopesy.exe` → `.gitignore:5`) |
| `csopesy-quiz` has **no** Build step | ✅ the shared entry is CLion 2026.2.3's own output (`type="CLionExternalRunConfiguration"`) with the Build task removed — the committed XML carries **no** `<method>` block, so a Run press cannot recompile anything (see `docs/clion-run-config.md`) |
| `csopesy-quiz` Program arguments empty for graded takes | ✅ no `PROGRAM_PARAMS` attribute on the shared entry (graded takes pass nothing; measurement takes set `--poll-ms=N --refresh-ms=M` per D14) |
| Executable path is the frozen binary | ✅ `RUN_PATH="$PROJECT_DIR$/frozen/csopesy.exe"` (plus the `csopesy-frozen` custom build target in the committed `.idea/customTargets.xml`) |
| Binary starts and exits 0 on the `--no-tty` path | ✅ `help` → six command lines; `exit` → one goodbye, rc=0; also with `--refresh-ms=25 --poll-ms=5` |
| SHA-256 unchanged after a `csopesy-quiz` **Run** press | ✅ **T3.4 passed 2026-09-27** — see below |

## T3.4 — the CLion graded-run gate (2026-09-27)

Run on the Windows machine with CLion 2026.2.3 (`CL-262.10968.117`), on the tracked shared `csopesy-quiz`
entry, in the recorded **external console** (`USE_EXTERNAL_CONSOLE="true"`, `EMULATE_TERMINAL="false"`).

| Check | Result |
| --- | --- |
| Shared entry loads and is the graded config | ✅ `type="CLionExternalRunConfiguration"`, `RUN_PATH="$PROJECT_DIR$/frozen/csopesy.exe"`, `TARGET_NAME/CONFIG_NAME="csopesy-frozen"` |
| Before-launch Build task | ✅ absent (no `<method>` block); the Run press rebuilt and relinked nothing |
| SHA-256 **before** the Run | `ec86bb06a3e5614d3924c5afce01cab67f8e442e0f3e0f8867905e49303e90b0` |
| SHA-256 **after** the Run | `ec86bb06a3e5614d3924c5afce01cab67f8e442e0f3e0f8867905e49303e90b0` (unchanged) |
| Runs in a real console | ✅ the external console's screen buffer showed the live marquee + prompt |
| Marquee animates while the prompt accepts typing | ✅ `CLION-RUN-GATE` band: 19 distinct positions in 2.0 s; 20 typed keystrokes echoed in ~5 ms |
| No frame accumulation | ✅ exactly **one** `Welcome to CSOPESY!` on the whole visible screen after 2 s of animation |
| Resize mid-animation repaints cleanly | ✅ resized to 90×30 while running: first row exactly 90 cells, still exactly one welcome row; restored to 120×30 |
| Exit while animating | ✅ `exit` → process exit code 0 |

The live checks were read back from the launched process's real console screen buffer (the T5.1 method),
not a pipe or a mock. After the press the run configuration and the local workspace were released; the
frozen byte stream did not change.

## Open item to resolve during the T3.4 press — resolved

`docs/clion-run-config.md` records the terminal-switch tension (`EMULATE_TERMINAL` vs
`USE_EXTERNAL_CONSOLE`). The T3.4 press settled it: the shared `csopesy-quiz` entry runs in the
**external console** (`USE_EXTERNAL_CONSOLE="true"`), which is the Windows backend's requirement for
`_kbhit`/`SetConsoleMode`. The switch lives in the run configuration XML only — flipping it does not change
the binary or its SHA-256, so the freeze above stays valid.

## Re-freeze rule

If any tracked source file changes after this record, the artifact is stale: rebuild, recompute the SHA-256
here, and move the `quiz-frozen` tag in the same commit. A binary whose hash does not match this table must
not be used for a graded take (§7 T-0:00).
