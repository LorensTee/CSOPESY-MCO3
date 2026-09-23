@echo off
REM REHEARSAL AND RECOVERY ONLY -- never graded evidence. A submitted take must show the
REM Run/Debug press on csopesy-quiz inside CLion; see the quiz-day runbook in
REM docs\IMPLEMENTATION_PLAN_v3.md, section 7. This script never builds, never modifies source,
REM and never copies case files (v3 has no config file), so it cannot recompile anything.
setlocal
cd /d "%~dp0.."

REM First hit wins: the frozen artifact is what the quiz runs, so rehearsing with it is preferred
REM once T6.3 has produced it; before that, any development build will do.
set "EXE="
if exist "frozen\csopesy.exe" set "EXE=frozen\csopesy.exe"
if not defined EXE if exist "build\release\csopesy.exe" set "EXE=build\release\csopesy.exe"
if not defined EXE if exist "build\debug\csopesy.exe" set "EXE=build\debug\csopesy.exe"
if not defined EXE if exist "build\vs\Debug\csopesy.exe" set "EXE=build\vs\Debug\csopesy.exe"
if not defined EXE if exist "cmake-build-debug\csopesy.exe" set "EXE=cmake-build-debug\csopesy.exe"

if not defined EXE goto :missing

echo rehearse_windows: launching %EXE% %*
"%EXE%" %*
exit /b %ERRORLEVEL%

:missing
echo rehearse_windows: MISSING csopesy.exe - looked for:
echo   frozen\csopesy.exe
echo   build\release\csopesy.exe
echo   build\debug\csopesy.exe
echo   build\vs\Debug\csopesy.exe
echo   cmake-build-debug\csopesy.exe
echo Build the target in CLion first - this script never builds.
exit /b 1
