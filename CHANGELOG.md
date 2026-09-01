# Changelog

## 1.1.0

- Replace layout-affecting compile-time storage overrides with per-context,
  caller-selected rendering depth and read-buffer capacity. This keeps the
  public ABI stable across translation units while allowing constrained
  responses to use smaller storage.
- Pin ESP32 tests to the Arduino 3-compatible pioarduino platform release.

## 1.0.2

- Ensure open iterator handles are closed when rendering resets, stalls, or
  terminates with an error.
- Document the supported build-flag configuration model so every translation
  unit uses the same DFTE object layout.

## 1.0.1

- Establish the first semantic-versioned release of the maintained DFTE
  package.
