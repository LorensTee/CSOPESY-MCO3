CSOPESY MCO3 — Marquee Console
==============================

Members
-------
  W1  Lorens   (Linux)
  W2  Byron    (Linux + Windows)   <- contracts, build, CLI, interpreter
  W3  Nathan   (Windows)
  W4  Kim      (macOS + Windows)

Entry file
----------
  src/main.cpp                      function main()
  src/app/console_app.cpp           class csopesy::ConsoleApp  (owns the run loop)

Build (no config file, no extra dependencies beyond a C++17 compiler + CMake 3.21+)
----------------------------------------------------------------------------------
  POSIX (Linux / macOS):
      cmake --preset debug
      cmake --build --preset debug
      ctest  --preset debug

  Windows (Visual Studio 17 2022):
      cmake --preset windows-vs
      cmake --build --preset windows-vs
      ctest  --preset windows-vs

  Release build (the configuration used for the submitted artifact):
      cmake --preset release && cmake --build --preset release

Run
---
  POSIX:    ./build/debug/csopesy
  Windows:  build\vs\Debug\csopesy.exe

  Plain line mode (no raw mode, no ANSI, no animation, no worker thread; dev/CI only):
      csopesy --no-tty

Program arguments (there is NO config file)
-------------------------------------------
  --no-tty            plain line mode; dev/CI only, never used in the graded run
  --refresh-ms=N      marquee refresh in ms; clamped to [1, 10000] and reported
  --poll-ms=N         max idle wait in ms;   clamped to [1, 1000]  and reported

  `--flag=value` is the only accepted form. An unknown flag or a malformed value prints a
  warning and keeps the default: a typo never aborts startup. Precedence is
  runtime command > CLI flag > built-in default.

Runtime commands (case-sensitive; type `help` for the same list)
----------------------------------------------------------------
  help                        displays the commands and their descriptions
  start_marquee               starts the marquee animation
  stop_marquee                stops the marquee animation
  set_text <text>             sets the marquee text (printable ASCII only)
  set_speed <milliseconds>    sets the refresh interval (clamped to [1, 10000])
  exit                        terminates the console and restores the terminal

Parameters (defaults; the four that set values are reachable at runtime)
-----------------------------------------------------------------------
  text          "CSOPESY"   set_text target; printable ASCII 0x20-0x7E
  refresh_ms    100         set_speed target; [1, 10000]
  polling_ms    10          max idle wait when nothing is ready; [1, 1000]
  developers    De La Cruz, Juan; Santos, Alex
  version_date  (blank)
  no_tty        false       --no-tty; dev/CI only

Threads
-------
  The program runs two threads: one reads your keystrokes and interprets commands, and one
  animates the marquee and owns the screen (it is the only writer of the terminal). They share
  exactly one mutex and one condition variable; `exit`/Ctrl+C stop the worker and join it before
  the terminal is restored. See docs/threading-model.md for the full contract.
