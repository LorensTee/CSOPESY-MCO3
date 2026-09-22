#!/usr/bin/env bash
# scripts/check_layers.sh — the §3.1 layer guard, wired into the CI matrix.
#
# Layer order, downward-only imports:   app > features > entities > shared
# platform/ is a peer of shared/ whose ONLY allowed import is shared/.
#
# Contract headers live flat in include/csopesy/, so a header's layer cannot be inferred from its path —
# hence the explicit map below. A header that is not in the map is a hard failure: the guard must never
# silently stop guarding a new contract header.
#
# Usage: bash scripts/check_layers.sh   (works from any directory; non-zero exit on violation)

set -u

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$root" || exit 2

violations=0

report() {
  printf 'LAYER VIOLATION: %s\n' "$1"
  violations=$((violations + 1))
}

# include/csopesy/<name> -> layer
layer_of_header() {
  case "$1" in
  keys.hpp | terminal.hpp | shutdown.hpp) echo "shared" ;;
  parameters.hpp | cli.hpp | process.hpp) echo "entities" ;;
  renderer.hpp) echo "features:marquee" ;;
  line_editor.hpp | interpreter.hpp) echo "features:commands" ;;
  scheduler.hpp | console_app.hpp) echo "app" ;;
  *) echo "unknown" ;;
  esac
}

# layer -> depth rank (higher may import lower)
rank() {
  case "$1" in
  shared) echo 0 ;;
  entities) echo 1 ;;
  features:*) echo 2 ;;
  app) echo 3 ;;
  *) echo -1 ;;
  esac
}

layer_of_source() {
  case "$1" in
  src/main.cpp | src/app/*) echo "app" ;;
  src/features/marquee/*) echo "features:marquee" ;;
  src/features/commands/*) echo "features:commands" ;;
  src/entities/*) echo "entities" ;;
  src/shared/*) echo "shared" ;;
  src/platform/*) echo "platform" ;;
  include/csopesy/*) layer_of_header "$(basename "$1")" ;;
  *) echo "unknown" ;;
  esac
}

check_file() {
  local file="$1" own inc target
  own=$(layer_of_source "$file")
  if [ "$own" = "unknown" ]; then
    report "$file: no layer mapping — add it to layer_of_source()/layer_of_header()"
    return
  fi
  # Only quoted project includes create a layer edge; <system> headers add no layer (§3.1).
  while IFS= read -r inc; do
    [ -n "$inc" ] || continue
    case "$inc" in
    platform/*)
      if [ "$own" != "platform" ]; then
        report "$file: $own may not include \"$inc\" — platform/ is imported by platform/ only"
      fi
      continue
      ;;
    csopesy/*)
      target=$(layer_of_header "${inc#csopesy/}")
      ;;
    *)
      # Bare "foo.hpp" (same-directory form): resolve it against include/csopesy/ if it names a contract.
      if [ -f "include/csopesy/$inc" ]; then
        target=$(layer_of_header "$inc")
      else
        continue
      fi
      ;;
    esac
    if [ "$target" = "unknown" ]; then
      report "$file: unknown contract header \"$inc\" — add it to layer_of_header()"
      continue
    fi
    if [ "$own" = "platform" ]; then
      if [ "$target" != "shared" ]; then
        report "$file: platform/ may include shared/ only, not \"$inc\""
      fi
      continue
    fi
    case "$own:$target" in
    features:*:features:*)
      if [ "$own" != "$target" ]; then
        report "$file: $own may not cross-import \"$inc\" — no features/ header inside another slice (§3.1)"
      fi
      ;;
    *)
      if [ "$(rank "$target")" -gt "$(rank "$own")" ]; then
        report "$file: upward include \"$inc\" ($own -> $target)"
      fi
      ;;
    esac
  done < <(grep -o '#include *"[^"]*"' "$file" | sed 's/.*"\(.*\)"/\1/')
}

while IFS= read -r file; do
  check_file "$file"
done < <(find src include/csopesy -type f \( -name '*.cpp' -o -name '*.hpp' \) | sort)

# §3.1: test doubles live in tests/support/, never in shared/ (or anywhere under src/ or include/).
while IFS= read -r file; do
  if grep -q 'FakeTerminal' "$file"; then
    report "$file: FakeTerminal is a test double and belongs in tests/support/ (§3.1)"
  fi
done < <(find src include/csopesy -type f | sort)

if [ "$violations" -eq 0 ]; then
  printf 'check_layers: OK (%s files scanned)\n' "$(find src include/csopesy -type f | wc -l | tr -d ' ')"
  exit 0
fi

printf 'check_layers: %d violation(s)\n' "$violations"
exit 1
