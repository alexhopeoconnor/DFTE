#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 vMAJOR.MINOR.PATCH" >&2
    exit 2
}

tag="${1:-}"
[[ "$tag" =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]] || usage

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
version="${tag#v}"
changelog="$root/CHANGELOG.md"
output="$(mktemp)"
trap 'rm -f "$output"' EXIT

awk -v version="$version" '
    $0 == "## " version { capture = 1; next }
    capture && /^## / { exit }
    capture { print }
' "$changelog" > "$output"

if [[ ! -s "$output" ]]; then
    echo "No release notes found for $tag in CHANGELOG.md" >&2
    exit 1
fi

printf '%s\n\n' "# DeviceFrameworkTemplateEngine $tag"
cat "$output"
