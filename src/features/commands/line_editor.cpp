// src/features/commands/line_editor.cpp — implements the §3.11 line-editor rules for Interpreter (T2.3).
//
// The editable state (line_/message_/quit_) and the line editor's only named helper (visibleSlice) are
// frozen on Interpreter / interpreter.hpp, so this translation unit declares nothing of its own yet. It
// includes the two contracts it will serve, which also keeps the unit non-empty (MSVC /W4 C4206).
#include "csopesy/interpreter.hpp"
#include "csopesy/line_editor.hpp"

// TODO(T2.3): printable chars append; Backspace deletes the char before the cursor (no-op at column 0);
// Enter submits; Eof submits/quiets; arrows are ignored (no crash, no state change); plus the
// tail-window implementation of visibleSlice.
