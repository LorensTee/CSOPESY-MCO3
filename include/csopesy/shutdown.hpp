// include/csopesy/shutdown.hpp — platform shutdown handling.
#pragma once
namespace csopesy {

// Install the platform shutdown handlers.
void installShutdownHandlers();

// Return true once for a pending shutdown request.
// The input thread calls this function and clears the request.
bool shutdownRequested();

}
