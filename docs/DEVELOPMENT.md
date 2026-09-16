# Development and releases

Released applications should use a public Git tag. While changing DFTE with a sibling library, select a local `symlink://` or `file://` dependency from an ignored PlatformIO override rather than changing tracked application dependencies.

## Target pins

The ESP32 test and example environments pin pioarduino `51.03.05`, which
selects Arduino-ESP32 3.0.5 / ESP-IDF 5.1.4+. This is a fixture contract, not a
DFTE package dependency: a consuming application owns its `platform` pin and
tests the complete framework/toolchain stack. The ESP8266 environments pin
framework commit `521ae60` for the upstream Postmortem large-jump linker fix;
the exact rationale and update rule are in the shared [ESP8266
linker-workaround note](https://github.com/alexhopeoconnor/arduino-home-assistant/blob/main/docs/ESP8266-LINKER-WORKAROUND.md).

For the pioarduino release-to-Core mapping and the narrow repair for a stale
global PlatformIO tool package, see [DeviceFramework's toolchain guide](https://github.com/alexhopeoconnor/DeviceFramework/blob/main/docs/TOOLCHAINS.md).

Start a release with `bump-version.sh`. It updates package metadata and canonical installation snippets, then creates the changelog section. Replace its generated TODO with the release summary and update any behavioural documentation before running:

```bash
./scripts/bump-version.sh vMAJOR.MINOR.PATCH
# Replace the generated CHANGELOG TODO with the release summary.
./scripts/check-docs.sh
./scripts/test.sh compile --platform esp8266
./scripts/test.sh compile --platform esp32
./scripts/test.sh examples --platform esp8266
./scripts/test.sh examples --platform esp32
./scripts/prepare-release.sh vMAJOR.MINOR.PATCH --tag
```

Push the branch and annotated tag. GitHub Actions repeats the board-free compile checks, validates the package, and creates a GitHub Release from that version’s changelog section. It does not publish to the PlatformIO Registry.

Back to [documentation](README.md) · [project overview](../README.md).
