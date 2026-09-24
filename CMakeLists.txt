# CMakeLists.txt — §T0.2. Reproduced from the plan verbatim; do not "simplify" it.
cmake_minimum_required(VERSION 3.21)     # presets schema v3 + VS 17 2022 generator both require 3.21
project(csopesy_marquee CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Warning flags. Written as a plain if/else ON PURPOSE. The tempting one-liner
#   add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/W4>$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>)
# is split by CMake's argument parser at the space inside the generator expression: CMake *configures* cleanly
# and then fails at compile time with `c++: error: unrecognized command-line option '-Wextra>'` (confirmed on
# CMake 4.3 + GCC 16). Do not "simplify" this back into one line.
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra)
endif()

# std::thread needs -pthread on POSIX (GCC/Clang); Threads::Threads is CMake's portable answer and a no-op on
# MSVC. Without it the POSIX build fails to LINK with `undefined reference to pthread_create` — the compile
# succeeds, so this is only caught at link time on a machine you may not be holding.
find_package(Threads REQUIRED)

set(CORE_SOURCES
  src/app/console_app.cpp src/app/scheduler.cpp
  src/features/marquee/renderer.cpp
  src/features/commands/interpreter.cpp src/features/commands/line_editor.cpp
  src/entities/parameters.cpp src/entities/cli.cpp
  src/entities/process.cpp)

if(WIN32)
  list(APPEND CORE_SOURCES src/platform/terminal_win32.cpp)
else()
  list(APPEND CORE_SOURCES src/platform/terminal_posix.cpp)   # Linux + macOS
endif()

add_library(csopesy_core STATIC ${CORE_SOURCES})
target_include_directories(csopesy_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(csopesy_core PUBLIC Threads::Threads)   # PUBLIC: csopesy and csopesy_tests both link it

add_executable(csopesy src/main.cpp)
target_link_libraries(csopesy PRIVATE csopesy_core)

enable_testing()
add_executable(csopesy_tests
  tests/support/check.cpp tests/unit/test_scroll.cpp
  tests/unit/test_process.cpp tests/unit/test_parameters.cpp tests/unit/test_interpreter.cpp
  tests/unit/test_scheduler.cpp tests/unit/test_renderer.cpp tests/unit/test_cli.cpp)
target_include_directories(csopesy_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/support)
target_link_libraries(csopesy_tests PRIVATE csopesy_core)
add_test(NAME unit COMMAND csopesy_tests)
# The scheduler tests now drive a real worker thread. A missing join would hang them forever; a bounded timeout
# turns that hang into a red test instead of a stuck CI job. 60 s is ~100x the suite's real runtime.
set_tests_properties(unit PROPERTIES TIMEOUT 60)
# The real-binary end-to-end smoke test is registered in T2.5, alongside the plain line mode that makes it pass
# (a stub `main` cannot satisfy it here, and a test that stays red for two phases trains people to ignore CI).
