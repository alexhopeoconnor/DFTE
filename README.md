# Device Framework Template Engine (DFTE)

Device Framework Template Engine provides a lightweight templating layer for Arduino-based projects within the Device Framework ecosystem. It exposes a registry for dynamic placeholders, a rendering engine, and utilities for composing nested templates.

## Structure

- `include/` and `src/` contain the core template engine components.
- `test/test_template_engine/` hosts PlatformIO-based unit tests covering placeholder registration, context handling, renderer behaviours, and integration scenarios.

## Getting Started

The library is distributed as a PlatformIO library. To use it in another PlatformIO project, add the repository as a dependency in your `platformio.ini`:

```
lib_deps =
    alexhopeoconnor/DFTE
```

To develop locally or contribute, clone the repository and run the tests through PlatformIO:

```
pio test -e test_template_engine
```

## License

This project is released under the MIT License. See `LICENSE` for details.
