// include/csopesy/process.hpp — §3.8.
#pragma once
#include <string>
namespace csopesy {
enum class ProcessState { Stopped, Running };   // Ready/Finished are unused: v1 kept them as decoration

struct MarqueeProcess {                 // PCB
  int pid = 1;
  std::string name = "marquee";
  ProcessState state = ProcessState::Stopped;
  long long cycles = 0;                 // rendered frames == "instructions executed"
  long long lastRenderMs = 0;
  bool hasRendered = false;             // false => the next tick draws immediately (fresh OR restarted)
  bool start();                         // false if already Running; clears hasRendered
  bool stop();                          // false if already Stopped
};
}
