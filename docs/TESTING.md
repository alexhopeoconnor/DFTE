# Testing

The PlatformIO Unity commands compile the complete DFTE test suites without uploading or executing them, so they require no attached board.

```bash
./scripts/test.sh compile --platform esp8266
./scripts/test.sh compile --platform esp32
./scripts/test.sh examples --platform esp8266
./scripts/test.sh examples --platform esp32
```

`esp32` uses pioarduino `55.03.311` (Arduino-ESP32 3.3.11 / ESP-IDF 5.5.5).
The `compile` commands compile the complete suites with `test_build_src = yes`; the example commands
compile every standalone project on each target, protecting the code that the
documentation links users to. CI runs every listed target lane on the
maintained branch and pull requests.

The ESP32 lane uses a persistent dedicated Core/cache directory so the
package-form Arduino-ESP32 uploader cannot inherit stale global Python
metadata. The test script never clears that cache.

The standalone examples are buildable PlatformIO projects; see [examples](../examples/README.md).

Back to [documentation](README.md) · [project overview](../README.md).
