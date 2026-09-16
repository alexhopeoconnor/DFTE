# Testing

The two PlatformIO Unity commands compile the complete DFTE test suites without uploading or executing them, so they require no attached board.

```bash
./scripts/test.sh compile --platform esp8266
./scripts/test.sh compile --platform esp32
./scripts/test.sh examples --platform esp8266
./scripts/test.sh examples --platform esp32
```

The first two commands compile the library test suites with `test_build_src = yes`. The example commands compile every standalone project on each target, protecting the code that the documentation links users to. CI runs the library target checks on the maintained branch and pull requests.

The standalone examples are buildable PlatformIO projects; see [examples](../examples/README.md).

Back to [documentation](README.md) · [project overview](../README.md).
