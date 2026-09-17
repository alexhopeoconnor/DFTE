# Development and releases

Released applications should use a public Git tag. While changing DFTE with a sibling library, select a local `symlink://` or `file://` dependency from an ignored PlatformIO override rather than changing tracked application dependencies.

## Target pins

DFTE keeps two explicit ESP32 fixture lanes while consumers move to the current
framework baseline:

| Test selector | pioarduino platform | Framework stack | Purpose |
| --- | --- | --- | --- |
| `esp32` | `51.03.05` | Arduino-ESP32 3.0.5 / ESP-IDF 5.1.4+ | Deliberate legacy compatibility coverage |
| `esp32_3_3_11` | `55.03.311` | Arduino-ESP32 3.3.11 / ESP-IDF 5.5.5 | Current validation baseline |

These are fixture contracts, not DFTE package dependencies: a consuming
application owns its `platform` pin and tests the complete framework/toolchain
stack. Do not copy a compiler or toolchain package between lanes; each pinned
pioarduino platform resolves its matched framework, uploader, and toolchain.
The legacy lane is retained deliberately until the supported ESP32 floor is
changed in a reviewed compatibility decision.

All DFTE ESP8266 test and example environments pin framework commit `521ae60`
for the upstream Postmortem large-jump linker fix. The exact rationale and
update rule are in the shared [ESP8266 linker-workaround
note](https://github.com/alexhopeoconnor/arduino-home-assistant/blob/main/docs/ESP8266-LINKER-WORKAROUND.md).

For the pioarduino release-to-Core mapping and cache-collision diagnosis, see
[DeviceFramework's toolchain guide](https://github.com/alexhopeoconnor/DeviceFramework/blob/main/docs/TOOLCHAINS.md).

`./scripts/test.sh` keeps the `esp32_3_3_11` lane in a dedicated PlatformIO
Core/cache directory by default:
`${XDG_CACHE_HOME:-$HOME/.cache}/dfte-platformio/core-3.3.11`. That prevents a
legacy 3.0.5 `tool-esptoolpy` installation from shadowing the current
pioarduino uploader. Override it with `DFTE_PLATFORMIO_CORE_DIR`,
`DFTE_PLATFORMIO_PACKAGES_DIR`, and `DFTE_PLATFORMIO_CACHE_DIR` for another
disk or a disposable clean-room investigation; do not delete an unrelated
global toolchain or pin one compiler separately.

Start a release with `bump-version.sh`. It updates package metadata and canonical installation snippets, then creates the changelog section. Replace its generated TODO with the release summary and update any behavioural documentation before running:

```bash
./scripts/bump-version.sh vMAJOR.MINOR.PATCH
# Replace the generated CHANGELOG TODO with the release summary.
./scripts/check-docs.sh
./scripts/test.sh compile --platform esp8266
./scripts/test.sh compile --platform esp32
./scripts/test.sh examples --platform esp8266
./scripts/test.sh examples --platform esp32
./scripts/test.sh compile --platform esp32_3_3_11
./scripts/test.sh packages --platform esp32_3_3_11
./scripts/test.sh examples --platform esp32_3_3_11
./scripts/prepare-release.sh vMAJOR.MINOR.PATCH --tag
```

Push the branch and annotated tag. GitHub Actions repeats the board-free compile checks, validates the package, and creates a GitHub Release from that version’s changelog section. It does not publish to the PlatformIO Registry.

Back to [documentation](README.md) · [project overview](../README.md).
