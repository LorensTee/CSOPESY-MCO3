// include/csopesy/line_editor.hpp — §3.2 assigns features/commands a line_editor.cpp, and §T2.3 owns
// that translation unit: the raw-mode line editor (immediate echo, Backspace semantics, horizontal
// prompt scrolling, ignored arrows, Eof).
//
// It deliberately declares NO type or function of its own. §3.11 freezes the editing state on
// Interpreter (line_ / message_ / quit_) and declares the line editor's only named helper,
// visibleSlice(buffer, availWidth), in interpreter.hpp. Adding a second owner (or a second copy of
// that declaration) here would put two homes on one piece of state — exactly what §3.11's
// "Interpreter itself takes no lock; the caller owns it" rule exists to prevent.
//
// So `src/features/commands/line_editor.cpp` implements the §3.11 rules; this header stays empty of
// declarations until T2.3 (and the T0.4 freeze) decide otherwise.
#pragma once
