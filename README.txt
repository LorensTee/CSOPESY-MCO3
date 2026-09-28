CSOPESY MCO3 — Marquee Console
================================

Members
-------
  Lorens   (Linux)
  Byron    (Linux + Windows)
  Nathan   (Windows)
  Kim      (macOS + Windows)

Entry file
----------
  src/main.cpp
      Contains function main().

  src/app/console_app.cpp
      Contains class csopesy::ConsoleApp.
      This class owns the application run loop.

Build requirements
------------------
  C++17 compiler
  CMake 3.21 or later
  Ninja

POSIX build
-----------
  cmake --preset debug
  cmake --build --preset debug
  ctest --preset debug

Windows build
-------------
  cmake --preset windows-vs
  cmake --build --preset windows-vs
  ctest --preset windows-vs

Release build
-------------
  This is the configuration used for the submitted artifact.

  cmake --preset release
  cmake --build --preset release

Run
---
  POSIX:
      ./build/debug/csopesy

  Windows:
      build\vs\Debug\csopesy.exe

Plain line mode
---------------
  This mode disables raw mode, ANSI output, animation, and the worker thread.
  Use it for development and CI only.

      csopesy --no-tty

Configuration file
------------------
  The optional config.txt file is read from the working directory.

  If config.txt is absent, the program uses its built-in defaults.

  Use one key=value pair per line.
  Blank lines are ignored.
  Lines that start with # are ignored.

  Recognized keys:
      refresh_ms
      polling_ms

  Unknown keys and malformed values produce a warning and are ignored.
  Values outside the allowed range are clamped and reported.

Program arguments
-----------------
  --no-tty
      Run in plain line mode. Use for development and CI only.

  --refresh-ms=N
      Set the marquee refresh interval in milliseconds.
      Allowed range: 1 to 10000.

  --poll-ms=N
      Set the maximum idle wait in milliseconds.
      Allowed range: 1 to 1000.

  Use the --flag=value form.
  Unknown flags and malformed values produce a warning.
  The parser keeps the current value when a value is invalid.

  Precedence:
      runtime command
      command-line option
      config.txt
      built-in default

Runtime commands
----------------
  Commands are case-sensitive.

  help
      Display the commands and their descriptions.

  start_marquee
      Start the marquee animation.

  stop_marquee
      Stop the marquee animation.

  set_text <text>
      Set the marquee text.
      The text must contain printable ASCII characters.

  set_speed <milliseconds>
      Set the refresh interval.
      Allowed range: 1 to 10000.

  exit
      Terminate the console and restore the terminal.

Parameters
----------
  text
      Default: CSOPESY
      set_text target.
      Printable ASCII range: 0x20 to 0x7E.

  refresh_ms
      Default: 100 ms.
      set_speed target.
      Allowed range: 1 to 10000.

  polling_ms
      Default: 10 ms.
      Maximum idle wait when no input is ready.
      Allowed range: 1 to 1000.

  developers
      Ang, Byron Scott
      Laborada, Nathan
      Sotingco, Kimbery Wynelle
      Tee, John Lorens

  version_date
      2026-09-27

  no_tty
      Default: false.
      Enabled by --no-tty.
      Development and CI only.

Threads
-------
  Normal TTY mode uses two threads.

  Input thread:
      Reads keyboard events.
      Processes commands.

  Marquee thread:
      Animates the marquee.
      Writes terminal frames.

  The threads share one mutex and one condition variable.
  The exit command and Ctrl+C stop the worker.
  The worker is joined before the terminal is restored.
