# Frozen artifact record (T6.3)

The graded binary is **`frozen/csopesy.exe`**, the executable named by the committed `csopesy-quiz`
run configuration. This file records *which source produced it*, *how it was built*, and *its SHA-256*, so
that the §7 runbook's T-0:00 check is a real verification and not a formality.

`frozen/` is **gitignored** (`.gitignore:5`) and the binary is **never committed** — only this record is.

## The artifact

| | |
| --- | --- |
| Tag | `quiz-frozen` |
| Source commit | `4c5e8cf047bbc4878dcaf15391d9e5ad0270a245` (`main`, 2026-09-24; the last **code** commit is `97ce13a`, the two commits above it are documentation only) |
| Path | `frozen/csopesy.exe` |
| SHA-256 | `a9cbc09e7ffbefda9f2e3714845995e0320fd349ac47fe7f531d15ec1fd4f15f` |
| Size | 186 436 bytes |
| Build type | **Release** (`cmake --preset release`) |

### How it was built

```text
cmake --preset release
cmake --build --preset release
ctest --preset release          # 2/2 (unit 117 + smoke)
cp build/release/csopesy.exe frozen/csopesy.exe
```

| | |
| --- | --- |
| OS | Windows 11 `[Version 10.0.26200.8875]`, x86_64 |
| Compiler | GCC 14.2.0 (MinGW-W64 UCRT, Brecht Sanders r2), `-O2` |
| CMake / Ninja | CMake 3.30.4, Ninja 1.12.1 |
| Warnings | **0** under `-Wall -Wextra` |

The Release build was chosen over Debug so the graded run and the D14 measurement sweep run an optimized
binary. The source is identical to the Debug tree that was verified on all three OSes; the Release `ctest`
run above re-confirms the suite on this exact optimization level.

## §7 runbook pre-flight

| Pre-flight item | Status |
| --- | --- |
| `frozen/` present on disk and correctly **absent** from git | ✅ (`git check-ignore frozen/csopesy.exe` → `.gitignore:5`) |
| `csopesy-quiz` has **no** Build step | ✅ `CustomBuildApplication`, `<configuration …>` carries no build entry |
| `csopesy-quiz` Program arguments empty for graded takes | ✅ `PROGRAM_PARAMS=""` |
| Executable path is the frozen binary | ✅ `EXECUTABLE="file://$PROJECT_DIR$/frozen/csopesy.exe"` |
| Binary starts and exits 0 on the `--no-tty` path | ✅ `help` → six command lines; `exit` → one goodbye, rc=0; also with `--refresh-ms=25 --poll-ms=5` |
| SHA-256 unchanged after a `csopesy-quiz` **Run** press | ⬜ **T3.4 hand-run gate — not yet run (needs the CLion GUI on the owner's Windows hardware)** |

## Open item to resolve during the T3.4 press

`docs/clion-run-config.md` records a known tension: the terminal switch (`EMULATE_TERMINAL` vs
`USE_EXTERNAL_CONSOLE`) lives *inside* the run configuration, and the committed `csopesy-quiz.xml` currently
sets `EMULATE_TERMINAL="true"`. On Windows the backend needs a **real console handle** for `_kbhit` /
`SetConsoleMode`; the doc's own guidance is that W3's Windows build uses **Run in external console**. The
T3.4 press is where this is settled: if the emulated terminal fails the loud VT startup check (D5), either
flip `csopesy-quiz.xml` to `USE_EXTERNAL_CONSOLE="true"` (a tracked change, acceptable because this file *is*
the Windows graded config) or use the T0.5b external-launch fallback. **Flipping the switch does not change
this binary or its SHA-256** — only the run configuration XML — so the freeze above stays valid.

## Re-freeze rule

If any tracked source file changes after this record, the artifact is stale: rebuild, recompute the SHA-256
here, and move the `quiz-frozen` tag in the same commit. A binary whose hash does not match this table must
not be used for a graded take (§7 T-0:00).
