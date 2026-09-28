# CMakeLists.txt - build definition for the CSOPESY Marquee Console (Windows).
cmake_minimum_required(VERSION 3.21)
project(csopesy_marquee CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(/W4)

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
