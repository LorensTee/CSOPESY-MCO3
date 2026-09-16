// include/csopesy/shutdown.hpp — §3.10. Implemented once, in the selected src/platform/* file.
#pragma once
namespace csopesy {
void installShutdownHandlers();   // POSIX: SIGINT/SIGTERM -> flag. Win32: SetConsoleCtrlHandler -> flag
bool shutdownRequested();         // read-and-clear; called by the INPUT thread only (§3.8, §3.10)
}
