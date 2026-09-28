// include/csopesy/keys.hpp — keyboard event types used by Terminal.
#pragma once
namespace csopesy {

enum class KeyType {
  None,
  Char,
  Enter,
  Backspace,
  Tab,
  Escape,
  ArrowUp,
  ArrowDown,
  ArrowLeft,
  ArrowRight,
  Eof
};

struct KeyEvent {
  KeyType type = KeyType::None;
  char ch = 0;  // Character value for KeyType::Char.
};

}
