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

> **Resolved 2026-09-24: `csopesy-quiz` is now CLion 2026.2.3's own output.** The file was recreated in
> the GUI (*Edit Configurations → + → Custom Build Application*, executable `frozen/csopesy.exe`,
> working directory project root, no program arguments, **Run in external console**, Before-launch **Build
> removed**) and shared to `.idea/runConfigurations/csopesy-quiz.xml` via the GUI's *Store as project
> file* mechanism (which moves the entry out of the local `workspace.xml` `<RunManager>` into the tracked
> file wrapped in `<component name="ProjectRunConfigurationManager">` — content otherwise identical).
> The committed XML is that generated entry with only the Build task removed: `type` is the registered id
> **`CLionExternalRunConfiguration`**, the executable travels as `RUN_PATH="$PROJECT_DIR$/frozen/csopesy.exe"`
> (plus `TARGET_NAME="csopesy-frozen"` / `CONFIG_NAME="csopesy-frozen"`, whose definition lives in the
> likewise-committed `.idea/customTargets.xml`), `USE_EXTERNAL_CONSOLE="true"`, and there is **no**
> `<method>` (Before-launch) block — so a Run press cannot recompile anything (handout D10, plan §T3.4).
> Correction to the handoff: the GUI entry as first created still carried
> `<option name="CLION.EXTERNAL.BUILD" enabled="true" />` (proven by the `workspace.xml` bytes) — the
> "no Build" state was *not* yet true and was fixed by removing the task, not assumed. Owner still owes
> the final confirmation in the GUI: the shared entry loads without warnings, and a Run press leaves the
> frozen SHA-256 unchanged (T3.4 gate, `docs/frozen-artifact.md`).

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

> **Settled 2026-09-24 for `csopesy-quiz`; still open for `csopesy-dev`.** The shared quiz entry now sets
> `USE_EXTERNAL_CONSOLE="true"` / `EMULATE_TERMINAL="false"` — the external-console choice the manual run
> proved on Windows (marquee + prompt + typing + resize all worked). `csopesy-dev` still sets
> `EMULATE_TERMINAL="true"`, which suits Linux/macOS; if a Windows dev press needs the external console,
> keep that flip local (`git update-index --skip-worktree`) rather than committing a third file.

## `csopesy-quiz` Before-launch list (Step 5, captured not-run)

T6.3 is complete (`docs/frozen-artifact.md`, tag `quiz-frozen`), so the frozen artifact now exists. The
shared `csopesy-quiz` entry above is CLion 2026.2.3's own output with the Build task removed — the
committed XML carries **no** `<method>` (Before-launch) block, so the press cannot rebuild. Still owed by
the owner in the GUI (T3.4 gate): press Run on the *shared* entry and confirm the frozen SHA-256 is
unchanged.

## T6.3 Step 0 — re-verification once the artifact exists

Recorded here when it happens: press Run on `csopesy-quiz` and confirm no `Build` entry runs, the process
starts the binary in `frozen/`, and the binary's SHA-256 is **unchanged** after the press (a relink would
change it and invalidate the frozen claim).
