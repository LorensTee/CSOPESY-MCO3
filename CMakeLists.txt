# CMakeLists.txt - build definition for the CSOPESY Marquee Console (Windows).
cmake_minimum_required(VERSION 3.21)
project(csopesy_marquee CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Keep the warning flags in an if/else block. One generator expression that holds several
# space-separated flags is split by the CMake argument parser and then fails at compile time.
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra)
endif()

# std::thread needs an explicit library on some compilers.
# Threads::Threads supplies the correct flag for the active compiler.
find_package(Threads REQUIRED)

add_executable(csopesy
  src/main.cpp
  src/app/console_app.cpp
  src/app/scheduler.cpp
  src/entities/cli.cpp
  src/entities/parameters.cpp
  src/entities/process.cpp
  src/features/commands/interpreter.cpp
  src/features/commands/line_editor.cpp
  src/features/marquee/renderer.cpp
  src/platform/terminal_win32.cpp)

target_include_directories(csopesy PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(csopesy PRIVATE Threads::Threads)
