#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
required=(
    README.md CHANGELOG.md
    docs/README.md docs/GETTING_STARTED.md docs/TEMPLATE_LANGUAGE.md
    docs/ASYNC_WEB.md docs/CONFIGURATION.md docs/TESTING.md docs/DEVELOPMENT.md
    examples/README.md
)

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

echo "Documentation links and required files passed"
