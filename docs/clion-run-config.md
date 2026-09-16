# CLion Run/Debug configurations — the record (T0.5 Steps 3–6)

This file is the **evidence that the graded run configuration is real** (§2.4). It gets filled by the
owners running the T0.5 Step 5 gate on their own hardware — the gate is a hand-run checklist because no
unit test can observe raw-mode restoration, a live resize, or a console that isn't a terminal (§5.0, §6.1).

## The two committed configurations

Both live in `.idea/runConfigurations/` and are **tracked**, so run arguments cannot drift across the four
machines.

| | `csopesy-dev` | `csopesy-quiz` |
| --- | --- | --- |
| Type | CMake Application | Custom Build Application (see caveat below) |
| Executable / target | CMake target `csopesy` | `$PROJECT_DIR$/frozen/csopesy.exe` |
| Working dir | `$ProjectFileDir$` | `$ProjectFileDir$` |
| Program arguments | `--config=config/csopesy.ini` | `--config=config/csopesy.ini` |
| Before-launch `Build` | **present** (it is the dev config; Run may rebuild) | **absent** — nothing is built at Run time |
| Used for | developing, and the Step 5 terminal gate | the recorded/graded run |

**Why the quiz configuration is not a CMake target.** JetBrains documents that *"Build is the default
pre-launch step for CMake applications"*, so a CMake-Application config can relink at Run time — exactly
what the handout's *"no longer recompile the project when taking the quiz"* forbids. Splitting the two
configurations is the fix (§10, "Split `csopesy-dev` … from `csopesy-quiz`"): the dev config keeps the
normal CMake build, and the quiz config has no build step at all.

> **Caveat on `csopesy-quiz` — confirm in the GUI.** Its XML was authored by hand, and the
> `CustomBuildApplication` type id could **not** be verified on the machine that wrote it (the CLion
> distribution is compressed, so the type id is not discoverable offline). The plan documents a second
> accepted route: a CMake-Application configuration whose `Executable` is overridden to the frozen path
> **with its `Build` entry removed** (§2.4). **Either is acceptable to the gate** — but if CLion does not
> recognise the file, recreate the configuration through *Edit Configurations → + → Custom Build
> Application* with all build-target fields left empty, and commit what CLion writes. That version is
> authoritative; delete this one.

## Step 5 gate — per OS (fill the result cells)

Using **`csopesy-dev`**, press Run/Debug and confirm:

| Check | Linux (W1) | Windows (W2/W3) | macOS (W4) |
| --- | --- | --- | --- |
| (a) ANSI renders as layout, not literal escape codes | *to fill* | *to fill* | *to fill* |
| (b) `start_marquee` animates **while** the prompt accepts typing | *to fill* | *to fill* | *to fill* |
| (c) `--diag` reports `isTty=true` (POSIX) / VT enabled (Windows) | *to fill* | *to fill* | *to fill* |
| (d) window resize mid-animation repaints cleanly | *to fill* | *to fill* | *to fill* |
| Terminal switch chosen | *to fill* | *to fill* | *to fill* |
| CLion version / toolchain | *to fill* | *to fill* | *to fill* |
| Verified by (name, date) | *to fill* | *to fill* | *to fill* |

Resize works because **both backends re-read `Terminal::size()` every tick** (§3.8) rather than relying on
a platform resize event — so (d) exercises the same path on all three OSes.

### The terminal switch, per §2.4

| Option | Who uses it |
| --- | --- |
| **Emulate terminal in the output console** | W1 Linux, W4 macOS — and on Windows it is documented as supported with **MSVC + LLDB** (not GDB) |
| **Run in external console (Windows)** | W3 Nathan's Windows build — yields a real console handle for `_kbhit` / `SetConsoleMode` |

If a terminal check fails: try the other switch, then the **T0.5b** fallback
(`scripts/run_external.{sh,bat}`, which launches the already-built binary in a real terminal — the video
still shows Run/Debug in CLion initializing the program, which is what the handout asks).

> **Known tension, still open.** The terminal switch is stored *inside* the run configuration
> (`EMULATE_TERMINAL` / `USE_EXTERNAL_CONSOLE`), so one committed file cannot encode both the
> Linux/macOS choice and W3's external-console choice. The committed files currently set
> `EMULATE_TERMINAL="true"`, which suits three of the four members. If W3 has to flip it, the tracked file
> goes dirty. Decide one of: keep the committed default and have W3 suppress the local change
> (`git update-index --skip-worktree .idea/runConfigurations/csopesy-dev.xml`), or commit a third
> Windows-specific configuration. **Not resolved by the plan; resolve before the freeze.**

## `csopesy-quiz` Before-launch list (Step 5, captured not-run)

Confirm **without running it** (the frozen artifact does not exist until T2.6/T6.3):

*Before-launch list as shown in the GUI — to capture.*

## T6.3 Step 0 — re-verification once the artifact exists

Recorded here when it happens: press Run on `csopesy-quiz` and confirm no `Build` entry runs, the process
starts the binary in `frozen/`, and the binary's SHA-256 is **unchanged** after the press (a relink would
change it and invalidate the frozen claim).
