// include/csopesy/process.hpp — marquee process state.
#pragma once
#include <string>
namespace csopesy {

enum class ProcessState {
  Stopped,
  Running
};

struct MarqueeProcess {
  int pid = 1;
  std::string name = "marquee";
  ProcessState state = ProcessState::Stopped;

  // Number of rendered marquee frames.
  long long cycles = 0;

  // Time of the last rendered frame.
  long long lastRenderMs = 0;

  // False until the next frame is rendered after start() or restart.
  bool hasRendered = false;

  // Start the process. Return false when it is already running.
  bool start();

  // Stop the process. Return false when it is already stopped.
  bool stop();
};

}
