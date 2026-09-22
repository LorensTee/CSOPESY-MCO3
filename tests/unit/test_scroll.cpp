// tests/unit/test_scroll.cpp — T4.1: scrollOffset wraps at textWidth + bandWidth and stays in
// [0, textWidth + bandWidth); sliceRow is ALWAYS exactly bandWidth bytes, for short, equal, long and empty
// text. Phase 0 registers no tests on purpose: T0.1 Step 3 expects `OK  0 tests`.
#include "check.hpp"
