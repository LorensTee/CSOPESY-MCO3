// src/features/commands/line_editor.cpp — implements the §3.11 line-editor rules for Interpreter (T2.3).
// The editing state and visibleSlice live on Interpreter, so this unit declares nothing of its own; it
// includes both contracts it serves, which also keeps the unit non-empty (MSVC /W4 C4206).
#include "csopesy/interpreter.hpp"
#include "csopesy/line_editor.hpp"

// TODO(T2.3): printable chars append; Backspace deletes the char before the cursor (no-op at column 0);
// Enter submits; Eof submits/quiets; arrows are ignored (no crash, no state change); plus the
// tail-window implementation of visibleSlice.
