// include/csopesy/keys.hpp — frozen contract, §3.3.
#pragma once
namespace csopesy {
enum class KeyType { None, Char, Enter, Backspace, Tab, Escape,
                     ArrowUp, ArrowDown, ArrowLeft, ArrowRight, Eof };
struct KeyEvent { KeyType type = KeyType::None; char ch = 0; };
}
