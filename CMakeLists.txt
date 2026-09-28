# CMakeLists.txt — build configuration for csopesy.
cmake_minimum_required(VERSION 3.21)     # presets schema v3 + VS 17 2022 generator both require 3.21
project(csopesy_marquee CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Warning flags. Keep as if/else. A single generator expression with space-separated
# flags causes CMake argument parsing errors with GCC/Clang at compile time.
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra)
endif()

# POSIX systems require -pthread to link std::thread. Threads::Threads provides
# portable flags across POSIX and MSVC to prevent link-time undefined references.
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
# Bounded timeout prevents test execution from hanging if a worker thread join fails.
set_tests_properties(unit PROPERTIES TIMEOUT 60)
# Portable end-to-end smoke test: no shell, no `printf`, no guessed binary path, no *nix-only syntax.
# $<TARGET_FILE:csopesy> resolves to the real artifact on every generator (VS puts it in build/vs/Debug).
# It drives the real binary's --no-tty plain line mode, so it is byte-portable across bash, cmd and pwsh.
add_test(NAME smoke COMMAND ${CMAKE_COMMAND}
         -DEXE=$<TARGET_FILE:csopesy>
         -DINFILE=${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke/basic.txt
         -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke/run_smoke.cmake)
set_tests_properties(smoke PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
