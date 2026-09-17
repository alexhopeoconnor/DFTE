#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 compile|examples|packages --platform esp8266|esp32|esp32_3_3_11" >&2
    exit 2
}

[[ $# -eq 3 && ( "${1:-}" == "compile" || "${1:-}" == "examples" || "${1:-}" == "packages" ) && "${2:-}" == "--platform" ]] || usage
case "${3:-}" in
    esp8266|esp32|esp32_3_3_11) platform="$3" ;;
    *) usage ;;
esac

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
pio_for_platform() {
    if [[ "$platform" != "esp32_3_3_11" ]]; then
        pio "$@"
        return
    fi

    # Arduino-ESP32 Core 3.3.11 uses a package-form uploader. Isolate its Core-managed
    # Python environment from the legacy 3.0.5 tool-esptoolpy graph.
    local core_dir packages_dir cache_dir
    core_dir="${DFTE_PLATFORMIO_CORE_DIR:-${XDG_CACHE_HOME:-$HOME/.cache}/dfte-platformio/core-3.3.11}"
    packages_dir="${DFTE_PLATFORMIO_PACKAGES_DIR:-$core_dir/packages}"
    cache_dir="${DFTE_PLATFORMIO_CACHE_DIR:-$core_dir/cache}"
    install -d -m 700 "$core_dir" "$packages_dir" "$cache_dir"
    PLATFORMIO_CORE_DIR="$core_dir" PLATFORMIO_PACKAGES_DIR="$packages_dir" \
        PLATFORMIO_CACHE_DIR="$cache_dir" pio "$@"
}

case "$1" in
compile|packages)
    case "$platform" in
        esp8266) test_environment="test_template_engine_8266" ;;
        esp32) test_environment="test_template_engine_esp32" ;;
        esp32_3_3_11) test_environment="test_template_engine_esp32_3_3_11" ;;
    esac
    if [[ "$1" == "packages" ]]; then
        pio_for_platform pkg list -d "$root" -e "$test_environment"
        exit 0
    fi
    pio_for_platform test -d "$root" -e "$test_environment" --without-uploading --without-testing
    echo "DFTE compile check passed for $platform"
    exit 0
    ;;
esac

suffix="$platform"
mapfile -t examples < <(find "$root/examples" -mindepth 2 -maxdepth 2 -type f -name platformio.ini -printf '%h\n' | sort)
if (( ${#examples[@]} == 0 )); then
    echo "No example projects found" >&2
    exit 1
fi
for example in "${examples[@]}"; do
    env_name="example_$suffix"
    [[ "$(basename "$example")" == "AsyncDashboardDemo" ]] && env_name="dashboard_$suffix"
    pio_for_platform run -d "$example" -e "$env_name" </dev/null
done
echo "DFTE examples compile check passed for $platform"
