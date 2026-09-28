CSOPESY MCO3 - Marquee Console
==============================

Members
-------
  Ang, Byron Scott
  Laborada, Nathan
  Sotingco, Kimbery Wynelle
  Tee, John Lorens

Entry file
----------
  src/main.cpp
      Contains function main().
      It creates csopesy::ConsoleApp and runs the console.

Build requirements
------------------
  Windows 10 or Windows 11, 64-bit
  Visual Studio 2022 with the "Desktop development with C++" workload
  CMake 3.21 or later

  The program uses the C++17 standard library only.
  It has no external dependencies.

Build
-----
  Run these commands in the source directory:

      cmake --preset windows-vs
      cmake --build --preset windows-vs

  The preset selects the Visual Studio 17 2022 generator for x64.
  The build writes the Debug executable to build\vs\Debug\csopesy.exe.

Run
---
  Start the executable from the source directory:

      build\vs\Debug\csopesy.exe

  Start it from the source directory, so that the program finds config.txt.
  The program restores the terminal when it exits.

Configuration file
------------------
  The config.txt file is optional and is read from the working directory.
  If the file is absent, the program uses its built-in defaults.

  Write one key=value pair per line.
  Blank lines are ignored.
  Lines that start with # are ignored.

  Keys:
      refresh_ms    Marquee frame interval in milliseconds. Range: 1 to 10000.
      polling_ms    Maximum input wait in milliseconds. Range: 1 to 1000.

  An unknown key or a malformed value produces a warning and does not stop the program.
  A valid value outside its range is clamped and reported.

Program arguments
-----------------
  --refresh-ms=N
      Set the marquee frame interval in milliseconds. Range: 1 to 10000.

  --poll-ms=N
      Set the maximum input wait in milliseconds. Range: 1 to 1000.

  Use the --flag=value form.
  An unknown flag or a malformed value produces a warning.
  The parser keeps the current value when a value is invalid.

  The order of precedence is:
      runtime command, then program argument, then config.txt, then built-in default.

Commands
--------
  Commands are case-sensitive.

  help
      Display the commands and their descriptions.

  start_marquee
      Start the marquee animation.

  stop_marquee
      Stop the marquee animation.

  set_text <text>
      Set the marquee text. Use printable ASCII characters only.

  set_speed <milliseconds>
      Set the marquee frame interval. Range: 1 to 10000.

  exit
      Terminate the console and restore the terminal.

Default values
--------------
  text          CSOPESY
  refresh_ms    100
  polling_ms    10
