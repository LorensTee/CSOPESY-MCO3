// tests/support/check.hpp — the zero-dependency assertion harness (§T0.1 Step 1).
// No third-party framework: no install, no network, works on all three CI images. Test doubles live in
// tests/support/, never under src/ or include/.
#pragma once
#include <algorithm>
#include <cstdio>
#include <deque>
#include <string>
#include <vector>
namespace ck {
struct Case { const char* name; void (*fn)(); };
inline std::vector<Case>& registry() { static std::vector<Case> r; return r; }
inline int failures = 0;
struct Reg { Reg(const char* n, void (*f)()) { registry().push_back({n, f}); } };
}
#define TEST(name) static void name(); static ck::Reg reg_##name(#name, name); static void name()
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); ++ck::failures; } } while (0)
#define CHECK_EQ(a, b) do { auto _a = (a); auto _b = (b); if (!(_a == _b)) { \
  std::printf("FAIL %s:%d  %s == %s\n", __FILE__, __LINE__, #a, #b); ++ck::failures; } } while (0)
#define CHECK_STR(a, b) do { std::string _a = (a); std::string _b = (b); if (_a != _b) { \
  std::printf("FAIL %s:%d\n  actual:   [%s]\n  expected: [%s]\n", __FILE__, __LINE__, _a.c_str(), _b.c_str()); \
  ++ck::failures; } } while (0)
