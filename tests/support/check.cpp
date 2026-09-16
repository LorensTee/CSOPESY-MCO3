// tests/support/check.cpp — the harness runner (§T0.1 Step 1). Every registered TEST() runs; a non-zero
// exit means at least one assertion failed, which is what CTest's `unit` test reports.
#include "check.hpp"
int main() {
  for (const auto& c : ck::registry()) { c.fn(); }
  if (ck::failures == 0) { std::printf("OK  %zu tests\n", ck::registry().size()); return 0; }
  std::printf("FAILED  %d assertion(s)\n", ck::failures);
  return 1;
}
