# Testing

The three PlatformIO Unity commands compile the complete DFTE test suites without uploading or executing them, so they require no attached board.

```bash
./scripts/test.sh compile --platform esp8266
./scripts/test.sh compile --platform esp32
./scripts/test.sh examples --platform esp8266
./scripts/test.sh examples --platform esp32
./scripts/test.sh compile --platform esp32_3_3_11
./scripts/test.sh packages --platform esp32_3_3_11
./scripts/test.sh examples --platform esp32_3_3_11
```

`esp32` is the explicit Arduino-ESP32 3.0.5 compatibility lane.
`esp32_3_3_11` uses pioarduino `55.03.311` (Arduino-ESP32 3.3.11 / ESP-IDF
5.5.5) as the current validation baseline. The three `compile` commands
compile the complete suites with `test_build_src = yes`; the example commands
compile every standalone project on each target, protecting the code that the
documentation links users to. CI runs every listed target lane on the
maintained branch and pull requests.

The current lane is isolated by the script in a dedicated Core/cache directory
so the package-form Arduino-ESP32 Core 3.3.11 uploader cannot inherit legacy 3.0.5 Python
metadata. `packages` prints the resolved package graph after the current build;
it is a verification step, not a request to change a compiler pin.

The standalone examples are buildable PlatformIO projects; see [examples](../examples/README.md).

Back to [documentation](README.md) · [project overview](../README.md).
