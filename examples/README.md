# DFTE examples

Every example is a standalone PlatformIO project. Build, upload, and monitor from its directory:

```bash
pio run -d examples/HelloPlaceholder -e example_esp8266
pio run -d examples/HelloPlaceholder -e example_esp8266 -t upload
pio run -d examples/HelloPlaceholder -e example_esp8266 -t monitor
```

Choose the ESP32 environment where provided.

| Example | What it demonstrates |
| --- | --- |
| [HelloPlaceholder](HelloPlaceholder/) | Smallest registry/context/chunk flow over serial |
| [NestedLayouts](NestedLayouts/) | Templates, partials, conditionals, and iterators without a web server |
| [StreamingAsync](StreamingAsync/) | Request-scoped streaming HTTP response through a SoftAP portal |
| [AsyncDashboardDemo](AsyncDashboardDemo/) | Dashboard telemetry, iterator rows, and captive-portal flow |

The SoftAP examples print their network names and passwords to serial. They are demonstrations, not production provisioning implementations.

Back to [DFTE documentation](../docs/README.md) · [project overview](../README.md).
