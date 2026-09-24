# CLion Run/Debug configurations — the record (T0.5 Steps 3–6)

This file is the **evidence that the graded run configuration is real** (§2.4). It gets filled by the
owners running the T0.5 Step 5 gate on their own hardware — the gate is a hand-run checklist because no
unit test can observe raw-mode restoration, a live resize, or a console that isn't a terminal (§5.0, §6.1).

> **v3.0 update (T0.6, 2026-09-22).** The config layer is gone (D1), so both configurations now pass **no**
> program arguments and `config/csopesy.ini` no longer exists. The `--diag` check below is replaced by watching
> the program start (a Windows VT-enable failure is now a loud startup error, D5). And the live gate this file
> records is now **T3.4, not T0.5** — no part of it could be observed before features existed, so v3 moved it
> behind them (`gpt-v12`; plan §5). The rows below stay until someone fills them there.
> For the PPT's refresh/polling sweep, a *measurement take* sets `--poll-ms=N --refresh-ms=M` in the run
> configuration and presses Run again — never a rebuild (D14, plan §1.2).

## The two committed configurations

Both live in `.idea/runConfigurations/` and are **tracked**, so run arguments cannot drift across the four
machines.

| | `csopesy-dev` | `csopesy-quiz` |
| --- | --- | --- |
| Type | CMake Application | Custom Build Application (display name; type id `CLionExternalRunConfiguration` — see caveat below) |
| Executable / target | CMake target `csopesy` | `$PROJECT_DIR$/frozen/csopesy.exe` |
| Working dir | `$ProjectFileDir$` | `$ProjectFileDir$` |
| Program arguments | *(none)* | *(none)* — a graded take sets nothing; a measurement take sets `--poll-ms=N --refresh-ms=M` (D14) |
| Before-launch `Build` | **present** (it is the dev config; Run may rebuild) | **absent** — nothing is built at Run time |
| Used for | developing, and the Step 5 terminal gate | the recorded/graded run |

**Why the quiz configuration is not a CMake target.** JetBrains documents that *"Build is the default
pre-launch step for CMake applications"*, so a CMake-Application config can relink at Run time — exactly
what the handout's *"no longer recompile the project when taking the quiz"* forbids. Splitting the two
configurations is the fix (§10, "Split `csopesy-dev` … from `csopesy-quiz`"): the dev config keeps the
normal CMake build, and the quiz config has no build step at all.

> **Caveat on `csopesy-quiz` — the committed type id is not registered by CLion 2026.2.3 (checked
> 2026-09-24).** The XML was authored by hand and declares `type="CustomBuildApplication"`, but that id
> occurs **nowhere** in the installed CLion 2026.2.3: not in any `.jar` entry, not in any plain file. The
> entry the GUI actually offers under the name *Custom Build Application* is
> `com.jetbrains.cidr.cpp.execution.external.run.CLionExternalRunConfigurationType`
> (`intellij.clion.execution.jar`), and its `ConfigurationTypeBase` id is
> **`CLionExternalRunConfiguration`** (verified by disassembling the constructor; the display name is the
> bundle key `external.run.configuration.name` = "Custom Build Application" in
> `CLionExecutionBundle.properties`). Its executable is read through `ExecutableData.loadExternal` (flat
> attribute `RUN_PATH`, or a configured *Custom Build Target*) — **not** the hand-written `EXECUTABLE`
> attribute — so the rest of the quiz XML's attribute set should be treated as unverified too. (For contrast,
> `csopesy-dev`'s `type="CMakeRunConfiguration"` **is** the registered id
> — `CMakeAppRunConfigurationType`, display name "Application" — so the dev config is fine.)
> So CLion will most likely show the committed `csopesy-quiz` as an
> unknown/invalid configuration. This is a **T3.4 prerequisite**: recreate it in the GUI via *Edit
> Configurations → + → Custom Build Application* with an empty custom build target and the executable set to
> `frozen/csopesy.exe`, no build step, and commit what CLion writes as the authoritative
> file. The plan's second accepted route — a CMake Application config whose `Executable` is overridden to
> the frozen path **with its `Build` entry removed** (§2.4) — remains valid too.

## Step 5 gate — per OS (fill the result cells)

Using **`csopesy-dev`**, press Run/Debug and confirm:

| Check | Linux (W1) | Windows (W2/W3) | macOS (W4) |
| --- | --- | --- | --- |
| (a) ANSI renders as layout, not literal escape codes | *to fill* | *to fill* | *to fill* |
| (b) `start_marquee` animates **while** the prompt accepts typing | *to fill* | *to fill* | *to fill* |
| (c) The program animates while accepting typing — i.e. raw mode is on and VT output works. (v3.0 removed `--diag`; on Windows a VT-enable failure is a **loud startup error**, D5) | *to fill* | *to fill* | *to fill* |
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
> Windows-specific configuration. **Not resolved by the plan.** T6.3 (the freeze) has since landed
(`docs/frozen-artifact.md`), so this is now a decision to settle during the T3.4 GUI press; the switch
lives only in the run-config XML, so flipping it changes neither `frozen/csopesy.exe` nor its SHA-256.

## `csopesy-quiz` Before-launch list (Step 5, captured not-run)

T6.3 is complete (`docs/frozen-artifact.md`, tag `quiz-frozen`), so the frozen artifact now exists and this
section can be filled by the T3.4 GUI press rather than captured from the editor alone. Confirm:

*Before-launch list as shown in the GUI — to capture.*

## T6.3 Step 0 — re-verification once the artifact exists

Recorded here when it happens: press Run on `csopesy-quiz` and confirm no `Build` entry runs, the process
starts the binary in `frozen/`, and the binary's SHA-256 is **unchanged** after the press (a relink would
change it and invalidate the frozen claim).
