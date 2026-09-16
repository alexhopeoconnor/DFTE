#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
required=(
    README.md CHANGELOG.md
    docs/README.md docs/GETTING_STARTED.md docs/TEMPLATE_LANGUAGE.md
    docs/ASYNC_WEB.md docs/CONFIGURATION.md docs/TESTING.md docs/DEVELOPMENT.md
    examples/README.md
)

check_cpp_fence_scope() {
    local markdown="$1"
    awk '
        function brace_delta(line, copy) {
            copy = line
            return gsub(/\{/, "{", copy) - gsub(/\}/, "}", copy)
        }
        /^```cpp[[:space:]]*$/ { in_cpp = 1; depth = 0; next }
        in_cpp && /^```[[:space:]]*$/ { in_cpp = 0; next }
        in_cpp {
            line = $0
            sub(/^[[:space:]]+/, "", line)
            if (depth == 0 &&
                (line ~ /^(if|for|while|switch)[[:space:]]*\(/ ||
                 line ~ /^[A-Za-z_][A-Za-z0-9_:]*::[A-Za-z0-9_]+[[:space:]]*\(/ ||
                 line ~ /^[A-Za-z_][A-Za-z0-9_]*\./ ||
                 line ~ /^[A-Za-z_][A-Za-z0-9_]*[[:space:]]*\(/)) {
                printf "%s:%d: C++ expression appears at namespace scope; wrap it in a function.\n", FILENAME, FNR > "/dev/stderr"
                failed = 1
            }
            depth += brace_delta($0)
        }
        END { exit failed }
    ' "$markdown"
}

for path in "${required[@]}"; do
    [[ -f "$root/$path" ]] || { echo "Missing required documentation: $path" >&2; exit 1; }
done

while IFS= read -r -d '' markdown; do
    while IFS= read -r target; do
        [[ -z "$target" || "$target" == \#* || "$target" == http://* || "$target" == https://* || "$target" == mailto:* ]] && continue
        target="${target%%#*}"
        case "$target" in
            /*) candidate="$root/${target#/}" ;;
            *) candidate="$(dirname "$markdown")/$target" ;;
        esac
        [[ -e "$candidate" ]] || { echo "Broken relative link in ${markdown#$root/}: $target" >&2; exit 1; }
    done < <(sed -nE 's/.*\]\(([^ )]+)( "[^"]*")?\).*/\1/p' "$markdown")
done < <(find "$root" -path "$root/.git" -prune -o -path '*/.pio' -prune -o -name '*.md' -type f -print0)

while IFS= read -r markdown; do
    check_cpp_fence_scope "$markdown"
done < <(find "$root" -path "$root/.git" -prune -o -path '*/.pio' -prune -o -name '*.md' -type f -print)

while IFS= read -r example; do
    for required in README.md platformio.ini; do
        [[ -f "$example/$required" ]] || { echo "Incomplete example: ${example#$root/} is missing $required" >&2; exit 1; }
    done
    find "$example/src" -type f \( -name '*.ino' -o -name '*.cpp' \) -print -quit | grep -q . || {
        echo "Incomplete example: ${example#$root/} has no source" >&2
        exit 1
    }
done < <(find "$root/examples" -mindepth 2 -maxdepth 2 -type f -name platformio.ini -printf '%h\n' | sort)

while IFS= read -r markdown; do
    (( $(grep -Ec '^[[:space:]]*```' "$markdown") % 2 == 0 )) || {
        echo "Unclosed code fence in ${markdown#$root/}" >&2
        exit 1
    }
done < <(find "$root" -path "$root/.git" -prune -o -path '*/.pio' -prune -o -name '*.md' -type f -print)

echo "Documentation links and required files passed"
