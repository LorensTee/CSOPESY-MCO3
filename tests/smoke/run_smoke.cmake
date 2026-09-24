# tests/smoke/run_smoke.cmake — T2.5 Step 4. Called as:
#   cmake -DEXE=<binary> -DINFILE=<input> -P run_smoke.cmake
# execute_process (not a shell pipe) is what makes this portable across bash, cmd and pwsh, and
# $<TARGET_FILE:csopesy> is the only thing that knows where a generator put the binary.
# FILE is a CMake built-in when running with -P; do not shadow it.
execute_process(COMMAND "${EXE}" --no-tty
                INPUT_FILE "${INFILE}"
                OUTPUT_VARIABLE out ERROR_VARIABLE err
                RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "smoke: exit code ${rc}\n${err}")
endif()
foreach(needle "Available commands:" "out of range [1, 10000]; clamped to 1 ms" "Exiting CSOPESY. Goodbye!")
  string(FIND "${out}" "${needle}" pos)
  if(pos EQUAL -1)
    message(FATAL_ERROR "smoke: missing [${needle}] in output:\n${out}")
  endif()
endforeach()
message(STATUS "smoke: OK")
