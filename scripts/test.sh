#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 compile|examples --platform esp8266|esp32" >&2
    exit 2
}

[[ $# -eq 3 && ( "${1:-}" == "compile" || "${1:-}" == "examples" ) && "${2:-}" == "--platform" ]] || usage
case "${3:-}" in
    esp8266|esp32) platform="$3" ;;
    *) usage ;;
esac

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ "$1" == "compile" ]]; then
    case "$platform" in
        esp8266) test_environment="test_template_engine_8266" ;;
        esp32) test_environment="test_template_engine_esp32" ;;
    esac
    pio test -d "$root" -e "$test_environment" --without-uploading --without-testing
    echo "DFTE compile check passed for $platform"
    exit 0
fi

suffix="$platform"
mapfile -t examples < <(find "$root/examples" -mindepth 2 -maxdepth 2 -type f -name platformio.ini -printf '%h\n' | sort)
if (( ${#examples[@]} == 0 )); then
    echo "No example projects found" >&2
    exit 1
fi
for example in "${examples[@]}"; do
    env_name="example_$suffix"
    [[ "$(basename "$example")" == "AsyncDashboardDemo" ]] && env_name="dashboard_$suffix"
    pio run -d "$example" -e "$env_name" </dev/null
done
echo "DFTE examples compile check passed for $platform"
