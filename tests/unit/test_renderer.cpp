// tests/unit/test_renderer.cpp — T4.2/T4.3: the plain-text frame layout (chrome rows, the band at kBandRow,
// the prompt row last), the tight-terminal priority rule (band and prompt are never dropped), and the
// one-complete-string-per-frame contract. No FrameBuffer and no glyph table: both are gone in v3.0 (D6, D2).
// Phase 0 registers no tests on purpose: T0.1 Step 3 expects `OK  0 tests`.
#include "check.hpp"
