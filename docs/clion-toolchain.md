# CLion toolchains — pinned per member (T0.5 Step 2)

CLion's *default* toolchain may not be the one that built the frozen binary (on Windows it prefers the
bundled MinGW over MSVC), so the pairing is **pinned and recorded here**, not assumed.

Fill the version columns from the owner's own machine: *Settings → Build, Execution, Deployment →
Toolchains* for the compiler, *Help → About* for the CLion build.

| Member | OS | Toolchain (pinned) | Compiler version | CLion version | Recorded by |
| --- | --- | --- | --- | --- | --- |
| W1 Lorens | Linux | system GCC/clang + Ninja | GCC 16.2.1 | *to fill* | *to fill* |
| W2 Byron | Linux | GCC + Ninja | *to fill* | *to fill* | *to fill* |
| W2 Byron | Windows | **MSVC** | *to fill* | *to fill* | *to fill* |
| W3 Nathan | Windows | **MSVC** | *to fill* | *to fill* | *to fill* |
| W4 Kim | macOS | **AppleClang + Ninja** | *to fill* | *to fill* | *to fill* |
| W4 Kim | Windows | MSVC | *to fill* | *to fill* | *to fill* |

Measured on the W1 Linux machine while writing this file, for reference:

| Tool | Version | Note |
| --- | --- | --- |
| GCC / G++ | 16.2.1 20260819 (Red Hat 16.2.1-2) | system compiler |
| CMake | 4.3.0 | system; CLion ≥ 3.21 bundles its own |
| Ninja | 1.13.2 | system |
| clang++ | not installed | — |

## Presets are read-only in CLion (why local edits don't leak)

`CMakePresets.json` is committed and **shared**. CLion loads presets read-only and writes any local
override to `CMakeUserPresets.json`, which is **gitignored** — so a member's machine-specific toolchain
path or extra cache variable cannot land in the shared file. Preset profiles are **disabled by default**;
each member enables them once via *Settings → Build, Execution, Deployment → CMake → Use CMake presets*.

Configure presets, and what they select:

| Preset | Generator | Build dir | Notes |
| --- | --- | --- | --- |
| `debug` | Ninja | `build/debug` | the development default |
| `release` | Ninja | `build/release` | used for the measured runs |
| `windows-vs` | Visual Studio 17 2022 (x64) | `build/vs` | only listed where that generator exists |

`buildPresets` and `testPresets` (`debug`, `release`, `windows-vs`) are also defined, so
`cmake --build --preset debug` and `ctest --preset debug` work without naming a build directory.

CMake floor is **3.21** (presets schema v3 + the VS 17 2022 generator both require it) — see §T0.2.
