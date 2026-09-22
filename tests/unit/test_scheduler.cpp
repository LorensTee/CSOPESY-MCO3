// tests/unit/test_scheduler.cpp — T1.3: the pure step, the threaded invariants (one terminal writer, one
// whole frame per write(), prompt wakeups, clean join) and the deadlines driven by term_.nowMs().
#include "check.hpp"
