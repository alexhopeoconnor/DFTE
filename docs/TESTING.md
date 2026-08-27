# Testing

The two PlatformIO Unity commands compile the complete DFTE test suites without uploading or executing them, so they require no attached board.

```bash
./scripts/test.sh compile --platform esp8266
./scripts/test.sh compile --platform esp32
```

The test environments include the library sources with `test_build_src = yes`. CI runs both target checks on the maintained branch and pull requests.

The standalone examples are buildable PlatformIO projects; see [examples](../examples/README.md).

Back to [documentation](README.md) · [project overview](../README.md).
