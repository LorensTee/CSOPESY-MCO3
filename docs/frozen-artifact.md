# Frozen artifact record (T6.3)

The graded binary is **`frozen/csopesy.exe`**, the executable named by the committed `csopesy-quiz`
run configuration. This file records *which source produced it*, *how it was built*, and *its SHA-256*, so
that the §7 runbook's T-0:00 check is a real verification and not a formality.

`frozen/` is **gitignored** (`.gitignore:5`) and the binary is **never committed** — only this record is.

## The artifact

| | |
| --- | --- |
| Tag | `quiz-frozen` — ⚠️ **still at the 2026-09-27 evidence commit; not moved** |
| Source commit | `1c5a0b54c3bb71fffa0a63808594c41e20851eb2` (`feat: config implementation`, 2026-09-28; the first commit carrying the `config.txt` default layer, D19 — **not the tip**, see the note below) |
| Path | `frozen/csopesy.exe` |
| SHA-256 | `e525e675b16bb6c73b4f36c857c878841f44d66283e7fa0f734d8b2f37126265` |
| Size | 202 353 bytes |
| Build type | **Release** (`cmake --preset release`) |

This re-freeze replaces the 2026-09-27 artifact (source `c825138`, SHA-256 `ec86bb06…03e90b0`). Source
changed after that freeze — `include/csopesy/cli.hpp` (contracts v3.1), `src/entities/cli.cpp` and
`tests/unit/test_cli.cpp` gained the `config.txt` default layer (D19) — so the old artifact was stale by the
re-freeze rule below.

> **⚠️ Provisional entry — the source has moved past it (2026-09-28).** These bytes were built from
> `1c5a0b5`, which is **not** the source being submitted: commit `3419d4f` later changed the malformed-value
> warning string in `src/entities/cli.cpp`. The SHA-256 above therefore corresponds to no committed tip, the
> `quiz-frozen` tag has **not** been moved, and this entry must not be treated as the graded artifact. The
> binary does not embed `config.txt` (it is read at runtime from the working directory), so that file does not
> affect the hash. The T3.4 live gate below was run on the *previous* (`ec86bb06…`) bytes.
>
> **A re-freeze from the merged `main` tip is required before a graded take:** rebuild Release, recompute and
> record the SHA-256 and size, re-confirm the test count below, move the `quiz-frozen` tag to the evidence
> commit, and re-press the T3.4 gate on the new bytes.

### How it was built

```text
cmake --preset release
cmake --build --preset release
ctest --preset release          # 2/2 (unit 140 + smoke)
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
| SHA-256 unchanged after a `csopesy-quiz` **Run** press | ⏳ **pending for the `e525e675…` bytes** — the 2026-09-27 pass below applies to the superseded `ec86bb06…` artifact |

## T3.4 — the CLion graded-run gate (2026-09-27, on the superseded `ec86bb06…` bytes)

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
