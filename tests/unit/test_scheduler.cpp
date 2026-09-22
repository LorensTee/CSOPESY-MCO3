// tests/unit/test_scheduler.cpp — T1.3: the pure step, the threaded invariants (one writer of the terminal,
// one whole frame per write(), prompt wakeups, clean join) and the deadlines driven by term_.nowMs() (D7).
// There is no --measure contract any more: the PPT's measurement is a manual sweep (D4, D14).
// Phase 0 registers no tests on purpose: T0.1 Step 3 expects `OK  0 tests`.
#include "check.hpp"
